// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2memorytransfer_p.h"

#include <QtMultimedia/private/qv4l2camera_p.h>
#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qv4l2camera_p.h>
#include <QtMultimedia/private/qv4l2filedescriptor_p.h>
#include <QtMultimedia/private/qv4l2formats_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qvarlengtharray.h>

#include <sys/mman.h>

#include <cerrno>
#include <cstddef>
#include <memory>
#include <optional>
#include <system_error>
#include <utility>
#include <vector>

#if QT_CONFIG(linux_dmabuf)
#  include <QtMultimedia/private/qcaptureslotregistry_p.h>
#  include <QtMultimedia/private/qdmabufallocator_p.h>
#  include <QtMultimedia/private/qdmabufsource_p.h>
#  include <QtMultimedia/private/qdmabufvideobuffer_p.h>
#  include <QtMultimedia/private/qmaterializingvideobuffer_p.h>
#  include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#endif

QT_BEGIN_NAMESPACE

namespace QtV4L2 {

Q_STATIC_LOGGING_CATEGORY(qLcV4L2MemoryTransfer, "qt.multimedia.v4l2.memorytransfer");

namespace {

v4l2_buffer makeV4l2Buffer(v4l2_memory memoryType, quint32 index = 0)
{
    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = memoryType;
    buf.index = index;
    return buf;
}

class UserPtrMemoryTransfer : public QV4L2MemoryTransfer
{
public:
    static q23::expected<QV4L2MemoryTransferUPtr, std::error_code>
    create(QV4L2FileDescriptorPtr fileDescriptor, quint32 imageSize, int bytesPerLine)
    {
        auto requestResult = fileDescriptor->requestBuffers(V4L2_MEMORY_USERPTR, 2);
        if (!requestResult) {
            qCDebug(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_USERPTR buffers";
            return q23::unexpected(requestResult.error());
        }

        std::unique_ptr<UserPtrMemoryTransfer> result(new UserPtrMemoryTransfer{
                std::move(fileDescriptor),
                quint32(*requestResult),
                imageSize,
                bytesPerLine,
        });

        auto enqueueResult = result->enqueueBuffers();
        if (!enqueueResult)
            return q23::unexpected(enqueueResult.error());

        return result;
    }

    ~UserPtrMemoryTransfer() override { fileDescriptor().releaseAllBuffers(V4L2_MEMORY_USERPTR); }

    q23::expected<Frame, std::error_code> dequeueFrame() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_USERPTR);
        auto dequeueResult = ioctl(VIDIOC_DQBUF, &v4l2Buffer);
        if (!dequeueResult)
            return q23::unexpected(dequeueResult.error());

        Q_ASSERT(v4l2Buffer.index < m_byteArrays.size());

        auto &byteArray = m_byteArrays[v4l2Buffer.index];

        Q_ASSERT(!byteArray.isEmpty());
        Q_ASSERT(qsizetype(v4l2Buffer.bytesused) <= byteArray.size());

        // truncate jpeg
        byteArray.resize(v4l2Buffer.bytesused);
        Frame frame{
            v4l2Buffer.timestamp,
            std::make_unique<QMemoryVideoBuffer>(std::move(byteArray), m_bytesPerLine),
        };

        if (auto enqueueResult = enqueueBuffer(v4l2Buffer.index); !enqueueResult)
            qCWarning(qLcV4L2MemoryTransfer)
                    << "Cannot add buffer:" << qt_error_string(enqueueResult.error().value());

        return frame;
    }

    quint32 buffersCount() const override { return static_cast<quint32>(m_byteArrays.size()); }

private:
    UserPtrMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, quint32 buffersCount,
                          quint32 imageSize, int bytesPerLine)
        : QV4L2MemoryTransfer(std::move(fileDescriptor)),
          m_imageSize(imageSize),
          m_bytesPerLine(bytesPerLine),
          m_byteArrays(buffersCount)
    {
    }

    q23::expected<void, std::error_code> enqueueBuffer(quint32 index) override
    {
        Q_ASSERT(index < m_byteArrays.size());
        Q_ASSERT(m_byteArrays[index].isEmpty());

        auto buf = makeV4l2Buffer(V4L2_MEMORY_USERPTR, index);
        static_assert(sizeof(decltype(buf.m.userptr)) == sizeof(size_t), "Not compatible sizes");

        m_byteArrays[index] = QByteArray(static_cast<int>(m_imageSize), Qt::Uninitialized);

        buf.m.userptr = (decltype(buf.m.userptr))m_byteArrays[index].data();
        buf.length = m_byteArrays[index].size();

        auto result = ioctl(VIDIOC_QBUF, &buf);
        if (!result) {
            qCWarning(qLcV4L2MemoryTransfer)
                    << "Couldn't add V4L2 buffer" << result.error() << index;
            return q23::unexpected(result.error());
        }

        return {};
    }

    const quint32 m_imageSize;
    const int m_bytesPerLine;
    std::vector<QByteArray> m_byteArrays;
};

class MMapMemoryTransfer : public QV4L2MemoryTransfer
{
public:
    struct MemorySpan
    {
        void *data = nullptr;
        size_t size = 0;
        bool inQueue = false;
    };

    static q23::expected<QV4L2MemoryTransferUPtr, std::error_code>
    create(QV4L2FileDescriptorPtr fileDescriptor, int bytesPerLine)
    {
        auto requestResult = fileDescriptor->requestBuffers(V4L2_MEMORY_MMAP, 2);
        if (!requestResult) {
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_MMAP buffers";
            return q23::unexpected(requestResult.error());
        }

        std::unique_ptr<MMapMemoryTransfer> result(
                new MMapMemoryTransfer(std::move(fileDescriptor), bytesPerLine));

        auto initResult = result->init(*requestResult);
        if (!initResult)
            return q23::unexpected(initResult.error());

        return result;
    }

    q23::expected<void, std::error_code> init(quint32 buffersCount)
    {
        for (quint32 index = 0; index < buffersCount; ++index) {
            auto buf = makeV4l2Buffer(V4L2_MEMORY_MMAP, index);

            auto queryResult = ioctl(VIDIOC_QUERYBUF, &buf);
            if (!queryResult) {
                qCWarning(qLcV4L2MemoryTransfer) << "Can't map buffer" << index
                                                 << qt_error_string(queryResult.error().value());
                return q23::unexpected(queryResult.error());
            }

            auto mappedData = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                   fileDescriptor().get(), buf.m.offset);

            if (mappedData == MAP_FAILED) {
                const auto error = std::error_code{ errno, std::system_category() };
                qCWarning(qLcV4L2MemoryTransfer) << "mmap failed" << index << buf.length
                                                 << buf.m.offset << qt_error_string(error.value());
                return q23::unexpected(error);
            }

            m_spans.push_back(MemorySpan{ mappedData, buf.length, false });
        }

        m_spans.shrink_to_fit();

        return enqueueBuffers();
    }

    ~MMapMemoryTransfer() override
    {
        for (const auto &span : m_spans)
            munmap(span.data, span.size);
        fileDescriptor().releaseAllBuffers(V4L2_MEMORY_MMAP);
    }

    q23::expected<Frame, std::error_code> dequeueFrame() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_MMAP);
        auto dequeueResult = ioctl(VIDIOC_DQBUF, &v4l2Buffer);
        if (!dequeueResult)
            return q23::unexpected(dequeueResult.error());

        const auto index = v4l2Buffer.index;

        Q_ASSERT(index < m_spans.size());

        auto &span = m_spans[index];

        Q_ASSERT(span.inQueue);
        span.inQueue = false;

        Q_ASSERT(v4l2Buffer.bytesused <= span.size);

        // truncate jpeg
        QByteArray byteArray(reinterpret_cast<const char *>(span.data), v4l2Buffer.bytesused);

        Frame frame{
            v4l2Buffer.timestamp,
            std::make_unique<QMemoryVideoBuffer>(std::move(byteArray), m_bytesPerLine),
        };

        if (auto enqueueResult = enqueueBuffer(index); !enqueueResult)
            qCWarning(qLcV4L2MemoryTransfer)
                    << "Cannot add buffer:" << qt_error_string(enqueueResult.error().value());

        return frame;
    }

    quint32 buffersCount() const override { return static_cast<quint32>(m_spans.size()); }

private:
    MMapMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, int bytesPerLine)
        : QV4L2MemoryTransfer(std::move(fileDescriptor)), m_bytesPerLine(bytesPerLine)
    {
    }

    q23::expected<void, std::error_code> enqueueBuffer(quint32 index) override
    {
        Q_ASSERT(index < m_spans.size());
        Q_ASSERT(!m_spans[index].inQueue);

        auto buf = makeV4l2Buffer(V4L2_MEMORY_MMAP, index);
        auto result = ioctl(VIDIOC_QBUF, &buf);
        if (!result)
            return q23::unexpected(result.error());

        m_spans[index].inQueue = true;
        return {};
    }

    const int m_bytesPerLine;
    std::vector<MemorySpan> m_spans;
};

#if QT_CONFIG(linux_dmabuf)

constexpr std::size_t kDmaBufBuffersCount = 8;
constexpr std::size_t kDmaBufMinFreeBuffers = 2;

// Owns the DMA-BUFs and hands out their indices as capture slots. A frame holding a slot keeps
// its buffer queued-out of the driver; releasing the slot re-queues it.
class DmaBufRegistry final
    : public QtMultimediaPrivate::CaptureSlotRegistry<DmaBufRegistry, uint8_t, kDmaBufBuffersCount>
{
public:
    using QDmaBuf = QtMultimediaPrivate::QDmaBuf;

    DmaBufRegistry(QV4L2FileDescriptorPtr fileDescriptor,
                   std::vector<std::shared_ptr<QDmaBuf>> buffers)
        : CaptureSlotRegistry(buffers.size(), kDmaBufMinFreeBuffers),
          m_fileDescriptor(std::move(fileDescriptor)),
          m_buffers(std::move(buffers))
    {
        Q_ASSERT(m_buffers.size() == kDmaBufBuffersCount);
    }

    q23::expected<void, std::error_code> queueBuffer(quint32 index) const
    {
        const QDmaBuf &buf = *m_buffers[index];

        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_DMABUF, index);
        v4l2Buffer.m.fd = buf.fd();
        v4l2Buffer.length = uint32_t(buf.size());

        auto result = m_fileDescriptor->call(VIDIOC_QBUF, &v4l2Buffer);
        if (!result) {
            qCWarning(qLcV4L2MemoryTransfer)
                    << "Couldn't add V4L2 buffer" << result.error() << index;
            return q23::unexpected(result.error());
        }

        return {};
    }

    void recycleSlot(uint8_t index) const
    {
        if (auto result = queueBuffer(index); !result)
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot add buffer:" << result.error();
    }

    std::shared_ptr<QDmaBuf> buffer(quint32 index) const { return m_buffers[index]; }
    quint32 buffersCount() const { return quint32(m_buffers.size()); }

private:
    const QV4L2FileDescriptorPtr m_fileDescriptor;
    const std::vector<std::shared_ptr<QDmaBuf>> m_buffers;
};

class DmaBufMemoryTransfer final : public QV4L2MemoryTransfer
{
public:
    using QDmaBuf = QtMultimediaPrivate::QDmaBuf;
    using QDmaBufAllocator = QtMultimediaPrivate::QDmaBufAllocator;

    static q23::expected<QV4L2MemoryTransferUPtr, std::error_code>
    create(const QV4L2FileDescriptorPtr &fileDescriptor, quint32 imageSize, int bytesPerLine,
           const QVideoFrameFormat &format)
    {
        using namespace QtMultimediaPrivate;

        auto allocator = QDmaBufAllocator::create();
        if (!allocator) {
            qCDebug(qLcV4L2MemoryTransfer)
                    << "Cannot create DMA-BUF allocator:" << allocator.error();
            return q23::unexpected(std::make_error_code(std::errc::not_enough_memory));
        }

        auto requestResult =
                fileDescriptor->requestBuffers(V4L2_MEMORY_DMABUF, kDmaBufBuffersCount);
        if (!requestResult) {
            qCDebug(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_DMABUF buffers";
            return q23::unexpected(requestResult.error());
        }

        std::vector<std::shared_ptr<QDmaBuf>> buffers;

        for (quint32 index = 0; index < quint32(*requestResult); ++index) {
            auto buf = allocator->allocate(imageSize);
            if (!buf) {
                qCDebug(qLcV4L2MemoryTransfer) << "Cannot allocate DMA-BUF:" << buf.error();
                fileDescriptor->releaseAllBuffers(V4L2_MEMORY_DMABUF);
                return q23::unexpected(std::make_error_code(std::errc::not_enough_memory));
            }
            buffers.push_back(std::move(*buf));
        }

        std::optional<DmaBufLayout> layout = dmaBufLayoutForFormat(
                format.pixelFormat(), format.frameSize(), bytesPerLine, imageSize);

        std::unique_ptr<DmaBufMemoryTransfer> result(new DmaBufMemoryTransfer(
                fileDescriptor,
                std::make_shared<DmaBufRegistry>(fileDescriptor, std::move(buffers)), format,
                bytesPerLine, std::move(layout)));

        auto enqueueResult = result->enqueueBuffers();
        if (!enqueueResult) {
            qCDebug(qLcV4L2MemoryTransfer) << "Cannot queue V4L2_MEMORY_DMABUF buffers";
            return q23::unexpected(enqueueResult.error());
        }

        return result;
    }

    ~DmaBufMemoryTransfer() override
    {
        // Frames still holding a slot must not re-queue buffers once the stream is stopped.
        m_registry->stop();
        fileDescriptor().requestBuffers(V4L2_MEMORY_DMABUF, 0);
    }

    q23::expected<Frame, std::error_code> dequeueFrame() override
    {
        using namespace QtMultimediaPrivate;
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_DMABUF);
        auto dequeueResult = ioctl(VIDIOC_DQBUF, &v4l2Buffer);
        if (!dequeueResult)
            return q23::unexpected(dequeueResult.error());

        const quint32 index = v4l2Buffer.index;
        std::shared_ptr<QDmaBuf> buf = m_registry->buffer(index);

        std::unique_ptr<QAbstractVideoBuffer> videoBuffer;
        if (m_layout) {
            // Zero-copy: one DmaBufPlane per layout plane, all within the same DMA-BUF fd.
            const auto drmFormats = dmaBufFourccFromPixelFormat(m_format.pixelFormat());
            QVarLengthArray<DmaBufPlane, 4> planes;
            for (qsizetype plane = 0; plane < m_layout->size(); ++plane) {
                const DmaBufPlaneLayout &layout = (*m_layout)[plane];
                planes.push_back({
                        buf->fd(),
                        layout.offset,
                        layout.pitch,
                        drmFormats[plane],
                });
            }
            const QSpan<const DmaBufPlane> planeSpan(planes.data(), planes.size());

            using Buffer = QtMultimediaPrivate::MaterializingVideoBuffer<
                    DmaBufSource, DmaBufRegistry, QtMultimediaPrivate::QDmaBufVideoBuffer>;

            videoBuffer = Buffer::wrapOrMaterialize(*m_registry, index,
                                                    DmaBufSource(buf, *m_layout), m_format,
                                                    // QDmaBufVideoBuffer args:
                                                    m_format, planeSpan, buf);
        } else {
            // no zero-copy format: wrap the DMA-BUF into a plain MaterializingVideoBuffer without
            // zero-copy machinery
            const DmaBufLayout layout{
                DmaBufPlaneLayout{
                        0,
                        uint32_t(m_bytesPerLine),
                        qsizetype(v4l2Buffer.bytesused),
                },
            };

            using Buffer =
                    QtMultimediaPrivate::MaterializingVideoBuffer<DmaBufSource, DmaBufRegistry>;

            videoBuffer = Buffer::wrapOrMaterialize(*m_registry, index, DmaBufSource(buf, layout),
                                                    m_format);
        }

        return Frame{
            v4l2Buffer.timestamp,
            std::move(videoBuffer),
        };
    }

    quint32 buffersCount() const override { return m_registry->buffersCount(); }

private:
    using DmaBufLayout = QtMultimediaPrivate::DmaBufLayout;

    DmaBufMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor,
                         std::shared_ptr<DmaBufRegistry> registry, QVideoFrameFormat format,
                         int bytesPerLine, std::optional<DmaBufLayout> layout)
        : QV4L2MemoryTransfer(std::move(fileDescriptor)),
          m_registry(std::move(registry)),
          m_format(std::move(format)),
          m_bytesPerLine(bytesPerLine),
          m_layout(std::move(layout))
    {
    }

    q23::expected<void, std::error_code> enqueueBuffer(quint32 index) override
    {
        return m_registry->queueBuffer(index);
    }

    const std::shared_ptr<DmaBufRegistry> m_registry;
    const QVideoFrameFormat m_format;
    const int m_bytesPerLine;
    const std::optional<DmaBufLayout> m_layout;
};

#endif

} // namespace

QV4L2MemoryTransfer::QV4L2MemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor)
    : m_fileDescriptor(std::move(fileDescriptor))
{
    Q_ASSERT(m_fileDescriptor);
    Q_ASSERT(!m_fileDescriptor->streamStarted());
}

QV4L2MemoryTransfer::~QV4L2MemoryTransfer()
{
    Q_ASSERT(!m_fileDescriptor->streamStarted()); // to avoid possible corruptions
}

q23::expected<void, std::error_code> QV4L2MemoryTransfer::enqueueBuffers()
{
    for (quint32 i = 0; i < buffersCount(); ++i) {
        if (auto result = enqueueBuffer(i); !result)
            return q23::unexpected(result.error());
    }

    return {};
}

q23::expected<int, std::error_code> QV4L2MemoryTransfer::ioctl(int request, void *arg)
{
    return m_fileDescriptor->call(request, arg);
}

q23::expected<QV4L2MemoryTransferUPtr, std::error_code>
makeUserPtrMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, const v4l2_pix_format &format)
{
    return UserPtrMemoryTransfer::create(std::move(fileDescriptor), format.sizeimage,
                                         int(format.bytesperline));
}

q23::expected<QV4L2MemoryTransferUPtr, std::error_code>
makeMMapMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, const v4l2_pix_format &format)
{
    return MMapMemoryTransfer::create(std::move(fileDescriptor), int(format.bytesperline));
}

#if QT_CONFIG(linux_dmabuf)
q23::expected<QV4L2MemoryTransferUPtr, std::error_code>
makeDmaBufMemoryTransfer(const QV4L2FileDescriptorPtr &fileDescriptor,
                         const v4l2_pix_format &format)
{
    const QVideoFrameFormat frameFormat(QSize(format.width, format.height),
                                        formatForV4L2Format(format.pixelformat));
    return DmaBufMemoryTransfer::create(fileDescriptor, format.sizeimage, int(format.bytesperline),
                                        frameFormat);
}
#endif

} // namespace QtV4L2

QT_END_NAMESPACE
