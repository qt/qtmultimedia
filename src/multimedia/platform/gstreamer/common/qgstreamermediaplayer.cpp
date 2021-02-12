/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qgstreamermediaplayer_p.h>
#include <private/qgstreamerplayersession_p.h>
#include <private/qgstreamerstreamscontrol_p.h>
#include <qaudiodeviceinfo.h>

#include <QtCore/qdir.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#define DEBUG_PLAYBIN

QT_BEGIN_NAMESPACE

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QObject *parent)
    : QPlatformMediaPlayer(parent)
{
    m_session = new QGstreamerPlayerSession(this);
    connect(m_session, &QGstreamerPlayerSession::positionChanged, this, &QGstreamerMediaPlayer::positionChanged);
    connect(m_session, &QGstreamerPlayerSession::durationChanged, this, &QGstreamerMediaPlayer::durationChanged);
    connect(m_session, &QGstreamerPlayerSession::mutedStateChanged, this, &QGstreamerMediaPlayer::mutedChanged);
    connect(m_session, &QGstreamerPlayerSession::volumeChanged, this, &QGstreamerMediaPlayer::volumeChanged);
    connect(m_session, &QGstreamerPlayerSession::stateChanged, this, &QGstreamerMediaPlayer::updateSessionState);
    connect(m_session, &QGstreamerPlayerSession::bufferingProgressChanged, this, &QGstreamerMediaPlayer::setBufferProgress);
    connect(m_session, &QGstreamerPlayerSession::playbackFinished, this, &QGstreamerMediaPlayer::processEOS);
    connect(m_session, &QGstreamerPlayerSession::audioAvailableChanged, this, &QGstreamerMediaPlayer::audioAvailableChanged);
    connect(m_session, &QGstreamerPlayerSession::videoAvailableChanged, this, &QGstreamerMediaPlayer::videoAvailableChanged);
    connect(m_session, &QGstreamerPlayerSession::seekableChanged, this, &QGstreamerMediaPlayer::seekableChanged);
    connect(m_session, &QGstreamerPlayerSession::error, this, &QGstreamerMediaPlayer::error);
    connect(m_session, &QGstreamerPlayerSession::invalidMedia, this, &QGstreamerMediaPlayer::handleInvalidMedia);
    connect(m_session, &QGstreamerPlayerSession::playbackRateChanged, this, &QGstreamerMediaPlayer::playbackRateChanged);
    connect(m_session, &QGstreamerPlayerSession::metaDataChanged, this, &QGstreamerMediaPlayer::metaDataChanged);
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
}

qint64 QGstreamerMediaPlayer::position() const
{
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        return m_session->duration();

    return m_pendingSeekPosition != -1 ? m_pendingSeekPosition : m_session->position();
}

qint64 QGstreamerMediaPlayer::duration() const
{
    return m_session->duration();
}

QMediaPlayer::State QGstreamerMediaPlayer::state() const
{
    return m_currentState;
}

QMediaPlayer::MediaStatus QGstreamerMediaPlayer::mediaStatus() const
{
    return m_mediaStatus;
}

int QGstreamerMediaPlayer::bufferStatus() const
{
    if (m_bufferProgress == -1)
        return m_session->state() == QMediaPlayer::StoppedState ? 0 : 100;

    return m_bufferProgress;
}

int QGstreamerMediaPlayer::volume() const
{
    return m_session->volume();
}

bool QGstreamerMediaPlayer::isMuted() const
{
    return m_session->isMuted();
}

bool QGstreamerMediaPlayer::isSeekable() const
{
    return m_session->isSeekable();
}

QMediaTimeRange QGstreamerMediaPlayer::availablePlaybackRanges() const
{
    return m_session->availablePlaybackRanges();
}

qreal QGstreamerMediaPlayer::playbackRate() const
{
    return m_session->playbackRate();
}

void QGstreamerMediaPlayer::setPlaybackRate(qreal rate)
{
    m_session->setPlaybackRate(rate);
}

void QGstreamerMediaPlayer::setPosition(qint64 pos)
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO << pos/1000.0;
#endif

    pushState();

    if (m_mediaStatus == QMediaPlayer::EndOfMedia) {
        m_mediaStatus = QMediaPlayer::LoadedMedia;
    }

    if (m_currentState == QMediaPlayer::StoppedState) {
        m_pendingSeekPosition = pos;
        emit positionChanged(m_pendingSeekPosition);
    } else if (m_session->isSeekable()) {
        m_session->showPrerollFrames(true);
        m_session->seek(pos);
        m_pendingSeekPosition = -1;
    } else if (m_session->state() == QMediaPlayer::StoppedState) {
        m_pendingSeekPosition = pos;
        emit positionChanged(m_pendingSeekPosition);
    } else if (m_pendingSeekPosition != -1) {
        m_pendingSeekPosition = -1;
        emit positionChanged(m_pendingSeekPosition);
    }

    popAndNotifyState();
}

void QGstreamerMediaPlayer::play()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif
    //m_userRequestedState is needed to know that we need to resume playback when resource-policy
    //regranted the resources after lost, since m_currentState will become paused when resources are
    //lost.
    m_userRequestedState = QMediaPlayer::PlayingState;
    playOrPause(QMediaPlayer::PlayingState);
}

void QGstreamerMediaPlayer::pause()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif
    m_userRequestedState = QMediaPlayer::PausedState;
    // If the playback has not been started yet but pause is requested.
    // Seek to the beginning to show first frame.
    if (m_pendingSeekPosition == -1 && m_session->position() == 0)
        m_pendingSeekPosition = 0;

    playOrPause(QMediaPlayer::PausedState);
}

void QGstreamerMediaPlayer::playOrPause(QMediaPlayer::State newState)
{
    if (m_mediaStatus == QMediaPlayer::NoMedia)
        return;

    pushState();

    if (m_setMediaPending) {
        m_mediaStatus = QMediaPlayer::LoadingMedia;
        setMedia(m_currentResource, m_stream);
    }

    if (m_mediaStatus == QMediaPlayer::EndOfMedia && m_pendingSeekPosition == -1) {
        m_pendingSeekPosition = 0;
    }

    // show prerolled frame if switching from stopped state
    if (m_pendingSeekPosition == -1) {
        m_session->showPrerollFrames(true);
    } else if (m_session->state() == QMediaPlayer::StoppedState) {
        // Don't evaluate the next two conditions.
    } else if (m_session->isSeekable()) {
        m_session->pause();
        m_session->showPrerollFrames(true);
        m_session->seek(m_pendingSeekPosition);
        m_pendingSeekPosition = -1;
    } else {
        m_pendingSeekPosition = -1;
    }

    bool ok = false;

    //To prevent displaying the first video frame when playback is resumed
    //the pipeline is paused instead of playing, seeked to requested position,
    //and after seeking is finished (position updated) playback is restarted
    //with show-preroll-frame enabled.
    if (newState == QMediaPlayer::PlayingState && m_pendingSeekPosition == -1)
        ok = m_session->play();
    else
        ok = m_session->pause();

    if (!ok)
        newState = QMediaPlayer::StoppedState;

    if (m_mediaStatus == QMediaPlayer::InvalidMedia)
        m_mediaStatus = QMediaPlayer::LoadingMedia;

    m_currentState = newState;

    if (m_mediaStatus == QMediaPlayer::EndOfMedia || m_mediaStatus == QMediaPlayer::LoadedMedia) {
        if (m_bufferProgress == -1 || m_bufferProgress == 100)
            m_mediaStatus = QMediaPlayer::BufferedMedia;
        else
            m_mediaStatus = QMediaPlayer::BufferingMedia;
    }

    popAndNotifyState();

    emit positionChanged(position());
}

void QGstreamerMediaPlayer::stop()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif
    m_userRequestedState = QMediaPlayer::StoppedState;

    pushState();

    if (m_currentState != QMediaPlayer::StoppedState) {
        m_currentState = QMediaPlayer::StoppedState;
        m_session->showPrerollFrames(false); // stop showing prerolled frames in stop state
        // Since gst is not going to send GST_STATE_PAUSED
        // when pipeline is already paused,
        // needs to update media status directly.
        if (m_session->state() == QMediaPlayer::PausedState)
            updateMediaStatus();
        else
            m_session->pause();

        if (m_mediaStatus != QMediaPlayer::EndOfMedia) {
            m_pendingSeekPosition = 0;
            emit positionChanged(position());
        }
    }

    popAndNotifyState();
}

void QGstreamerMediaPlayer::setVolume(int volume)
{
    m_session->setVolume(volume);
}

void QGstreamerMediaPlayer::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

QUrl QGstreamerMediaPlayer::media() const
{
    return m_currentResource;
}

const QIODevice *QGstreamerMediaPlayer::mediaStream() const
{
    return m_stream;
}

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif

    pushState();

    m_currentState = QMediaPlayer::StoppedState;
    QUrl oldMedia = m_currentResource;
    m_pendingSeekPosition = -1;
    m_session->showPrerollFrames(false); // do not show prerolled frames until pause() or play() explicitly called
    m_setMediaPending = false;

    m_session->stop();

    bool userStreamValid = false;

    if (m_bufferProgress != -1) {
        m_bufferProgress = -1;
        emit bufferStatusChanged(0);
    }

    m_currentResource = content;
    m_stream = stream;

    QNetworkRequest request(content);

    if (m_stream)
        userStreamValid = stream->isOpen() && m_stream->isReadable();

#if !QT_CONFIG(gstreamer_app)
    m_session->loadFromUri(request);
#else
    if (m_stream) {
        if (userStreamValid){
            m_session->loadFromStream(request, m_stream);
        } else {
            m_mediaStatus = QMediaPlayer::InvalidMedia;
            emit error(QMediaPlayer::FormatError, tr("Attempting to play invalid user stream"));
            popAndNotifyState();
            return;
        }
    } else
        m_session->loadFromUri(request);
#endif

#if QT_CONFIG(gstreamer_app)
    if (!request.url().isEmpty() || userStreamValid) {
#else
    if (!request.url().isEmpty()) {
#endif
        m_mediaStatus = QMediaPlayer::LoadingMedia;
        m_session->pause();
    } else {
        m_mediaStatus = QMediaPlayer::NoMedia;
        setBufferProgress(0);
    }

    emit positionChanged(position());

    popAndNotifyState();
}

bool QGstreamerMediaPlayer::setAudioOutput(const QAudioDeviceInfo &info)
{
    m_session->setAudioOutputDevice(info);
    return true;
}

QAudioDeviceInfo QGstreamerMediaPlayer::audioOutput() const
{
    return m_session->audioOutputDevice();
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
    return m_session->metaData();
}

void QGstreamerMediaPlayer::setVideoSurface(QAbstractVideoSurface *surface)
{
    m_session->setVideoRenderer(surface);
}

QMediaStreamsControl *QGstreamerMediaPlayer::mediaStreams()
{
    if (!m_streamsControl)
        m_streamsControl = new QGstreamerStreamsControl(m_session, this);
    return m_streamsControl;
}

bool QGstreamerMediaPlayer::isAudioAvailable() const
{
    return m_session->isAudioAvailable();
}

bool QGstreamerMediaPlayer::isVideoAvailable() const
{
    return m_session->isVideoAvailable();
}

void QGstreamerMediaPlayer::updateSessionState(QMediaPlayer::State state)
{
    pushState();

    if (state == QMediaPlayer::StoppedState) {
        m_session->showPrerollFrames(false);
        m_currentState = QMediaPlayer::StoppedState;
    }

    if (state == QMediaPlayer::PausedState && m_currentState != QMediaPlayer::StoppedState) {
        if (m_pendingSeekPosition != -1 && m_session->isSeekable()) {
            m_session->showPrerollFrames(true);
            m_session->seek(m_pendingSeekPosition);
        }
        m_pendingSeekPosition = -1;

        if (m_currentState == QMediaPlayer::PlayingState) {
            if (m_bufferProgress == -1 || m_bufferProgress == 100)
                m_session->play();
        }
    }

    updateMediaStatus();

    popAndNotifyState();
}

void QGstreamerMediaPlayer::updateMediaStatus()
{
    //EndOfMedia status should be kept, until reset by pause, play or setMedia
    if (m_mediaStatus == QMediaPlayer::EndOfMedia)
        return;

    pushState();
    QMediaPlayer::MediaStatus oldStatus = m_mediaStatus;

    switch (m_session->state()) {
    case QMediaPlayer::StoppedState:
        if (m_currentResource.isEmpty())
            m_mediaStatus = QMediaPlayer::NoMedia;
        else if (oldStatus != QMediaPlayer::InvalidMedia)
            m_mediaStatus = QMediaPlayer::LoadingMedia;
        break;

    case QMediaPlayer::PlayingState:
    case QMediaPlayer::PausedState:
        if (m_currentState == QMediaPlayer::StoppedState) {
            m_mediaStatus = QMediaPlayer::LoadedMedia;
        } else {
            if (m_bufferProgress == -1 || m_bufferProgress == 100)
                m_mediaStatus = QMediaPlayer::BufferedMedia;
            else
                m_mediaStatus = QMediaPlayer::StalledMedia;
        }
        break;
    }

    popAndNotifyState();
}

void QGstreamerMediaPlayer::processEOS()
{
    pushState();
    m_mediaStatus = QMediaPlayer::EndOfMedia;
    emit positionChanged(position());
    m_session->endOfMediaReset();

    if (m_currentState != QMediaPlayer::StoppedState) {
        m_currentState = QMediaPlayer::StoppedState;
        m_session->showPrerollFrames(false); // stop showing prerolled frames in stop state
    }

    popAndNotifyState();
}

void QGstreamerMediaPlayer::setBufferProgress(int progress)
{
    if (m_bufferProgress == progress || m_mediaStatus == QMediaPlayer::NoMedia)
        return;

#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO << progress;
#endif
    m_bufferProgress = progress;

    if (m_currentState == QMediaPlayer::PlayingState &&
            m_bufferProgress == 100 &&
            m_session->state() != QMediaPlayer::PlayingState)
        m_session->play();

    if (!m_session->isLiveSource() && m_bufferProgress < 100 &&
            (m_session->state() == QMediaPlayer::PlayingState ||
             m_session->pendingState() == QMediaPlayer::PlayingState))
        m_session->pause();

    updateMediaStatus();

    emit bufferStatusChanged(m_bufferProgress);
}

void QGstreamerMediaPlayer::handleInvalidMedia()
{
    pushState();
    m_mediaStatus = QMediaPlayer::InvalidMedia;
    m_currentState = QMediaPlayer::StoppedState;
    m_setMediaPending = true;
    popAndNotifyState();
}

void QGstreamerMediaPlayer::pushState()
{
    m_stateStack.push(m_currentState);
    m_mediaStatusStack.push(m_mediaStatus);
}

void QGstreamerMediaPlayer::popAndNotifyState()
{
    Q_ASSERT(!m_stateStack.isEmpty());

    QMediaPlayer::State oldState = m_stateStack.pop();
    QMediaPlayer::MediaStatus oldMediaStatus = m_mediaStatusStack.pop();

    if (m_stateStack.isEmpty()) {
        if (m_mediaStatus != oldMediaStatus) {
#ifdef DEBUG_PLAYBIN
            qDebug() << "Media status changed:" << m_mediaStatus;
#endif
            emit mediaStatusChanged(m_mediaStatus);
        }

        if (m_currentState != oldState) {
#ifdef DEBUG_PLAYBIN
            qDebug() << "State changed:" << m_currentState;
#endif
            emit stateChanged(m_currentState);
        }
    }
}

QT_END_NAMESPACE
