// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QLIBCAMERA_CAMERA_P_H
#define QLIBCAMERA_CAMERA_P_H

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

#include "qlibcamera_stream_p.h" // for the full QLibCameraCameraStream definition

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtCore/private/qexpected_p.h>

#include <memory>
#include <tuple>
#include <utility>
#include <vector>

namespace libcamera {
class Camera;
class CameraConfiguration;
class FrameBufferAllocator;
class Request;
} // namespace libcamera

QT_BEGIN_NAMESPACE

namespace QLibCamera {

class QLibCameraInstance;

class QLibCameraCamera final : public QPlatformCamera
{
    Q_OBJECT
public:
    explicit QLibCameraCamera(QCamera *parent, std::shared_ptr<QLibCameraInstance>);
    ~QLibCameraCamera() override;

    bool isActive() const override { return m_active; }
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &) override;
    bool setCameraFormat(const QCameraFormat &) override;

    void setExposureMode(QCamera::ExposureMode) override;
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;
    void setExposureCompensation(float compensation) override;
    void setManualIsoSensitivity(int iso) override;
    void setManualExposureTime(float seconds) override;

    void setWhiteBalanceMode(QCamera::WhiteBalanceMode) override;
    void setColorTemperature(int temperature) override;

    void zoomTo(float newZoomFactor, float rate = -1.) override;

    QVideoFrameFormat frameFormat() const override;

private:
    int cameraPixelFormatScore(QVideoFrameFormat::PixelFormat,
                               QVideoFrameFormat::ColorRange) const override;

    using StartCapturingResult = q23::expected<void, std::tuple<QCamera::Error, QString>>;

    StartCapturingResult startCapturing();
    void startCapturingReportingErrors();
    void stopCapturing();

    template <typename Fn>
    void withPendingControls(Fn &&fn)
    {
        if (m_stream)
            m_stream->withPendingControls(std::forward<Fn>(fn));
    }

    // Lifecycle of the libcamera stream
    enum class StreamState : uint8_t {
        Idle,
        Acquired,
        Running,
    };

    std::shared_ptr<QLibCameraInstance> m_instance;
    std::shared_ptr<libcamera::Camera> m_camera;
    std::unique_ptr<QLibCameraCameraStream> m_stream;

    QCameraDevice m_cameraDevice;
    bool m_active = false;
    StreamState m_streamState = StreamState::Idle;

    std::unique_ptr<libcamera::CameraConfiguration> m_config;
    std::unique_ptr<libcamera::FrameBufferAllocator> m_allocator;
    std::vector<std::unique_ptr<libcamera::Request>> m_requests;

    int m_stride = 0;
    QVideoFrameFormat::ColorSpace m_colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    QVideoFrameFormat::ColorRange m_colorRange = QVideoFrameFormat::ColorRange_Unknown;
};

} // namespace QLibCamera

QT_END_NAMESPACE

#endif // QLIBCAMERA_CAMERA_P_H
