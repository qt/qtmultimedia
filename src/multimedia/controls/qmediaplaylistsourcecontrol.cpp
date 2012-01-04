/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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


#include "qmediaplaylistsourcecontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaPlaylistSourceControl
    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 1.0


    \brief The QMediaPlaylistSourceControl class provides access to the playlist playback
    functionality of a QMediaService.

    This control allows QMediaPlaylist to be passed directly to the service
    instead of playing media sources one by one.  This control should be
    implemented if backend benefits from knowing the next media source to be
    played, for example for preloading, cross fading or gap-less playback.

    If QMediaPlaylistSourceControl is provided, the backend must listen for
    current playlist item changes to load corresponding media source and
    advance the playlist  with QMediaPlaylist::next() when playback of the
    current media is finished.

    The interface name of QMediaPlaylistSourceControl is \c com.nokia.Qt.QMediaPlaylistSourceControl/1.0 as
    defined in QMediaPlaylistSourceControl_iid.

    \sa QMediaService::requestControl(), QMediaPlayer
*/

/*!
    \macro QMediaPlaylistSourceControl_iid

    \c com.nokia.Qt.QMediaPlaylistSourceControl/1.0

    Defines the interface name of the QMediaPlaylistSourceControl class.

    \relates QMediaPlaylistSourceControl
*/

/*!
  Create a new playlist source control object with the given \a parent.
*/
QMediaPlaylistSourceControl::QMediaPlaylistSourceControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
  Destroys the playlist control.
*/
QMediaPlaylistSourceControl::~QMediaPlaylistSourceControl()
{
}


/*!
  \fn QMediaPlaylistSourceControl::playlist() const

  Returns the current playlist.
  Should return a null pointer if no playlist is assigned.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistSourceControl::setPlaylist(QMediaPlaylist *playlist)

  Set the playlist of this media player to \a playlist.
  If a null pointer is passed, the playlist source should be disabled.

  The current media should be replaced with the current item of the media playlist.
  \since 1.0
*/


/*!
  \fn QMediaPlaylistSourceControl::playlistChanged(QMediaPlaylist* playlist)

  Signal emitted when the playlist has changed to \a playlist.
  \since 1.0
*/

#include "moc_qmediaplaylistsourcecontrol.cpp"
QT_END_NAMESPACE

