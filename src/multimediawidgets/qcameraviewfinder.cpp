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

#include "qcameraviewfinder.h"
#include "qvideowidget_p.h"

#include <qcamera.h>
#include <qvideodeviceselectorcontrol.h>
#include <private/qmediaobject_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraViewfinder


    \brief The QCameraViewfinder class provides a camera viewfinder widget.

    \inmodule QtMultimediaWidgets
    \ingroup camera

    \snippet multimedia-snippets/camera.cpp Camera

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
