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

#ifndef QCAMERAINFOCONTROL_H
#define QCAMERAINFOCONTROL_H

#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraInfoControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QCameraInfoControl();

    virtual QCamera::Position cameraPosition(const QString &deviceName) const = 0;
    virtual int cameraOrientation(const QString &deviceName) const = 0;

protected:
    explicit QCameraInfoControl(QObject *parent = nullptr);
};

#define QCameraInfoControl_iid "org.qt-project.qt.camerainfocontrol/5.3"
Q_MEDIA_DECLARE_CONTROL(QCameraInfoControl, QCameraInfoControl_iid)

QT_END_NAMESPACE

#endif // QCAMERAINFOCONTROL_H
