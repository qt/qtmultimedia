// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_stream_p.h"
#include "qlibcamera_camera_p.h"
#include "qlibcameralogging_p.h"
#if QT_CONFIG(linux_dmabuf)
#  include "qlibcamera_dmabufregistry_p.h"
#endif

#include <QtMultimedia/private/qmemoryvideobuffer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#if QT_CONFIG(linux_dmabuf)
#  include <QtMultimedia/private/qdmabufsource_p.h>
#  include <QtMultimedia/private/qdmabufvideobuffer_p.h>
#  include <QtMultimedia/private/qmaterializingvideobuffer_p.h>
#  include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#endif

#include <QtCore/qmetaobject.h>
#include <QtCore/qspan.h>
#include <QtCore/qvarlengtharray.h>
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

#if QT_CONFIG(linux_dmabuf)

// The frame wraps the request's DMA-BUF and holds the request as a capture slot.
q23::expected<QVideoFrame, QString> createZeroCopyFrame(QLibCameraDmaBufRegistry &registry,
                                                        Request *request, const FrameBuffer &buffer,
                                                        const QVideoFrameFormat &format,
                                                        int bytesPerLine)
{
    using namespace QtMultimediaPrivate;

    std::shared_ptr<QDmaBuf> dmaBuf = registry.dmaBufFor(request);
    if (!dmaBuf) {
        registry.recycleSlot(request);
        return q23::unexpected(u"Completed request does not belong to the capture registry"_s);
    }

    std::unique_ptr<QAbstractVideoBuffer> videoBuffer;
    const std::optional<DmaBufLayout> &layout = registry.layout();
    if (layout) {
        // One DmaBufPlane per layout plane, all within the same DMA-BUF.
        const auto drmFormats = dmaBufFourccFromPixelFormat(format.pixelFormat());
        QVarLengthArray<DmaBufPlane, 4> planes;
        for (qsizetype plane = 0; plane < layout->size(); ++plane) {
            const DmaBufPlaneLayout &planeLayout = (*layout)[plane];
            planes.push_back({
                    dmaBuf->fd(),
                    planeLayout.offset,
                    planeLayout.pitch,
                    drmFormats[plane],
            });
        }
        const QSpan<const DmaBufPlane> planeSpan(planes.data(), planes.size());

        using Buffer = MaterializingVideoBuffer<DmaBufSource, QLibCameraDmaBufRegistry,
                                                QDmaBufVideoBuffer>;
        videoBuffer =
                Buffer::wrapOrMaterialize(registry, request, DmaBufSource(dmaBuf, *layout), format,
                                          // QDmaBufVideoBuffer args:
                                          format, planeSpan, dmaBuf);
    } else {
        // Formats without a zero-copy layout (e.g. JPEG) hold only the bytes the driver used.
        const auto metadataPlanes = buffer.metadata().planes();
        if (metadataPlanes.empty()) {
            registry.recycleSlot(request);
            return q23::unexpected(u"Completed capture buffer has no plane metadata"_s);
        }

        const DmaBufLayout plainLayout{
            DmaBufPlaneLayout{
                    0,
                    uint32_t(bytesPerLine),
                    qsizetype(metadataPlanes[0].bytesused),
            },
        };

        using Buffer = MaterializingVideoBuffer<DmaBufSource, QLibCameraDmaBufRegistry>;
        videoBuffer = Buffer::wrapOrMaterialize(registry, request,
                                                DmaBufSource(dmaBuf, plainLayout), format);
    }

    return QVideoFramePrivate::createFrame(std::move(videoBuffer), format);
}

#endif // QT_CONFIG(linux_dmabuf)

} // namespace

void requeueRequest(Camera &camera, Request *request, QLibCameraPendingControls &pendingControls)
{
    request->reuse(Request::ReuseFlag::ReuseBuffers);
    pendingControls.mergeInto(request->controls());

    const int queueResult = camera.queueRequest(request);
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
}

QLibCameraCameraStream::QLibCameraCameraStream(
        std::shared_ptr<Camera> libcameraCamera, Stream *stream, int bytesPerLine,
        QVideoFrameFormat frameFormat, double maxFrameRate,
        std::shared_ptr<QLibCameraPendingControls> pendingControls
#if QT_CONFIG(linux_dmabuf)
        ,
        std::shared_ptr<QLibCameraDmaBufRegistry> dmaBufRegistry
#endif
        )
    : m_libcameraCamera(std::move(libcameraCamera)),
      m_stream(stream),
      m_bytesPerLine(bytesPerLine),
      m_frameFormat(std::move(frameFormat)),
      m_maxFrameRate(maxFrameRate),
      m_pendingControls(std::move(pendingControls))
#if QT_CONFIG(linux_dmabuf)
      ,
      m_dmaBufRegistry(std::move(dmaBufRegistry))
#endif
{
}

q23::expected<QVideoFrame, QString> QLibCameraCameraStream::createFrame(Request *request,
                                                                        const FrameBuffer &buffer)
{
#if QT_CONFIG(linux_dmabuf)
    if (m_dmaBufRegistry) {
        // The frame holds the request as a slot; the request is queued again once it lets go.
        return createZeroCopyFrame(*m_dmaBufRegistry, request, buffer, m_frameFormat,
                                   m_bytesPerLine);
    }
#endif

    q23::expected<QVideoFrame, QString> frame =
            createVideoFrameFromLibcameraBuffer(buffer, m_frameFormat, m_bytesPerLine);

    // we have created a deep copy, so recycle the buffer now
    requeueRequest(*m_libcameraCamera, request, *m_pendingControls);
    return frame;
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

    q23::expected<QVideoFrame, QString> frame = createFrame(request, *buffer);
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
