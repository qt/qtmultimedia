// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2memorytransfer_p.h"
#include "qv4l2filedescriptor_p.h"

#include <qloggingcategory.h>
#include <qdebug.h>
#include <sys/mman.h>
#include <optional>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcV4L2MemoryTransfer, "qt.multimedia.ffmpeg.v4l2camera.memorytransfer");

namespace {

v4l2_buffer makeV4l2Buffer(quint32 memoryType, quint32 index = 0)
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
    static QV4L2MemoryTransferUPtr create(QV4L2FileDescriptorPtr fileDescriptor, quint32 imageSize)
    {
        quint32 buffersCount = 2;
        if (!fileDescriptor->requestBuffers(V4L2_MEMORY_USERPTR, buffersCount)) {
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_USERPTR buffers";
            return {};
        }

        std::unique_ptr<UserPtrMemoryTransfer> result(
                new UserPtrMemoryTransfer(std::move(fileDescriptor), buffersCount, imageSize));

        return result->enqueueBuffers() ? std::move(result) : nullptr;
    }

    std::optional<Buffer> dequeueBuffer() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_USERPTR);
        if (!fileDescriptor().call(VIDIOC_DQBUF, &v4l2Buffer))
            return {};

        Q_ASSERT(v4l2Buffer.index < m_byteArrays.size());
        Q_ASSERT(!m_byteArrays[v4l2Buffer.index].isEmpty());

        return Buffer{ v4l2Buffer, std::move(m_byteArrays[v4l2Buffer.index]) };
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

        if (!fileDescriptor().call(VIDIOC_QBUF, &buf)) {
            qWarning() << "Couldn't add V4L2 buffer" << errno << strerror(errno) << index;
            return false;
        }

        return true;
    }

    quint32 buffersCount() const override { return static_cast<quint32>(m_byteArrays.size()); }

private:
    UserPtrMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor, quint32 buffersCount,
                          quint32 imageSize)
        : QV4L2MemoryTransfer(std::move(fileDescriptor)),
          m_imageSize(imageSize),
          m_byteArrays(buffersCount)
    {
    }

private:
    quint32 m_imageSize;
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

    static QV4L2MemoryTransferUPtr create(QV4L2FileDescriptorPtr fileDescriptor)
    {
        quint32 buffersCount = 2;
        if (!fileDescriptor->requestBuffers(V4L2_MEMORY_MMAP, buffersCount)) {
            qCWarning(qLcV4L2MemoryTransfer) << "Cannot request V4L2_MEMORY_MMAP buffers";
            return {};
        }

        std::unique_ptr<MMapMemoryTransfer> result(
                new MMapMemoryTransfer(std::move(fileDescriptor)));

        return result->init(buffersCount) ? std::move(result) : nullptr;
    }

    bool init(quint32 buffersCount)
    {
        for (quint32 index = 0; index < buffersCount; ++index) {
            auto buf = makeV4l2Buffer(V4L2_MEMORY_MMAP, index);

            if (!fileDescriptor().call(VIDIOC_QUERYBUF, &buf)) {
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
    }

    std::optional<Buffer> dequeueBuffer() override
    {
        auto v4l2Buffer = makeV4l2Buffer(V4L2_MEMORY_MMAP);
        if (!fileDescriptor().call(VIDIOC_DQBUF, &v4l2Buffer))
            return {};

        const auto index = v4l2Buffer.index;

        Q_ASSERT(index < m_spans.size());

        auto &span = m_spans[index];

        Q_ASSERT(span.inQueue);
        span.inQueue = false;

        return Buffer{ v4l2Buffer,
                       QByteArray(reinterpret_cast<const char *>(span.data), span.size) };
    }

    bool enqueueBuffer(quint32 index) override
    {
        Q_ASSERT(index < m_spans.size());
        Q_ASSERT(!m_spans[index].inQueue);

        auto buf = makeV4l2Buffer(V4L2_MEMORY_MMAP, index);
        if (!fileDescriptor().call(VIDIOC_QBUF, &buf))
            return false;

        m_spans[index].inQueue = true;
        return true;
    }

    quint32 buffersCount() const override { return static_cast<quint32>(m_spans.size()); }

private:
    using QV4L2MemoryTransfer::QV4L2MemoryTransfer;

private:
    std::vector<MemorySpan> m_spans;
};
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

QV4L2MemoryTransferUPtr makeUserPtrMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor,
                                                  quint32 imageSize)
{
    return UserPtrMemoryTransfer::create(std::move(fileDescriptor), imageSize);
}

QV4L2MemoryTransferUPtr makeMMapMemoryTransfer(QV4L2FileDescriptorPtr fileDescriptor)
{
    return MMapMemoryTransfer::create(std::move(fileDescriptor));
}

QT_END_NAMESPACE
