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

#ifndef QANDROIDCAMERAFLASHCONTROL_H
#define QANDROIDCAMERAFLASHCONTROL_H

#include <qcameraflashcontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidCameraFlashControl : public QCameraFlashControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraFlashControl(QAndroidCameraSession *session);

    QCameraExposure::FlashModes flashMode() const override;
    void setFlashMode(QCameraExposure::FlashModes mode) override;
    bool isFlashModeSupported(QCameraExposure::FlashModes mode) const override;
    bool isFlashReady() const override;

private Q_SLOTS:
    void onCameraOpened();

private:
    QAndroidCameraSession *m_session;
    QList<QCameraExposure::FlashModes> m_supportedFlashModes;
    QCameraExposure::FlashModes m_flashMode;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAFLASHCONTROL_H
