/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/


#ifndef QANDROIDCAMERACONTROL_H
#define QANDROIDCAMERACONTROL_H

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

#include <private/qplatformcamera_p.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;
class QAndroidCameraVideoRendererControl;
class QAndroidMediaCaptureSession;

class QAndroidCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    explicit QAndroidCamera(QCamera *camera);
    virtual ~QAndroidCamera();

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session) override;

    void setFocusMode(QCamera::FocusMode mode) override;
    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

    void zoomTo(float factor, float rate) override;

    void setFlashMode(QCamera::FlashMode mode) override;
    bool isFlashModeSupported(QCamera::FlashMode mode) const override;
    bool isFlashReady() const override;

    void setTorchMode(QCamera::TorchMode mode) override;
    bool isTorchModeSupported(QCamera::TorchMode mode) const override;

    void setExposureMode(QCamera::ExposureMode mode) override;
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;

    void setExposureCompensation(float bias) override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode mode) override;

private Q_SLOTS:
    void onCameraOpened();
    void setCameraFocusArea();

private:
    void reactivateCameraSession();

    QAndroidCameraSession *m_cameraSession = nullptr;
    QAndroidMediaCaptureSession *m_service = nullptr;

    QList<QCamera::FocusMode> m_supportedFocusModes;
    bool m_continuousPictureFocusSupported = false;
    bool m_continuousVideoFocusSupported = false;
    bool m_focusPointSupported = false;

    QList<int> m_zoomRatios;

    QList<QCamera::ExposureMode> m_supportedExposureModes;
    int m_minExposureCompensationIndex;
    int m_maxExposureCompensationIndex;
    qreal m_exposureCompensationStep;

    bool isFlashSupported = false;
    bool isFlashAutoSupported = false;
    bool isTorchSupported = false;
    bool isPendingSetActive = false;
    QCameraDevice m_cameraDev;

    QMap<QCamera::WhiteBalanceMode, QString> m_supportedWhiteBalanceModes;
    QCameraFormat m_cameraFormat;
};


QT_END_NAMESPACE

#endif // QANDROIDCAMERACONTROL_H
