// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDCAMERA_H
#define QANDROIDCAMERA_H

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

#include <QtMultimedia/private/qplatformcamera_p.h>

#include <QtCore/qjniobject.h>
#include <QtCore/qobject.h>

#include <QtMultimedia/qimagecapture.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>

QT_BEGIN_NAMESPACE

class QVideoFrame;
class QAndroidVideoFrameFactory;

namespace QFFmpeg {

class QAndroidCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    enum State { Closed, WaitingOpen, WaitingStart, Started };
    explicit QAndroidCamera(QCamera *camera);
    ~QAndroidCamera() override;

    bool isActive() const override { return m_state == State::Started; }
    bool isFlashModeSupported(QCamera::FlashMode mode) const override;
    bool isFlashReady() const override;
    bool isFocusModeSupported(QCamera::FocusMode focusMode) const override;
    bool isTorchModeSupported(QCamera::TorchMode mode) const override;
    void setActive(bool active) override;
    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;
    void setFlashMode(QCamera::FlashMode mode) override;
    void setFocusDistance(float distance) override;

    // FocusModeAuto maps to CameraMetadata.CONTROL_AF_MODE_CONTINUOUS_PICTURE
    //
    // The CustomFocusPoint functionality maps to CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
    // but with the CaptureRequest.CONTROL_AF_REGIONS setting.
    //
    // FocusModeManual maps to CaptureRequest.CONTROL_AF_MODE_OFF
    // but also requires us to be able to set focusDistance using
    // CaptureRequest.LENS_FOCUS_DISTANCE.
    //
    // Currently implemented focus-modes: Auto, Manual
    void setFocusMode(QCamera::FocusMode mode) override;
    void setTorchMode(QCamera::TorchMode mode) override;
    void zoomTo(float factor, float rate) override;

    std::optional<int> ffmpegHWPixelFormat() const override;

    QVideoFrameFormat frameFormat() const override;

    static bool registerNativeMethods();

    void capture();
    void updateExif(const QString &filename);
public slots:
    void onApplicationStateChanged();
    void onCameraOpened();
    void onCameraDisconnect();
    void onCameraError(int error);
    void frameAvailable(QJniObject image, bool takePhoto = false);
    void onCaptureSessionConfigured();
    void onCaptureSessionConfigureFailed();
    void onCaptureSessionFailed(int reason, long frameNumber);
    void onSessionActive();
    void onSessionClosed();
    void onStillPhotoCaptureFailed();

signals:
    void onStillPhotoCaptured(const QVideoFrame&);
    void onImageCaptureFailed(QImageCapture::Error error, const QString &errMsg);

private:
    bool isActivating() const { return m_state != State::Closed; }

    void setState(State newState);
    QtVideo::Rotation rotation() const;
    void updateCameraCharacteristics();
    void cleanCameraCharacteristics();

    State m_state = State::Closed;
    QCameraDevice m_cameraDevice;
    QJniObject m_jniCamera;

    std::unique_ptr<QFFmpeg::HWAccel> m_hwAccel;

    std::shared_ptr<QAndroidVideoFrameFactory> m_frameFactory;
    QVideoFrameFormat::PixelFormat m_androidFramePixelFormat;
    QList<QCamera::FlashMode> m_supportedFlashModes;
    // List of supported focus-modes as reported by the Android camera device. Queried once when
    // camera-device is initialized. Useful for avoiding multiple JNI calls.
    QList<QCamera::FocusMode> m_supportedFocusModes;
    bool m_TorchModeSupported = false;
    bool m_wasActive = false;

    bool m_waitingForFirstFrame = false;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QANDROIDCAMERA_H
