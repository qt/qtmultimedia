// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2memorytransfer_p.h"

#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qv4l2camera_p.h>
#include <QtMultimedia/private/qv4l2filedescriptor_p.h>
#include <QtMultimedia/private/qv4l2formats_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qvarlengtharray.h>

#include <sys/mman.h>

#include <optional>
#include <vector>

#if QT_CONFIG(linux_dmabuf)
#  include <QtMultimedia/private/qdmabufallocator_p.h>
#  include <QtMultimedia/private/qmultimedia_dmabuf_support_p.h>
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
    static QV4L2MemoryTransferUPtr create(QV4L2FileDescriptorPtr fileDescriptor, quint32 imageSize,
                                          int bytesPerLine)
    {
        if (auto requestResult = fileDescriptor->requestBuffers(V4L2_MEMORY_USERPTR, 2)) {
            std::unique_ptr<UserPtrMemoryTransfer> result(new UserPtrMemoryTransfer{
                    std::move(fileDescriptor),
                    quint32(*requestResult),
                    imageSize,
                    bytesPerLine,
            });

            return result->enqueueBuffers() ? std::move(result) : nullptr;
        } else {
            qCDebug(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_USERPTR buffers";
            return {};
        }
    }

    ~UserPtrMemoryTransfer() override { fileDescriptor().releaseAllBuffers(V4L2_MEMORY_USERPTR); }

    std::optional<Frame> dequeueFrame() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_USERPTR);
        if (!ioctl(VIDIOC_DQBUF, &v4l2Buffer))
            return {};

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

        if (!enqueueBuffer(v4l2Buffer.index))
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot add buffer";

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

    bool enqueueBuffer(quint32 index) override
    {
        Q_ASSERT(index < m_byteArrays.size());
        Q_ASSERT(m_byteArrays[index].isEmpty());

        auto buf = makeV4l2Buffer(V4L2_MEMORY_USERPTR, index);
        static_assert(sizeof(decltype(buf.m.userptr)) == sizeof(size_t), "Not compatible sizes");

        m_byteArrays[index] = QByteArray(static_cast<int>(m_imageSize), Qt::Uninitialized);

        buf.m.userptr = (decltype(buf.m.userptr))m_byteArrays[index].data();
        buf.length = m_byteArrays[index].size();

        if (!ioctl(VIDIOC_QBUF, &buf)) {
            qWarning() << "Couldn't add V4L2 buffer" << errno << qt_error_string(errno) << index;
            return false;
        }

        return true;
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

    static QV4L2MemoryTransferUPtr create(QV4L2FileDescriptorPtr fileDescriptor, int bytesPerLine)
    {
        auto requestResult = fileDescriptor->requestBuffers(V4L2_MEMORY_MMAP, 2);
        if (!requestResult) {
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_MMAP buffers";
            return {};
        }

        std::unique_ptr<MMapMemoryTransfer> result(
                new MMapMemoryTransfer(std::move(fileDescriptor), bytesPerLine));

        return result->init(*requestResult) ? std::move(result) : nullptr;
    }

    bool init(quint32 buffersCount)
    {
        for (quint32 index = 0; index < buffersCount; ++index) {
            auto buf = makeV4l2Buffer(V4L2_MEMORY_MMAP, index);

            if (!ioctl(VIDIOC_QUERYBUF, &buf)) {
                qWarning() << "Can't map buffer" << index;
                return false;
            }

            auto mappedData = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                   fileDescriptor().get(), buf.m.offset);

            if (mappedData == MAP_FAILED) {
                qWarning() << "mmap failed" << index << buf.length << buf.m.offset;
                return false;
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

    std::optional<Frame> dequeueFrame() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_MMAP);
        if (!ioctl(VIDIOC_DQBUF, &v4l2Buffer))
            return {};

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

        if (!enqueueBuffer(index))
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot add buffer";

        return frame;
    }

    quint32 buffersCount() const override { return static_cast<quint32>(m_spans.size()); }

private:
    MMapMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, int bytesPerLine)
        : QV4L2MemoryTransfer(std::move(fileDescriptor)), m_bytesPerLine(bytesPerLine)
    {
    }

    bool enqueueBuffer(quint32 index) override
    {
        Q_ASSERT(index < m_spans.size());
        Q_ASSERT(!m_spans[index].inQueue);

        auto buf = makeV4l2Buffer(V4L2_MEMORY_MMAP, index);
        if (!ioctl(VIDIOC_QBUF, &buf))
            return false;

        m_spans[index].inQueue = true;
        return true;
    }

    const int m_bytesPerLine;
    std::vector<MemorySpan> m_spans;
};

#if QT_CONFIG(linux_dmabuf)

constexpr quint32 kDmaBufBuffersCount = 4;

class DmaBufMemoryTransfer : public QV4L2MemoryTransfer
{
public:
    using QDmaBuf = QtMultimediaPrivate::QDmaBuf;
    using QDmaBufAllocator = QtMultimediaPrivate::QDmaBufAllocator;
    using QDmaBufVector = QVarLengthArray<std::shared_ptr<QDmaBuf>, kDmaBufBuffersCount>;

    static QV4L2MemoryTransferUPtr create(const QV4L2FileDescriptorPtr &fileDescriptor,
                                          quint32 imageSize, int bytesPerLine)
    {
        auto allocator = QDmaBufAllocator::create();
        if (!allocator) {
            qCDebug(qLcV4L2MemoryTransfer)
                    << "Cannot create DMA-BUF allocator:" << allocator.error();
            return {};
        }

        auto requestResult =
                fileDescriptor->requestBuffers(V4L2_MEMORY_DMABUF, kDmaBufBuffersCount);
        if (!requestResult) {
            qCDebug(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_DMABUF buffers";
            return {};
        }

        QDmaBufVector buffers;

        for (quint32 index = 0; index < quint32(*requestResult); ++index) {
            auto buf = allocator->allocate(imageSize);
            if (!buf) {
                qCDebug(qLcV4L2MemoryTransfer) << "Cannot allocate DMA-BUF:" << buf.error();
                fileDescriptor->requestBuffers(V4L2_MEMORY_DMABUF, 0);
                return {};
            }
            buffers.push_back(std::move(*buf));
        }

        std::unique_ptr<DmaBufMemoryTransfer> result(
                new DmaBufMemoryTransfer(fileDescriptor, std::move(buffers), bytesPerLine));

        if (result->enqueueBuffers())
            return result;

        qCDebug(qLcV4L2MemoryTransfer) << "Cannot queue V4L2_MEMORY_DMABUF buffers";
        fileDescriptor->requestBuffers(V4L2_MEMORY_DMABUF, 0);
        return {};
    }

    ~DmaBufMemoryTransfer() override { fileDescriptor().releaseAllBuffers(V4L2_MEMORY_DMABUF); }

    std::optional<Frame> dequeueFrame() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_DMABUF);
        if (!ioctl(VIDIOC_DQBUF, &v4l2Buffer))
            return {};

        const quint32 index = v4l2Buffer.index;
        Q_ASSERT(index < m_buffers.size());
        const QDmaBuf &buf = *m_buffers[index];

        // Copy out before re-queueing, so the driver cannot overwrite the frame while it is read.
        auto data = QtMultimediaPrivate::copyDmaBufData(buf, 0, v4l2Buffer.bytesused);

        if (!enqueueBuffer(index))
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot add buffer";

        if (!data)
            return {};

        return Frame{
            v4l2Buffer.timestamp,
            std::make_unique<QMemoryVideoBuffer>(std::move(*data), m_bytesPerLine),
        };
    }

    quint32 buffersCount() const override { return quint32(m_buffers.size()); }

private:
    DmaBufMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, QDmaBufVector buffers,
                         int bytesPerLine)
        : QV4L2MemoryTransfer(std::move(fileDescriptor)),
          m_bytesPerLine(bytesPerLine),
          m_buffers(std::move(buffers))
    {
    }

    bool enqueueBuffer(quint32 index) override
    {
        Q_ASSERT(index < m_buffers.size());

        const auto &buf = *m_buffers[index];

        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_DMABUF, index);
        v4l2Buffer.m.fd = buf.fd();
        v4l2Buffer.length = quint32(buf.size());

        if (!ioctl(VIDIOC_QBUF, &v4l2Buffer)) {
            qWarning() << "Couldn't add V4L2 buffer" << errno << qt_error_string(errno) << index;
            return false;
        }

        return true;
    }

    const int m_bytesPerLine;
    QDmaBufVector m_buffers;
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

bool QV4L2MemoryTransfer::enqueueBuffers()
{
    for (quint32 i = 0; i < buffersCount(); ++i)
        if (!enqueueBuffer(i))
            return false;

    return true;
}

q23::expected<int, std::error_code> QV4L2MemoryTransfer::ioctl(int request, void *arg)
{
    return m_fileDescriptor->call(request, arg);
}

QV4L2MemoryTransferUPtr makeUserPtrMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor,
                                                  const v4l2_pix_format &format)
{
    return UserPtrMemoryTransfer::create(std::move(fileDescriptor), format.sizeimage,
                                         int(format.bytesperline));
}

QV4L2MemoryTransferUPtr makeMMapMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor,
                                               const v4l2_pix_format &format)
{
    return MMapMemoryTransfer::create(std::move(fileDescriptor), int(format.bytesperline));
}

#if QT_CONFIG(linux_dmabuf)
QV4L2MemoryTransferUPtr makeDmaBufMemoryTransfer(const QV4L2FileDescriptorPtr &fileDescriptor,
                                                 const v4l2_pix_format &format)
{
    return DmaBufMemoryTransfer::create(fileDescriptor, format.sizeimage, int(format.bytesperline));
}
#endif

} // namespace QtV4L2

QT_END_NAMESPACE
