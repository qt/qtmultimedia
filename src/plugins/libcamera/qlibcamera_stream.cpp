// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_stream_p.h"
#include "qlibcamera_camera_p.h"
#include "qlibcameralogging_p.h"

#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

#include <QtCore/qmetaobject.h>
#include <QtCore/private/qsystemerror_p.h>

#include <libcamera/base/span.h>
#include <libcamera/camera.h>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>

#include <sys/mman.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstring>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QLibCamera {

using namespace libcamera;

namespace {

q23::expected<QVideoFrame, QString> createVideoFrameFromLibcameraBuffer(const FrameBuffer &buffer,
                                                                        QVideoFrameFormat format,
                                                                        int bytesPerLine)
{
    Span<const FrameBuffer::Plane> planes = buffer.planes();
    if (planes.empty())
        return q23::unexpected(u"Captured frame buffer has no planes"_s);

    // All libcamera planes are commonly packed into a single fd with per-plane offsets (see
    // FrameBuffer::Plane::offset); map that one fd once and copy the whole region.
    const int fd = planes[0].fd.get();
    size_t totalLength = 0;
    for (const FrameBuffer::Plane &plane : planes)
        totalLength = std::max<size_t>(totalLength, plane.offset + plane.length);

    void *mapped = ::mmap(nullptr, totalLength, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        return q23::unexpected(u"mmap fallback failed for captured frame: %1"_s.arg(
                QString::fromLocal8Bit(strerror(errno))));
    }

    QByteArray data(static_cast<const char *>(mapped), qsizetype(totalLength));
    ::munmap(mapped, totalLength);

    auto videoBuffer = std::make_unique<QMemoryVideoBuffer>(std::move(data), bytesPerLine);
    return QVideoFramePrivate::createFrame(std::move(videoBuffer), std::move(format));
}

} // namespace

QLibCameraCameraStream::QLibCameraCameraStream(std::shared_ptr<Camera> libcameraCamera,
                                               Stream *stream, int bytesPerLine,
                                               QVideoFrameFormat frameFormat, double maxFrameRate)
    : m_libcameraCamera(std::move(libcameraCamera)),
      m_stream(stream),
      m_bytesPerLine(bytesPerLine),
      m_frameFormat(std::move(frameFormat)),
      m_maxFrameRate(maxFrameRate)
{
}

void QLibCameraCameraStream::handleRequestCompleted(Request *request)
{
    // Runs on libcamera's CameraManager thread
    using namespace std::chrono;

    switch (request->status()) {
    case Request::RequestComplete:
        break;
    case Request::RequestCancelled:
        return; // e.g. during stop()
    case Request::RequestPending:
        qCWarning(lcLibCamera) << "Completed request is still pending";
        return;
    }

    FrameBuffer *buffer = request->findBuffer(m_stream);
    if (!buffer) {
        qCWarning(lcLibCamera) << "Completed request has no buffer for our stream";
        return;
    }

    const auto timestamp = nanoseconds(int64_t(buffer->metadata().timestamp));
    if (!m_firstFrameTimestamp)
        m_firstFrameTimestamp = timestamp;

    q23::expected<QVideoFrame, QString> frame =
            createVideoFrameFromLibcameraBuffer(*buffer, m_frameFormat, m_bytesPerLine);

    // we have created a deep copy, so recycle the buffer now
    request->reuse(Request::ReuseFlag::ReuseBuffers);
    withPendingControls([&](ControlList &pendingControls) {
        request->controls().merge(pendingControls, ControlList::MergePolicy::OverwriteExisting);
    });
    const int queueResult = m_libcameraCamera->queueRequest(request);
    switch (queueResult) {
    case 0:
    case -EACCES: // the camera is stopping
        break;
    case -ENODEV: // the camera has been disconnected
        qCDebug(lcLibCamera) << "Camera disconnected, not requeueing request";
        break;
    default:
        qCWarning(lcLibCamera) << "Failed to requeue request:"
                               << QSystemError::stdString(-queueResult);
        break;
    }

    if (!frame) {
        qCWarning(lcLibCamera) << "Failed to build video frame:" << frame.error();
        return;
    }

    const auto elapsed = duration_cast<microseconds>(timestamp - *m_firstFrameTimestamp);
    frame->setStartTime(elapsed.count());
    if (m_maxFrameRate > 0) {
        const auto frameDuration =
                duration_cast<microseconds>(duration<double>(1.0 / m_maxFrameRate));
        frame->setEndTime((elapsed + frameDuration).count());
    }

    QMetaObject::invokeMethod(this, [this, frame = std::move(*frame)] {
        // emit on app thread: destructor will cancel in-flight events
        Q_EMIT newVideoFrame(frame);
    }, Qt::QueuedConnection);
}

} // namespace QLibCamera

QT_END_NAMESPACE

#include "moc_qlibcamera_stream_p.cpp"
