// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAMERA_P_H
#define QAVFCAMERA_P_H

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

#include <QtMultimedia/private/qavfcamerabase_p.h>

#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

#include <QtMultimedia/private/qplatformmediacapture_p.h>

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureSession);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceInput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoDataOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(QAVFSampleBufferDelegate);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceRotationCoordinator);

QT_BEGIN_NAMESPACE

struct VideoTransformation;

class QAVFCamera : public QAVFCameraBase
{
    Q_OBJECT

public:
    explicit QAVFCamera(QCamera *parent);
    ~QAVFCamera();

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    std::optional<int> ffmpegHWPixelFormat() const override;

    int cameraPixelFormatScore(QVideoFrameFormat::PixelFormat pixelFmt,
                               QVideoFrameFormat::ColorRange colorRange) const override;

    QVideoFrameFormat frameFormat() const override;

protected:
    void onActiveChanged(bool active) override;
    void onCameraDeviceChanged(const QCameraDevice &device) override;
    bool tryApplyCameraFormat(const QCameraFormat&) override;

private:
    void updateCameraFormat(const QCameraFormat&);
    void refreshAvCaptureSessionInputDevice();
    void setPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat, uint32_t inputCvPixFormat);
    [[nodiscard]] QSize adjustedResolution(const QCameraFormat& format) const;
    VideoTransformation surfaceTransform() const;

    void updateRotationTracking();
    void clearRotationTracking();
    int getCurrentRotationAngleDegrees() const;

    bool isFrontCamera() const;

    QMediaCaptureSession *m_qMediaCaptureSession = nullptr;
    AVCaptureSession *m_avCaptureSession = nullptr;
    AVCaptureDeviceInput *m_avCaptureDeviceVideoInput = nullptr;
    AVCaptureVideoDataOutput *m_avCaptureVideoDataOutput = nullptr;
    QAVFSampleBufferDelegate *m_qAvfSampleBufferDelegate = nullptr;
    dispatch_queue_t m_delegateQueue;
    AVPixelFormat m_hwPixelFormat = AV_PIX_FMT_NONE;
    // The current CVPixelFormat used by the AVCaptureVideoDataOutput.
    // This can in some cases be different from the AVCaptureDeviceFormat
    // used by the camera.
    uint32_t m_cvPixelFormat = 0;

    // If running iOS 17+, we use AVCaptureDeviceRotationCoordinator
    // to get the camera rotation directly from the camera-device.
    //
    // Gives us rotational information about the camera-device.
    API_AVAILABLE(macos(14.0), ios(17.0))
    AVCaptureDeviceRotationCoordinator *m_avRotationCoordinator = nullptr;
#ifdef Q_OS_IOS
    // If running iOS 16 or older, we use the UIDeviceOrientation
    // and the AVCaptureCameraPosition to apply rotation metadata
    // to the cameras frames.
    bool m_receivingUiDeviceOrientationNotifications = false;
#endif
};

QT_END_NAMESPACE


#endif // QAVFCAMERA_P_H
