/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamerplayercontrol.h"
#include "qgstreamerplayersession.h"
#include "playerresourcepolicy.h"

#include <qmediaplaylistnavigator.h>


#include <QtCore/qdir.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

//#define DEBUG_PLAYBIN

QT_BEGIN_NAMESPACE

QGstreamerPlayerControl::QGstreamerPlayerControl(QGstreamerPlayerSession *session, QObject *parent)
    : QMediaPlayerControl(parent)
    , m_ownStream(false)
    , m_session(session)
    , m_state(QMediaPlayer::StoppedState)
    , m_mediaStatus(QMediaPlayer::NoMedia)
    , m_bufferProgress(-1)
    , m_seekToStartPending(false)
    , m_pendingSeekPosition(-1)
    , m_stream(0)
    , m_fifoNotifier(0)
    , m_fifoCanWrite(false)
    , m_bufferSize(0)
    , m_bufferOffset(0)
{
    m_fifoFd[0] = -1;
    m_fifoFd[1] = -1;

    m_resources = new PlayerResourcePolicy(this);

    connect(m_session, SIGNAL(positionChanged(qint64)),
            this, SLOT(updatePosition(qint64)));
    connect(m_session, SIGNAL(durationChanged(qint64)),
            this, SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(mutedStateChanged(bool)),
            this, SIGNAL(mutedChanged(bool)));
    connect(m_session, SIGNAL(volumeChanged(int)),
            this, SIGNAL(volumeChanged(int)));
    connect(m_session, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SLOT(updateSessionState(QMediaPlayer::State)));
    connect(m_session,SIGNAL(bufferingProgressChanged(int)),
            this, SLOT(setBufferProgress(int)));
    connect(m_session, SIGNAL(playbackFinished()),
            this, SLOT(processEOS()));
    connect(m_session, SIGNAL(audioAvailableChanged(bool)),
            this, SIGNAL(audioAvailableChanged(bool)));
    connect(m_session, SIGNAL(videoAvailableChanged(bool)),
            this, SIGNAL(videoAvailableChanged(bool)));
    connect(m_session, SIGNAL(seekableChanged(bool)),
            this, SIGNAL(seekableChanged(bool)));
    connect(m_session, SIGNAL(error(int,QString)),
            this, SIGNAL(error(int,QString)));
    connect(m_session, SIGNAL(invalidMedia()),
            this, SLOT(handleInvalidMedia()));
    connect(m_session, SIGNAL(playbackRateChanged(qreal)),
            this, SIGNAL(playbackRateChanged(qreal)));
    connect(m_session, SIGNAL(seekableChanged(bool)),
            this, SLOT(applyPendingSeek(bool)));

    connect(m_resources, SIGNAL(resourcesGranted()), SLOT(handleResourcesGranted()));
    connect(m_resources, SIGNAL(resourcesDenied()), SLOT(handleResourcesLost()));
    connect(m_resources, SIGNAL(resourcesLost()), SLOT(handleResourcesLost()));
}

QGstreamerPlayerControl::~QGstreamerPlayerControl()
{
    if (m_fifoFd[0] >= 0) {
        ::close(m_fifoFd[0]);
        ::close(m_fifoFd[1]);
        m_fifoFd[0] = -1;
        m_fifoFd[1] = -1;
    }
}

qint64 QGstreamerPlayerControl::position() const
{
    return m_seekToStartPending ? 0 : m_session->position();
}

qint64 QGstreamerPlayerControl::duration() const
{
    return m_session->duration();
}

QMediaPlayer::State QGstreamerPlayerControl::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus QGstreamerPlayerControl::mediaStatus() const
{
    return m_mediaStatus;
}

int QGstreamerPlayerControl::bufferStatus() const
{
    if (m_bufferProgress == -1) {
        return m_session->state() == QMediaPlayer::StoppedState ? 0 : 100;
    } else
        return m_bufferProgress;
}

int QGstreamerPlayerControl::volume() const
{
    return m_session->volume();
}

bool QGstreamerPlayerControl::isMuted() const
{
    return m_session->isMuted();
}

bool QGstreamerPlayerControl::isSeekable() const
{
    return m_session->isSeekable();
}

QMediaTimeRange QGstreamerPlayerControl::availablePlaybackRanges() const
{
    return m_session->availablePlaybackRanges();
}

qreal QGstreamerPlayerControl::playbackRate() const
{
    return m_session->playbackRate();
}

void QGstreamerPlayerControl::setPlaybackRate(qreal rate)
{
    m_session->setPlaybackRate(rate);
}

void QGstreamerPlayerControl::setPosition(qint64 pos)
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO << pos/1000.0;
#endif

    pushState();

    if (m_mediaStatus == QMediaPlayer::EndOfMedia) {
        m_mediaStatus = QMediaPlayer::LoadedMedia;
        m_seekToStartPending = true;
    }

    if (m_session->isSeekable() && m_session->seek(pos)) {
        m_seekToStartPending = false;
    } else {
        m_pendingSeekPosition = pos;
        //don't display the first video frame since it's not what user requested.
        m_session->showPrerollFrames(false);
    }

    popAndNotifyState();
}

void QGstreamerPlayerControl::play()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif

    playOrPause(QMediaPlayer::PlayingState);
}

void QGstreamerPlayerControl::pause()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif

    playOrPause(QMediaPlayer::PausedState);
}

void QGstreamerPlayerControl::playOrPause(QMediaPlayer::State newState)
{
    if (m_mediaStatus == QMediaPlayer::NoMedia)
        return;

    pushState();
#ifdef Q_WS_MAEMO_6
    //this is a work around for the gstreamer bug,
    //should be remove once it get fixed
    if (newState == QMediaPlayer::PlayingState && m_mediaStatus == QMediaPlayer::InvalidMedia) {
        setMedia(m_currentResource, m_stream);
    }
#endif

    if (m_mediaStatus == QMediaPlayer::EndOfMedia) {
        m_mediaStatus = QMediaPlayer::BufferedMedia;
        m_seekToStartPending = true;
    }

    if (!m_resources->isGranted() && !m_resources->isRequested())
        m_resources->acquire();

    if (m_resources->isGranted()) {
        if (m_seekToStartPending) {
            m_session->pause();
            if (!m_session->seek(0)) {
                m_bufferProgress = -1;
                m_session->stop();
                m_mediaStatus = QMediaPlayer::LoadingMedia;
            }
            m_seekToStartPending = false;
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
    }

    if (m_mediaStatus == QMediaPlayer::InvalidMedia)
        m_mediaStatus = QMediaPlayer::LoadingMedia;

    m_state = newState;

    if (m_mediaStatus == QMediaPlayer::EndOfMedia || m_mediaStatus == QMediaPlayer::LoadedMedia) {
        if (m_bufferProgress == -1 || m_bufferProgress == 100)
            m_mediaStatus = QMediaPlayer::BufferedMedia;
        else
            m_mediaStatus = QMediaPlayer::BufferingMedia;
    }

    popAndNotifyState();

    emit positionChanged(position());
}

void QGstreamerPlayerControl::stop()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif

    pushState();

    if (m_state != QMediaPlayer::StoppedState) {
        m_state = QMediaPlayer::StoppedState;
        if (m_resources->isGranted())
            m_session->pause();

        if (m_mediaStatus != QMediaPlayer::EndOfMedia) {
            m_seekToStartPending = true;
            emit positionChanged(position());
        }
    }

    popAndNotifyState();
}

void QGstreamerPlayerControl::setVolume(int volume)
{
    m_session->setVolume(volume);
}

void QGstreamerPlayerControl::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

QMediaContent QGstreamerPlayerControl::media() const
{
    return m_currentResource;
}

const QIODevice *QGstreamerPlayerControl::mediaStream() const
{
    return m_stream;
}

void QGstreamerPlayerControl::setMedia(const QMediaContent &content, QIODevice *stream)
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif

    pushState();

    m_state = QMediaPlayer::StoppedState;
    QMediaContent oldMedia = m_currentResource;
    m_pendingSeekPosition = -1;
    m_session->showPrerollFrames(true);

    if (!content.isNull() || stream) {
        if (!m_resources->isRequested() && !m_resources->isGranted())
            m_resources->acquire();

        if (!m_resources->isGranted()) {
            m_currentResource = content;
            m_stream = stream;

            m_state = QMediaPlayer::StoppedState;
            m_mediaStatus = QMediaPlayer::LoadingMedia;
            if (m_currentResource != oldMedia)
                emit mediaChanged(m_currentResource);
            popAndNotifyState();
            return;
        }
    } else {
        m_resources->release();
    }

    m_session->stop();

    bool userStreamValid = false;

    if (m_bufferProgress != -1) {
        m_bufferProgress = -1;
        emit bufferStatusChanged(0);
    }

    if (m_stream) {
#if !defined(HAVE_GST_APPSRC)
        closeFifo();

        disconnect(m_stream, SIGNAL(readyRead()), this, SLOT(writeFifo()));
#endif

        if (m_ownStream)
            delete m_stream;
        m_stream = 0;
        m_ownStream = false;
    }

    // If the canonical URL refers to a Qt resource, open with QFile and use
    // the stream playback capability to play.
    if (stream == 0 && content.canonicalUrl().scheme() == QLatin1String("qrc")) {
        stream = new QFile(QLatin1Char(':') + content.canonicalUrl().path(), this);
        if (!stream->open(QIODevice::ReadOnly)) {
            delete stream;
            m_mediaStatus = QMediaPlayer::InvalidMedia;
            m_currentResource = content;
            emit mediaChanged(m_currentResource);
            emit error(QMediaPlayer::FormatError, tr("Attempting to play invalid Qt resource"));
            if (m_state != QMediaPlayer::PlayingState)
                m_resources->release();
            popAndNotifyState();
            return;
        }
        m_ownStream = true;
    }

    m_currentResource = content;
    m_stream = stream;
    m_seekToStartPending = false;

    QNetworkRequest request;

    if (m_stream) {
#if !defined(HAVE_GST_APPSRC)
        if (m_stream->isReadable() && openFifo()) {
            request = QNetworkRequest(QUrl(QString(QLatin1String("fd://%1")).arg(m_fifoFd[0])));
        }
#else
        userStreamValid = stream->isOpen() && m_stream->isReadable();
        request = content.canonicalRequest();
#endif
    } else if (!content.isNull()) {
        request = content.canonicalRequest();
    }

#if !defined(HAVE_GST_APPSRC)
    m_session->loadFromUri(request);
#else
    if (m_stream) {
        if (userStreamValid){
            m_session->loadFromStream(request, m_stream);
        } else {
            m_mediaStatus = QMediaPlayer::InvalidMedia;
            emit error(QMediaPlayer::FormatError, tr("Attempting to play invalid user stream"));
            if (m_state != QMediaPlayer::PlayingState)
                m_resources->release();
            popAndNotifyState();
            return;
        }
    } else
        m_session->loadFromUri(request);
#endif

#if !defined(HAVE_GST_APPSRC)
    if (m_fifoFd[1] >= 0) {
        m_fifoCanWrite = true;

        writeFifo();
    }
#endif

#if defined(HAVE_GST_APPSRC)
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

    if (m_currentResource != oldMedia)
        emit mediaChanged(m_currentResource);

    emit positionChanged(position());

    if (content.isNull() && !stream)
        m_resources->release();

    popAndNotifyState();
}

void QGstreamerPlayerControl::setVideoOutput(QObject *output)
{
    m_session->setVideoRenderer(output);
}

bool QGstreamerPlayerControl::isAudioAvailable() const
{
    return m_session->isAudioAvailable();
}

bool QGstreamerPlayerControl::isVideoAvailable() const
{
    return m_session->isVideoAvailable();
}

void QGstreamerPlayerControl::updateSessionState(QMediaPlayer::State state)
{
    pushState();

    if (state == QMediaPlayer::StoppedState)
        m_state = QMediaPlayer::StoppedState;

    updateMediaStatus();

    popAndNotifyState();
}

void QGstreamerPlayerControl::updateMediaStatus()
{
    pushState();
    QMediaPlayer::MediaStatus oldStatus = m_mediaStatus;

    switch (m_session->state()) {
    case QMediaPlayer::StoppedState:
        if (m_currentResource.isNull())
            m_mediaStatus = QMediaPlayer::NoMedia;
        else if (oldStatus != QMediaPlayer::InvalidMedia)
            m_mediaStatus = QMediaPlayer::LoadingMedia;
        break;

    case QMediaPlayer::PlayingState:
    case QMediaPlayer::PausedState:
        if (m_state == QMediaPlayer::StoppedState) {
            m_mediaStatus = QMediaPlayer::LoadedMedia;
        } else {
            if (m_bufferProgress == -1 || m_bufferProgress == 100)
                m_mediaStatus = QMediaPlayer::BufferedMedia;
            else
                m_mediaStatus = QMediaPlayer::StalledMedia;
        }
        break;
    }

    if (m_state == QMediaPlayer::PlayingState && !m_resources->isGranted())
        m_mediaStatus = QMediaPlayer::StalledMedia;

    //EndOfMedia status should be kept, until reset by pause, play or setMedia
    if (oldStatus == QMediaPlayer::EndOfMedia)
        m_mediaStatus = QMediaPlayer::EndOfMedia;

    popAndNotifyState();
}

void QGstreamerPlayerControl::processEOS()
{
    pushState();
    m_mediaStatus = QMediaPlayer::EndOfMedia;
    emit positionChanged(position());
    stop();
    popAndNotifyState();
}

void QGstreamerPlayerControl::setBufferProgress(int progress)
{
    if (m_bufferProgress == progress || m_mediaStatus == QMediaPlayer::NoMedia)
        return;

#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO << progress;
#endif
    m_bufferProgress = progress;

    if (m_resources->isGranted()) {
        if (m_state == QMediaPlayer::PlayingState &&
                m_bufferProgress == 100 &&
                m_session->state() != QMediaPlayer::PlayingState)
            m_session->play();

        if (!m_session->isLiveSource() && m_bufferProgress < 100 &&
                (m_session->state() == QMediaPlayer::PlayingState ||
                 m_session->pendingState() == QMediaPlayer::PlayingState))
            m_session->pause();
    }

    updateMediaStatus();

    emit bufferStatusChanged(m_bufferProgress);
}

void QGstreamerPlayerControl::writeFifo()
{
    if (m_fifoCanWrite) {
        qint64 bytesToRead = qMin<qint64>(
                m_stream->bytesAvailable(), PIPE_BUF - m_bufferSize);

        if (bytesToRead > 0) {
            int bytesRead = m_stream->read(&m_buffer[m_bufferOffset + m_bufferSize], bytesToRead);

            if (bytesRead > 0)
                m_bufferSize += bytesRead;
        }

        if (m_bufferSize > 0) {
            int bytesWritten = ::write(m_fifoFd[1], &m_buffer[m_bufferOffset], size_t(m_bufferSize));

            if (bytesWritten > 0) {
                m_bufferOffset += bytesWritten;
                m_bufferSize -= bytesWritten;

                if (m_bufferSize == 0)
                    m_bufferOffset = 0;
            } else if (errno == EAGAIN) {
                m_fifoCanWrite = false;
            } else {
                closeFifo();
            }
        }
    }

    m_fifoNotifier->setEnabled(m_stream->bytesAvailable() > 0);
}

void QGstreamerPlayerControl::fifoReadyWrite(int socket)
{
    if (socket == m_fifoFd[1]) {
        m_fifoCanWrite = true;

        writeFifo();
    }
}

bool QGstreamerPlayerControl::openFifo()
{
    Q_ASSERT(m_fifoFd[0] < 0);
    Q_ASSERT(m_fifoFd[1] < 0);

    if (::pipe(m_fifoFd) == 0) {
        int flags = ::fcntl(m_fifoFd[1], F_GETFD);

        if (::fcntl(m_fifoFd[1], F_SETFD, flags | O_NONBLOCK) >= 0) {
            m_fifoNotifier = new QSocketNotifier(m_fifoFd[1], QSocketNotifier::Write);

            connect(m_fifoNotifier, SIGNAL(activated(int)), this, SLOT(fifoReadyWrite(int)));

            return true;
        } else {
            qWarning("Failed to make pipe non blocking %d", errno);

            ::close(m_fifoFd[0]);
            ::close(m_fifoFd[1]);

            m_fifoFd[0] = -1;
            m_fifoFd[1] = -1;

            return false;
        }
    } else {
        qWarning("Failed to create pipe %d", errno);

        return false;
    }
}

void QGstreamerPlayerControl::closeFifo()
{
    if (m_fifoFd[0] >= 0) {
        delete m_fifoNotifier;
        m_fifoNotifier = 0;

        ::close(m_fifoFd[0]);
        ::close(m_fifoFd[1]);
        m_fifoFd[0] = -1;
        m_fifoFd[1] = -1;

        m_fifoCanWrite = false;

        m_bufferSize = 0;
        m_bufferOffset = 0;
    }
}

void QGstreamerPlayerControl::applyPendingSeek(bool isSeekable)
{
    if (isSeekable && m_pendingSeekPosition != -1)
        setPosition(m_pendingSeekPosition);
}

void QGstreamerPlayerControl::handleInvalidMedia()
{
    pushState();
    m_mediaStatus = QMediaPlayer::InvalidMedia;
    m_state = QMediaPlayer::StoppedState;
    popAndNotifyState();
}

void QGstreamerPlayerControl::handleResourcesGranted()
{
    pushState();

    QMediaPlayer::State state = m_state;

    //preserve m_pendingSeekPosition, it's reset on setMedia
    qint64 pos = m_pendingSeekPosition;
    setMedia(m_currentResource, m_stream);

    if (pos != -1)
        setPosition(pos);

    if (state != QMediaPlayer::StoppedState)
        playOrPause(state);
    else
        updateMediaStatus();

    popAndNotifyState();
}

void QGstreamerPlayerControl::handleResourcesLost()
{
    //on resource lost the pipeline should be stopped
    //player status is changed to paused

    pushState();
    QMediaPlayer::State oldState = m_state;

    qint64 pos = m_session->position();
    m_session->stop();
    m_pendingSeekPosition = pos;
    //don't blink the first video frame after playback is restored
    m_session->showPrerollFrames(false);

    if (oldState != QMediaPlayer::StoppedState )
        m_state = QMediaPlayer::PausedState;

    popAndNotifyState();
}

bool QGstreamerPlayerControl::isMediaDownloadEnabled() const
{
    return m_session->property("mediaDownloadEnabled").toBool();
}

void QGstreamerPlayerControl::setMediaDownloadEnabled(bool enabled)
{
    m_session->setProperty("mediaDownloadEnabled", enabled);
}

void QGstreamerPlayerControl::pushState()
{
    m_stateStack.push(m_state);
    m_mediaStatusStack.push(m_mediaStatus);
}

void QGstreamerPlayerControl::popAndNotifyState()
{
    Q_ASSERT(!m_stateStack.isEmpty());

    QMediaPlayer::State oldState = m_stateStack.pop();
    QMediaPlayer::MediaStatus oldMediaStatus = m_mediaStatusStack.pop();

    if (m_stateStack.isEmpty()) {
        if (m_state != oldState) {
#ifdef DEBUG_PLAYBIN
            qDebug() << "State changed:" << m_state;
#endif
            emit stateChanged(m_state);
        }

        if (m_mediaStatus != oldMediaStatus) {
#ifdef DEBUG_PLAYBIN
            qDebug() << "Media status changed:" << m_mediaStatus;
#endif
            emit mediaStatusChanged(m_mediaStatus);
        }
    }
}

void QGstreamerPlayerControl::updatePosition(qint64 pos)
{
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO << pos/1000.0 << "pending:" << m_pendingSeekPosition/1000.0;
#endif

    if (m_pendingSeekPosition != -1) {
        //seek request is complete, it's safe to resume playback
        //with prerolled frame displayed
        m_pendingSeekPosition = -1;
        m_session->showPrerollFrames(true);
        if (m_state == QMediaPlayer::PlayingState) {
            m_session->play();
        }
    }

    emit positionChanged(pos);
}

QT_END_NAMESPACE
