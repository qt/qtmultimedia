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

#include "qvideorenderercontrol.h"

#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QVideoRendererControl
    \obsolete

    \inmodule QtMultimedia
    \brief The QVideoRendererControl class provides a media control for rendering video to a QAbstractVideoSurface.


    \ingroup multimedia_control

    Using the surface() property of QVideoRendererControl a
    QAbstractVideoSurface may be set as the video render target of a
    QMediaService.

    \snippet multimedia-snippets/video.cpp Video renderer control

    QVideoRendererControl is one of a number of possible video output controls.

    The interface name of QVideoRendererControl is \c org.qt-project.qt.videorenderercontrol/5.0 as
    defined in QVideoRendererControl_iid.

    \sa QMediaService::requestControl(), QVideoWidget
*/

/*!
    \macro QVideoRendererControl_iid

    \c org.qt-project.qt.videorenderercontrol/5.0

    Defines the interface name of the QVideoRendererControl class.

    \relates QVideoRendererControl
*/

/*!
    Constructs a new video renderer media end point with the given \a parent.
*/
QVideoRendererControl::QVideoRendererControl(QObject *parent)
    : QMediaControl(parent)
{
}

/*!
    Destroys a video renderer media end point.
*/
QVideoRendererControl::~QVideoRendererControl()
{
}

/*!
    \fn QVideoRendererControl::surface() const

    Returns the surface a video producer renders to.
*/

/*!
    \fn QVideoRendererControl::setSurface(QAbstractVideoSurface *surface)

    Sets the \a surface a video producer renders to.
*/

QT_END_NAMESPACE

#include "moc_qvideorenderercontrol.cpp"
