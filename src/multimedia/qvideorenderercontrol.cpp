/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qvideorenderercontrol.h"

#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QVideoRendererControl


    \brief The QVideoRendererControl class provides a control for rendering
    to a video surface.

    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 1.0

    Using the surface() property of QVideoRendererControl a
    QAbstractVideoSurface may be set as the video render target of a
    QMediaService.

    \snippet doc/src/snippets/multimedia-snippets/video.cpp Video renderer control

    QVideoRendererControl is one of a number of possible video output controls.

    The interface name of QVideoRendererControl is \c com.nokia.Qt.QVideoRendererControl/1.0 as
    defined in QVideoRendererControl_iid.

    \sa QMediaService::requestControl(), QVideoWidget
*/

/*!
    \macro QVideoRendererControl_iid

    \c com.nokia.Qt.QVideoRendererControl/1.0

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
    \since 1.0
*/

/*!
    \fn QVideoRendererControl::setSurface(QAbstractVideoSurface *surface)

    Sets the \a surface a video producer renders to.
    \since 1.0
*/

#include "moc_qvideorenderercontrol.cpp"
QT_END_NAMESPACE

