/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCAMERAZOOMCONTROL_H
#define QCAMERAZOOMCONTROL_H

#include <qmediacontrol.h>
#include <qmediaobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Multimedia)


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
    QCameraZoomControl(QObject* parent = 0);
};

#define QCameraZoomControl_iid "com.nokia.Qt.QCameraZoomControl/1.0"
Q_MEDIA_DECLARE_CONTROL(QCameraZoomControl, QCameraZoomControl_iid)

QT_END_NAMESPACE

QT_END_HEADER

#endif  // QCAMERAZOOMCONTROL_H
