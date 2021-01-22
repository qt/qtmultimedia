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

#ifndef QCAMERAFLASHCONTROL_H
#define QCAMERAFLASHCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcameraexposure.h>
#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraFlashControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QCameraFlashControl();

    virtual QCameraExposure::FlashModes flashMode() const = 0;
    virtual void setFlashMode(QCameraExposure::FlashModes mode) = 0;
    virtual bool isFlashModeSupported(QCameraExposure::FlashModes mode) const = 0;

    virtual bool isFlashReady() const = 0;

Q_SIGNALS:
    void flashReady(bool);

protected:
    explicit QCameraFlashControl(QObject *parent = nullptr);
};

#define QCameraFlashControl_iid "org.qt-project.qt.cameraflashcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraFlashControl, QCameraFlashControl_iid)

QT_END_NAMESPACE


#endif  // QCAMERAFLASHCONTROL_H

