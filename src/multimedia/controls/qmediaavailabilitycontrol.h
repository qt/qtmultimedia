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

#ifndef QMEDIAAVAILABILITYCONTROL_H
#define QMEDIAAVAILABILITYCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>
#include <QtMultimedia/qmultimedia.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QMediaAvailabilityControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QMediaAvailabilityControl();

    virtual QMultimedia::AvailabilityStatus availability() const = 0;

Q_SIGNALS:
    void availabilityChanged(QMultimedia::AvailabilityStatus availability);

protected:
    explicit QMediaAvailabilityControl(QObject *parent = nullptr);
};

#define QMediaAvailabilityControl_iid "org.qt-project.qt.mediaavailabilitycontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QMediaAvailabilityControl, QMediaAvailabilityControl_iid)

QT_END_NAMESPACE

#endif // QMEDIAAVAILABILITYCONTROL_H
