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

#ifndef CAMERABINFLASHCONTROL_H
#define CAMERABINFLASHCONTROL_H

#include <qcamera.h>
#include <qcameraflashcontrol.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinFlash : public QCameraFlashControl
{
    Q_OBJECT
public:
    CameraBinFlash(CameraBinSession *session);
    virtual ~CameraBinFlash();

    QCameraExposure::FlashModes flashMode() const override;
    void setFlashMode(QCameraExposure::FlashModes mode) override;
    bool isFlashModeSupported(QCameraExposure::FlashModes mode) const override;

    bool isFlashReady() const override;

private:
    CameraBinSession *m_session;
};

QT_END_NAMESPACE

#endif // CAMERABINFLASHCONTROL_H

