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
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>

#include <libcamera/controls.h>

#include <chrono>
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

class QLibCameraCameraStream final : public QObject
{
    Q_OBJECT
public:
    QLibCameraCameraStream(std::shared_ptr<libcamera::Camera>, libcamera::Stream *,
                           int bytesPerLine, QVideoFrameFormat, double maxFrameRate);

    // Runs on libcamera's CameraManager thread -- see the class comment above.
    void handleRequestCompleted(libcamera::Request *);

    template <typename Fn>
    void withPendingControls(Fn &&fn)
    {
        std::lock_guard lock(m_controlsMutex);
        fn(m_pendingControls);
    }

    libcamera::Stream *stream() const { return m_stream; }

Q_SIGNALS:
    void newVideoFrame(const QVideoFrame &);

private:
    const std::shared_ptr<libcamera::Camera> m_libcameraCamera;
    libcamera::Stream *const m_stream;
    const int m_bytesPerLine;
    const QVideoFrameFormat m_frameFormat;
    const double m_maxFrameRate;

    std::optional<std::chrono::nanoseconds> m_firstFrameTimestamp;

    QMutex m_controlsMutex;
    libcamera::ControlList m_pendingControls;
};

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_STREAM_P_H
