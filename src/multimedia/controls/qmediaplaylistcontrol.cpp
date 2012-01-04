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


#include "qmediaplaylistcontrol.h"
#include "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QMediaPlaylistControl
    \inmodule QtMultimedia
    \ingroup multimedia-serv
    \since 1.0


    \brief The QMediaPlaylistControl class provides access to the playlist
    functionality of a QMediaService.

    If a QMediaService contains an internal playlist it will implement
    QMediaPlaylistControl.  This control provides access to the contents of the
    \l {playlistProvider()}{playlist}, as well as the \l
    {currentIndex()}{position} of the current media, and a means of navigating
    to the \l {next()}{next} and \l {previous()}{previous} media.

    The functionality provided by the control is exposed to application code
    through the QMediaPlaylist class.

    The interface name of QMediaPlaylistControl is \c com.nokia.Qt.QMediaPlaylistControl/1.0 as
    defined in QMediaPlaylistControl_iid.

    \sa QMediaService::requestControl(), QMediaPlayer
*/

/*!
    \macro QMediaPlaylistControl_iid

    \c com.nokia.Qt.QMediaPlaylistControl/1.0

    Defines the interface name of the QMediaPlaylistControl class.

    \relates QMediaPlaylistControl
*/

/*!
  Create a new playlist control object with the given \a parent.
*/
QMediaPlaylistControl::QMediaPlaylistControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
  Destroys the playlist control.
*/
QMediaPlaylistControl::~QMediaPlaylistControl()
{
}


/*!
  \fn QMediaPlaylistControl::playlistProvider() const

  Returns the playlist used by this media player.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::setPlaylistProvider(QMediaPlaylistProvider *playlist)

  Set the playlist of this media player to \a playlist.

  In many cases it is possible just to use the playlist
  constructed by player, but sometimes replacing the whole
  playlist allows to avoid copyting of all the items bettween playlists.

  Returns true if player can use this passed playlist; otherwise returns false.

  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::currentIndex() const

  Returns position of the current media source in the playlist.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::setCurrentIndex(int position)

  Jump to the item at the given \a position.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::nextIndex(int step) const

  Returns the index of item, which were current after calling next()
  \a step times.

  Returned value depends on the size of playlist, current position
  and playback mode.

  \since 1.0
  \sa QMediaPlaylist::playbackMode
*/

/*!
  \fn QMediaPlaylistControl::previousIndex(int step) const

  Returns the index of item, which were current after calling previous()
  \a step times.

  \since 1.0
  \sa QMediaPlaylist::playbackMode
*/

/*!
  \fn QMediaPlaylistControl::next()

  Moves to the next item in playlist.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::previous()

  Returns to the previous item in playlist.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::playbackMode() const

  Returns the playlist navigation mode.

  \since 1.0
  \sa QMediaPlaylist::PlaybackMode
*/

/*!
  \fn QMediaPlaylistControl::setPlaybackMode(QMediaPlaylist::PlaybackMode mode)

  Sets the playback \a mode.

  \since 1.0
  \sa QMediaPlaylist::PlaybackMode
*/

/*!
  \fn QMediaPlaylistControl::playlistProviderChanged()

  Signal emitted when the playlist provider has changed.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::currentIndexChanged(int position)

  Signal emitted when the playlist \a position is changed.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::playbackModeChanged(QMediaPlaylist::PlaybackMode mode)

  Signal emitted when the playback \a mode is changed.
  \since 1.0
*/

/*!
  \fn QMediaPlaylistControl::currentMediaChanged(const QMediaContent& content)

  Signal emitted when current media changes to \a content.
  \since 1.0
*/

#include "moc_qmediaplaylistcontrol.cpp"
QT_END_NAMESPACE

