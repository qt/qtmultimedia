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

#ifndef QCAMERAZOOMCONTROL_H
#define QCAMERAZOOMCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraZoomControl : public QMediaControl
{
    Q_OBJECT

public:
    ~QCameraZoomControl();

    virtual qreal maximumOpticalZoom() const = 0;
    virtual qreal maximumDigitalZoom() const = 0;

    virtual qreal requestedOpticalZoom() const = 0;
    virtual qreal requestedDigitalZoom() const = 0;
    virtual qreal currentOpticalZoom() const = 0;
    virtual qreal currentDigitalZoom() const = 0;

    virtual void zoomTo(qreal optical, qreal digital) = 0;

Q_SIGNALS:
    void maximumOpticalZoomChanged(qreal);
    void maximumDigitalZoomChanged(qreal);

    void requestedOpticalZoomChanged(qreal opticalZoom);
    void requestedDigitalZoomChanged(qreal digitalZoom);
    void currentOpticalZoomChanged(qreal opticalZoom);
    void currentDigitalZoomChanged(qreal digitalZoom);

protected:
    explicit QCameraZoomControl(QObject *parent = nullptr);
};

#define QCameraZoomControl_iid "org.qt-project.qt.camerazoomcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraZoomControl, QCameraZoomControl_iid)

QT_END_NAMESPACE

#endif  // QCAMERAZOOMCONTROL_H
