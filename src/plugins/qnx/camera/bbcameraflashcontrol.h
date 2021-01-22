/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBCAMERAFLASHCONTROL_H
#define BBCAMERAFLASHCONTROL_H

#include <qcameraflashcontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraFlashControl : public QCameraFlashControl
{
    Q_OBJECT
public:
    explicit BbCameraFlashControl(BbCameraSession *session, QObject *parent = 0);

    QCameraExposure::FlashModes flashMode() const override;
    void setFlashMode(QCameraExposure::FlashModes mode) override;
    bool isFlashModeSupported(QCameraExposure::FlashModes mode) const override;
    bool isFlashReady() const override;

private:
    BbCameraSession *m_session;
    QCameraExposure::FlashModes m_flashMode;
};

QT_END_NAMESPACE

#endif
