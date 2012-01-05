/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmediabackgroundplaybackcontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE


/*!
    \class QMediaBackgroundPlaybackControl
    \inmodule QtMultimedia
    \ingroup multimedia
    \since 5.0


    \brief The QMediaBackgroundPlaybackControl class provides access to the background playback
    related control of a QMediaService.

    If a QMediaService can play media in background, it should implement QMediaBackgroundPlaybackControl.
    This control provides a means to set the \l {setContextId()}{contextId} for application,
    \l {acquire()}{acquire the resource for playback} and \l {release()} {release the playback resource}.

    The interface name of QMediaBackgroundPlaybackControl is \c com.nokia.Qt.QMediaBackgroundPlaybackControl/1.0 as
    defined in QMediaBackgroundPlaybackControl_iid.

    \sa QMediaService::requestControl(), QMediaPlayer
*/

/*!
    \macro QMediaBackgroundPlaybackControl_iid

    \c com.nokia.Qt.QMediaBackgroundPlaybackControl/1.0

    Defines the interface name of the QMediaBackgroundPlaybackControl class.

    \relates QMediaBackgroundPlaybackControl
*/

/*!
    Destroys a media background playback control.
*/
QMediaBackgroundPlaybackControl::~QMediaBackgroundPlaybackControl()
{
}

/*!
    Constructs a new media background playback control with the given \a parent.
*/
QMediaBackgroundPlaybackControl::QMediaBackgroundPlaybackControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    \fn QMediaBackgroundPlaybackControl::setContextId(const QString& contextId)

    Sets the contextId for the application, the last contextId will be released if previously set.
    \l {acquire()}{acquire method} will be automatically invoked after setting a new contextId.

    contextId is an unique string set by the application and is used by the background daemon to
    distinguish and manage different context for different application.

    \since 1.0
*/

/*!
    \fn QMediaBackgroundPlaybackControl::acquire()

    Try to acquire the playback resource for current application
    \since 1.0
*/

/*!
    \fn QMediaBackgroundPlaybackControl::release()

    Give up the playback resource if current applicaiton holds it.
    \since 1.0
*/

/*!
    \property QMediaBackgroundPlaybackControl::isAcquired()
    \brief indicate whether the background playback resource is granted or not

    It may take sometime for the backend to actually update this value before the first use.

    By default this property is false

    \since 1.0
*/

/*!
    \fn QMediaBackgroundPlaybackControl::acquired()

    Signals that the playback resource is acquired

    \since 1.0
*/

/*!
    \fn QMediaBackgroundPlaybackControl::lost()

    Signals that the playback resource is lost

    \since 1.0
*/

#include "moc_qmediabackgroundplaybackcontrol.cpp"
QT_END_NAMESPACE


