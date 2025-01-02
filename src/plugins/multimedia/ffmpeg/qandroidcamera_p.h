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

#include "qffmpeghwaccel_p.h"
#include <private/qplatformcamera_p.h>
#include <QObject>
#include <QJniObject>

QT_BEGIN_NAMESPACE

class QVideoFrame;

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
    bool isTorchModeSupported(QCamera::TorchMode mode) const override;
    void setActive(bool active) override;
    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;
    void setFlashMode(QCamera::FlashMode mode) override;
    void setTorchMode(QCamera::TorchMode mode) override;
    void zoomTo(float factor, float rate) override;

    std::optional<int> ffmpegHWPixelFormat() const override;

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

Q_SIGNALS:
    void onCaptured(const QVideoFrame&);

private:
    bool isActivating() const { return m_state != State::Closed; }

    void setState(State newState);
    QVideoFrame::RotationAngle rotation();
    void updateCameraCharacteristics();
    void cleanCameraCharacteristics();

    State m_state = State::Closed;
    QCameraDevice m_cameraDevice;
    long lastTimestamp = 0;
    QJniObject m_jniCamera;

    std::unique_ptr<QFFmpeg::HWAccel> m_hwAccel;

    QVideoFrameFormat::PixelFormat m_androidFramePixelFormat;
    QList<QCamera::FlashMode> m_supportedFlashModes;
    bool m_waitingForFirstFrame = false;
    bool m_TorchModeSupported = false;
    bool m_wasActive = false;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERA_H
