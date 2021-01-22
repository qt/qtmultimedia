/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#ifndef AVFCAMERAFLASHCONTROL_H
#define AVFCAMERAFLASHCONTROL_H

#include <QtMultimedia/qcameraflashcontrol.h>
#include <QtMultimedia/qcamera.h>

#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;
class AVFCameraSession;

class AVFCameraFlashControl : public QCameraFlashControl
{
    Q_OBJECT
public:
    AVFCameraFlashControl(AVFCameraService *service);

    QCameraExposure::FlashModes flashMode() const override;
    void setFlashMode(QCameraExposure::FlashModes mode) override;
    bool isFlashModeSupported(QCameraExposure::FlashModes mode) const override;
    bool isFlashReady() const override;

private Q_SLOTS:
    void cameraStateChanged(QCamera::State newState);

private:
    bool applyFlashSettings();

    AVFCameraService *m_service;
    AVFCameraSession *m_session;

    // Set of bits:
    QCameraExposure::FlashModes m_supportedModes;
    // Only one bit set actually:
    QCameraExposure::FlashModes m_flashMode;
};

QT_END_NAMESPACE


#endif // AVFCAMERAFLASHCONTROL_H

