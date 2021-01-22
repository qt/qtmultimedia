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

#ifndef QCAMERACONTROL_H
#define QCAMERACONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraControl : public QMediaControl
{
    Q_OBJECT

public:
    enum PropertyChangeType {
        CaptureMode = 1,
        ImageEncodingSettings = 2,
        VideoEncodingSettings = 3,
        Viewfinder = 4,
        ViewfinderSettings = 5
    };

    ~QCameraControl();

    virtual QCamera::State state() const = 0;
    virtual void setState(QCamera::State state) = 0;

    virtual QCamera::Status status() const = 0;

    virtual QCamera::CaptureModes captureMode() const = 0;
    virtual void setCaptureMode(QCamera::CaptureModes) = 0;
    virtual bool isCaptureModeSupported(QCamera::CaptureModes mode) const = 0;

    virtual bool canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const = 0;

Q_SIGNALS:
    void stateChanged(QCamera::State);
    void statusChanged(QCamera::Status);
    void error(int error, const QString &errorString);
    void captureModeChanged(QCamera::CaptureModes mode);

protected:
    explicit QCameraControl(QObject *parent = nullptr);
};

#define QCameraControl_iid "org.qt-project.qt.cameracontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraControl, QCameraControl_iid)

QT_END_NAMESPACE


#endif  // QCAMERACONTROL_H

