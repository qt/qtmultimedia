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

#include "qcameraviewfinder.h"
#include "qvideowidget_p.h"

#include <qcamera.h>
#include <qvideodevicecontrol.h>
#include <private/qmediaobject_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraViewfinder


    \brief The QCameraViewfinder class provides a camera viewfinder widget.

    \inmodule QtMultimedia
    \ingroup camera

    \snippet doc/src/snippets/multimedia-snippets/camera.cpp Camera

*/

class QCameraViewfinderPrivate : public QVideoWidgetPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QCameraViewfinder)
public:
    QCameraViewfinderPrivate():
        QVideoWidgetPrivate()
    {
    }
};

/*!
    Constructs a new camera viewfinder widget.

    The \a parent is passed to QVideoWidget.
*/

QCameraViewfinder::QCameraViewfinder(QWidget *parent)
    :QVideoWidget(*new QCameraViewfinderPrivate, parent)
{
}

/*!
    Destroys a camera viewfinder widget.
*/
QCameraViewfinder::~QCameraViewfinder()
{
}

/*!
  \reimp
*/
QMediaObject *QCameraViewfinder::mediaObject() const
{
    return QVideoWidget::mediaObject();
}

/*!
  \reimp
*/
bool QCameraViewfinder::setMediaObject(QMediaObject *object)
{
    return QVideoWidget::setMediaObject(object);
}

QT_END_NAMESPACE

#include "moc_qcameraviewfinder.cpp"
