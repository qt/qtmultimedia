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

#ifndef QCAMERALOCKSCONTROL_H
#define QCAMERALOCKSCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraLocksControl : public QMediaControl
{
    Q_OBJECT
public:
    ~QCameraLocksControl();

    virtual QCamera::LockTypes supportedLocks() const = 0;

    virtual QCamera::LockStatus lockStatus(QCamera::LockType lock) const = 0;

    virtual void searchAndLock(QCamera::LockTypes locks) = 0;
    virtual void unlock(QCamera::LockTypes locks) = 0;

Q_SIGNALS:
    void lockStatusChanged(QCamera::LockType type, QCamera::LockStatus status, QCamera::LockChangeReason reason);

protected:
    explicit QCameraLocksControl(QObject *parent = nullptr);
};

#define QCameraLocksControl_iid "org.qt-project.qt.cameralockscontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraLocksControl, QCameraLocksControl_iid)

QT_END_NAMESPACE


#endif  // QCAMERALOCKSCONTROL_H

