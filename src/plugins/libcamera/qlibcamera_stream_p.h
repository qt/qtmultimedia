// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_STREAM_P_H
#define QLIBCAMERA_STREAM_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>

#include <libcamera/controls.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

namespace libcamera {
class Camera;
class FrameBuffer;
class Request;
class Stream;
} // namespace libcamera

QT_BEGIN_NAMESPACE

namespace QLibCamera {

#if QT_CONFIG(linux_dmabuf)
class QLibCameraDmaBufRegistry;
#endif

// Number of buffers requested from libcamera for a stream.
inline constexpr unsigned kCaptureBufferCount = 8;

// Controls set by the application. They are merged into the next request queued on the camera.
class QLibCameraPendingControls
{
public:
    template <typename Fn>
    void with(Fn &&fn)
    {
        std::lock_guard lock(m_mutex);
        fn(m_controls);
    }

    void mergeInto(libcamera::ControlList &controls)
    {
        std::lock_guard lock(m_mutex);
        controls.merge(m_controls, libcamera::ControlList::MergePolicy::OverwriteExisting);
    }

private:
    QMutex m_mutex;
    libcamera::ControlList m_controls;
};

// Re-arms a completed request, applies the pending controls and queues it on the camera again.
void requeueRequest(libcamera::Camera &camera, libcamera::Request *request,
                    QLibCameraPendingControls &pendingControls);

class QLibCameraCameraStream final : public QObject
{
    Q_OBJECT
public:
    QLibCameraCameraStream(std::shared_ptr<libcamera::Camera>, libcamera::Stream *,
                           int bytesPerLine, QVideoFrameFormat, double maxFrameRate,
                           std::shared_ptr<QLibCameraPendingControls>
#if QT_CONFIG(linux_dmabuf)
                           ,
                           std::shared_ptr<QLibCameraDmaBufRegistry>
#endif
    );

    // Runs on libcamera's CameraManager thread -- see the class comment above.
    void handleRequestCompleted(libcamera::Request *);

    libcamera::Stream *stream() const { return m_stream; }

Q_SIGNALS:
    void newVideoFrame(const QVideoFrame &);

private:
    q23::expected<QVideoFrame, QString> createFrame(libcamera::Request *,
                                                    const libcamera::FrameBuffer &);

    const std::shared_ptr<libcamera::Camera> m_libcameraCamera;
    libcamera::Stream *const m_stream;
    const int m_bytesPerLine;
    const QVideoFrameFormat m_frameFormat;
    const double m_maxFrameRate;
    const std::shared_ptr<QLibCameraPendingControls> m_pendingControls;
#if QT_CONFIG(linux_dmabuf)
    const std::shared_ptr<QLibCameraDmaBufRegistry> m_dmaBufRegistry;
#endif

    std::optional<std::chrono::nanoseconds> m_firstFrameTimestamp;
};

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_STREAM_P_H
