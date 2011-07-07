
/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "DebugMacros.h"

#include "s60mediaplayercontrol.h"
#include "s60mediaplayersession.h"

#include <QtCore/qdir.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>

/*!
    Constructs a new media player control with the given \a parent.
*/

S60MediaPlayerControl::S60MediaPlayerControl(MS60MediaPlayerResolver& mediaPlayerResolver, QObject *parent)
    : QMediaPlayerControl(parent),
      m_mediaPlayerResolver(mediaPlayerResolver),
      m_session(NULL),
      m_stream(NULL)
{
    DP0("S60MediaPlayerControl::S60MediaPlayerControl +++");

    DP0("S60MediaPlayerControl::S60MediaPlayerControl ---");

}

/*!
    Destroys a media player control.
*/

S60MediaPlayerControl::~S60MediaPlayerControl()
{
    DP0("S60MediaPlayerControl::~S60MediaPlayerControl +++");
    DP0("S60MediaPlayerControl::~S60MediaPlayerControl ---");
}

/*!
    \return the current playback position in milliseconds.
*/

qint64 S60MediaPlayerControl::position() const
{
   // DP0("S60MediaPlayerControl::position");

    if (m_session)
        return m_session->position();
    return 0;
}

/*!
   \return the duration of the current media in milliseconds.
*/

qint64 S60MediaPlayerControl::duration() const
{
   // DP0("S60MediaPlayerControl::duration");

    if (m_session)
        return m_session->duration();
    return -1;
}

/*!
    \return the state of a player control.
*/

QMediaPlayer::State S60MediaPlayerControl::state() const
{
    DP0("S60MediaPlayerControl::state");

    if (m_session)
        return m_session->state();
    return QMediaPlayer::StoppedState;
}

/*!
    \return the status of the current media.
*/

QMediaPlayer::MediaStatus S60MediaPlayerControl::mediaStatus() const
{
    DP0("QMediaPlayer::mediaStatus");

    if (m_session)
        return m_session->mediaStatus();
    return m_mediaSettings.mediaStatus();
}

/*!
    \return the buffering progress of the current media.  Progress is measured in the percentage
    of the buffer filled.
*/

int S60MediaPlayerControl::bufferStatus() const
{
   // DP0("S60MediaPlayerControl::bufferStatus");

    if (m_session)
        return m_session->bufferStatus();
    return 0;
}

/*!
    \return the audio volume of a player control.
*/

int S60MediaPlayerControl::volume() const
{
    DP0("S60MediaPlayerControl::volume");

    if (m_session)
        return m_session->volume();
    return m_mediaSettings.volume();
}

/*!
    \return the mute state of a player control.
*/

bool S60MediaPlayerControl::isMuted() const
{
    DP0("S60MediaPlayerControl::isMuted");

   if (m_session)
       return  m_session->isMuted();
   return m_mediaSettings.isMuted();
}

/*!
    Identifies if the current media is seekable.

    \return true if it possible to seek within the current media, and false otherwise.
*/

bool S60MediaPlayerControl::isSeekable() const
{
    DP0("S60MediaPlayerControl::isSeekable");

    if (m_session)
       return  m_session->isSeekable();
    return false;
}

/*!
    \return a range of times in milliseconds that can be played back.

    Usually for local files this is a continuous interval equal to [0..duration()]
    or an empty time range if seeking is not supported, but for network sources
    it refers to the buffered parts of the media.
*/

QMediaTimeRange S60MediaPlayerControl::availablePlaybackRanges() const
{
    DP0("S60MediaPlayerControl::availablePlaybackRanges");

    QMediaTimeRange ranges;

    if(m_session && m_session->isSeekable())
        ranges.addInterval(0, m_session->duration());

    return ranges;
}

/*!
    \return the rate of playback.
*/

qreal S60MediaPlayerControl::playbackRate() const
{
    DP0("S60MediaPlayerControl::playbackRate");

    return m_mediaSettings.playbackRate();
}

/*!
    Sets the \a rate of playback.
*/

void S60MediaPlayerControl::setPlaybackRate(qreal rate)
{
    DP0("S60MediaPlayerControl::setPlaybackRate +++");

    DP1("S60MediaPlayerControl::setPlaybackRate - ", rate);

    //getting the current playbackrate
    qreal currentPBrate = m_mediaSettings.playbackRate();
    //checking if we need to change the Playback rate
    if (!qFuzzyCompare(currentPBrate,rate))
    {
        if(m_session)
            m_session->setPlaybackRate(rate);

        m_mediaSettings.setPlaybackRate(rate);
    }

    DP0("S60MediaPlayerControl::setPlaybackRate ---");
}

/*!
    Sets the playback \a pos of the current media.  This will initiate a seek and it may take
    some time for playback to reach the position set.
*/

void S60MediaPlayerControl::setPosition(qint64 pos)
{
    DP0("S60MediaPlayerControl::setPosition +++");

    DP1("S60MediaPlayerControl::setPosition, Position:", pos);

    if (m_session)
        m_session->setPosition(pos);

    DP0("S60MediaPlayerControl::setPosition ---");
}

/*!
    Starts playback of the current media.

    If successful the player control will immediately enter the \l {QMediaPlayer::PlayingState}
    {playing} state.
*/

void S60MediaPlayerControl::play()
{
    DP0("S60MediaPlayerControl::play +++");

    if (m_session)
        m_session->play();

    DP0("S60MediaPlayerControl::play ---");
}

/*!
    Pauses playback of the current media.

    If sucessful the player control will immediately enter the \l {QMediaPlayer::PausedState}
    {paused} state.
*/

void S60MediaPlayerControl::pause()
{
    DP0("S60MediaPlayerControl::pause +++");

    if (m_session)
        m_session->pause();

    DP0("S60MediaPlayerControl::pause ---");
}

/*!
    Stops playback of the current media.

    If successful the player control will immediately enter the \l {QMediaPlayer::StoppedState}
    {stopped} state.
*/

void S60MediaPlayerControl::stop()
{
    DP0("S60MediaPlayerControl::stop +++");

    if (m_session)
        m_session->stop();

    DP0("S60MediaPlayerControl::stop ---");
}

/*!
    Sets the audio \a volume of a player control.
*/

void S60MediaPlayerControl::setVolume(int volume)
{
    DP0("S60MediaPlayerControl::setVolume +++");

    DP1("S60MediaPlayerControl::setVolume", volume);

    int boundVolume = qBound(0, volume, 100);
    if (boundVolume == m_mediaSettings.volume())
        return;

    m_mediaSettings.setVolume(boundVolume);

    if (m_session)
        m_session->setVolume(boundVolume);

    DP0("S60MediaPlayerControl::setVolume ---");
}

/*!
    Sets the \a muted state of a player control.
*/

void S60MediaPlayerControl::setMuted(bool muted)
{
    DP0("S60MediaPlayerControl::setMuted +++");

    DP1("S60MediaPlayerControl::setMuted - ", muted);

    if (m_mediaSettings.isMuted() == muted)
        return;

    m_mediaSettings.setMuted(muted);

    if (m_session)
        m_session->setMuted(muted);

    DP0("S60MediaPlayerControl::setMuted ---");
}

/*!
 * \return the current media source.
*/

QMediaContent S60MediaPlayerControl::media() const
{
    DP0("S60MediaPlayerControl::media");

    return m_currentResource;
}

/*!
    \return the current media stream. This is only a valid if a stream was passed to setMedia().

    \sa setMedia()
*/

const QIODevice *S60MediaPlayerControl::mediaStream() const
{
    DP0("S60MediaPlayerControl::mediaStream");

    return m_stream;
}

/*!
     Sets the current \a source media source.  If a \a stream is supplied; data will be read from that
    instead of attempting to resolve the media source.  The media source may still be used to
    supply media information such as mime type.

    Setting the media to a null QMediaContent will cause the control to discard all
    information relating to the current media source and to cease all I/O operations related
    to that media.
*/

void S60MediaPlayerControl::setMedia(const QMediaContent &source, QIODevice *stream)
{
    DP0("S60MediaPlayerControl::setMedia +++");

    Q_UNUSED(stream)

    if ((m_session && m_currentResource == source) && m_session->isStreaming())
        {
            m_session->load(source);
            return;
        }

    // we don't want to set & load media again when it is already loaded
    if (m_session && m_currentResource == source)
        return;

    // store to variable as session is created based on the content type.
    m_currentResource = source;
    S60MediaPlayerSession *newSession = m_mediaPlayerResolver.PlayerSession();
    m_mediaSettings.setMediaStatus(QMediaPlayer::UnknownMediaStatus);

    if (m_session)
        m_session->reset();
    else {
        emit mediaStatusChanged(QMediaPlayer::UnknownMediaStatus);
        emit error(QMediaPlayer::NoError, QString());
    }

    m_session = newSession;

    if (m_session)
        m_session->load(source);
    else {
        QMediaPlayer::MediaStatus status = (source.isNull()) ? QMediaPlayer::NoMedia : QMediaPlayer::InvalidMedia;
        m_mediaSettings.setMediaStatus(status);
        emit stateChanged(QMediaPlayer::StoppedState);
        emit error((source.isNull()) ? QMediaPlayer::NoError : QMediaPlayer::ResourceError, 
                   (source.isNull()) ? "" : tr("Media couldn't be resolved"));
        emit mediaStatusChanged(status);
    }
    emit mediaChanged(m_currentResource);

    DP0("S60MediaPlayerControl::setMedia ---");
}

/*!
 * \return media player session.
*/
S60MediaPlayerSession* S60MediaPlayerControl::session()
{
    DP0("S60MediaPlayerControl::session");

    return m_session;
}

/*!
 *   Sets \a output as a VideoOutput.
*/

void S60MediaPlayerControl::setVideoOutput(QObject *output)
{
    DP0("S60MediaPlayerControl::setVideoOutput +++");

   m_mediaPlayerResolver.VideoPlayerSession()->setVideoRenderer(output);

   DP0("S60MediaPlayerControl::setVideoOutput ---");
}

/*!
 * \return TRUE if Audio available or else FALSE.
*/

bool S60MediaPlayerControl::isAudioAvailable() const
{
    DP0("S60MediaPlayerControl::isAudioAvailable");

    if (m_session)
        return m_session->isAudioAvailable(); 
    return false;
}

/*!
 * \return TRUE if Video available or else FALSE.
*/

bool S60MediaPlayerControl::isVideoAvailable() const
{
    DP0("S60MediaPlayerControl::isVideoAvailable");

    if (m_session)
        return m_session->isVideoAvailable();
    return false;
}

/*!
 * \return media settings.
 *
 * Media Settings include volume, muted, playbackRate, mediaStatus, audioEndpoint.
*/
const S60MediaSettings& S60MediaPlayerControl::mediaControlSettings() const
{
    DP0("S60MediaPlayerControl::mediaControlSettings");
    return m_mediaSettings;
}

/*!
 * Set the audio endpoint to \a name.
*/

void S60MediaPlayerControl::setAudioEndpoint(const QString& name)
{
    DP0("S60MediaPlayerControl::setAudioEndpoint +++");

    DP1("S60MediaPlayerControl::setAudioEndpoint - ", name);

    m_mediaSettings.setAudioEndpoint(name);

    DP0("S60MediaPlayerControl::setAudioEndpoint ---");
}

/*!
 *  Sets media type \a type as Unknown, Video, Audio, Data.
*/

void S60MediaPlayerControl::setMediaType(S60MediaSettings::TMediaType type)
{
    DP0("S60MediaPlayerControl::setMediaType +++");

    DP1("S60MediaPlayerControl::setMediaType - ", type);

    m_mediaSettings.setMediaType(type);

    DP0("S60MediaPlayerControl::setMediaType ---");
}
