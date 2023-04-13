// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediaplayer_p.h"
#include <common/qwasmvideooutput_p.h>
#include <common/qwasmaudiooutput_p.h>
#include "qaudiooutput.h"

#include <QtCore/qloggingcategory.h>
#include <QUuid>
#include <QtGlobal>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(lcMediaPlayer, "qt.multimedia.mediaplayer.wasm");

QWasmMediaPlayer::QWasmMediaPlayer(QMediaPlayer *parent)
    : QPlatformMediaPlayer(parent),
      m_videoOutput(new QWasmVideoOutput),
      m_State(QWasmMediaPlayer::Idle)
{
     qCDebug(lcMediaPlayer) << Q_FUNC_INFO << this;

}

QWasmMediaPlayer::~QWasmMediaPlayer()
{
    delete m_videoOutput;
}

void QWasmMediaPlayer::initVideo()
{
    m_videoOutput->setVideoMode(QWasmVideoOutput::VideoOutput);
    QUuid videoElementId = QUuid::createUuid();
    qCDebug(lcMediaPlayer) << Q_FUNC_INFO << "videoElementId"<< videoElementId << this;

    m_videoOutput->createVideoElement(videoElementId.toString(QUuid::WithoutBraces).toStdString());
    m_videoOutput->doElementCallbacks();
    m_videoOutput->createOffscreenElement(QSize(1280, 720));
    m_videoOutput->updateVideoElementGeometry(QRect(0, 0, 1280, 720)); // initial size 720p standard

    connect(m_videoOutput, &QWasmVideoOutput::bufferingChanged, this,
            &QWasmMediaPlayer::bufferingChanged);
    connect(m_videoOutput, &QWasmVideoOutput::errorOccured, this,
            &QWasmMediaPlayer::errorOccured);
    connect(m_videoOutput, &QWasmVideoOutput::stateChanged, this,
            &QWasmMediaPlayer::mediaStateChanged);
    connect(m_videoOutput, &QWasmVideoOutput::progressChanged, this,
            &QWasmMediaPlayer::setPositionChanged);
    connect(m_videoOutput, &QWasmVideoOutput::durationChanged, this,
            &QWasmMediaPlayer::setDurationChanged);
    connect(m_videoOutput, &QWasmVideoOutput::sizeChange, this,
            &QWasmMediaPlayer::videoSizeChanged);
    connect(m_videoOutput, &QWasmVideoOutput::readyChanged, this,
            &QWasmMediaPlayer::videoOutputReady);
    connect(m_videoOutput, &QWasmVideoOutput::statusChanged, this,
            &QWasmMediaPlayer::onMediaStatusChanged);
    connect(m_videoOutput, &QWasmVideoOutput::metaDataLoaded, this,
            &QWasmMediaPlayer::videoMetaDataChanged);

    setVideoAvailable(true);
}

void QWasmMediaPlayer::initAudio()
{
    connect(m_audioOutput->q, &QAudioOutput::deviceChanged,
            this, &QWasmMediaPlayer::updateAudioDevice);
    connect(m_audioOutput->q, &QAudioOutput::volumeChanged,
            this, &QWasmMediaPlayer::volumeChanged);
    connect(m_audioOutput->q, &QAudioOutput::mutedChanged,
            this, &QWasmMediaPlayer::mutedChanged);

    connect(m_audioOutput, &QWasmAudioOutput::bufferingChanged, this,
            &QWasmMediaPlayer::bufferingChanged);
    connect(m_audioOutput, &QWasmAudioOutput::errorOccured, this,
            &QWasmMediaPlayer::errorOccured);
    connect(m_audioOutput, &QWasmAudioOutput::progressChanged, this,
            &QWasmMediaPlayer::setPositionChanged);
    connect(m_audioOutput, &QWasmAudioOutput::durationChanged, this,
            &QWasmMediaPlayer::setDurationChanged);
    connect(m_audioOutput, &QWasmAudioOutput::statusChanged, this,
            &QWasmMediaPlayer::onMediaStatusChanged);
    connect(m_audioOutput, &QWasmAudioOutput::stateChanged, this,
            &QWasmMediaPlayer::mediaStateChanged);
   setAudioAvailable(true);
}

qint64 QWasmMediaPlayer::duration() const
{
    return m_videoOutput->getDuration();
}

qint64 QWasmMediaPlayer::position() const
{
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        return duration();

    if (m_videoAvailable)
        return m_videoOutput->getCurrentPosition();

    return 0;
}

void QWasmMediaPlayer::setPosition(qint64 position)
{
    if (!isSeekable())
        return;

    const int seekPosition = (position > INT_MAX) ? INT_MAX : position;

    if (seekPosition == this->position())
        return;

    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        setMediaStatus(QMediaPlayer::LoadedMedia);

    if (m_videoAvailable)
        return m_videoOutput->seekTo(position);

    emit positionChanged(seekPosition);
}

void QWasmMediaPlayer::volumeChanged(float gain)
{
    if (m_State != QWasmMediaPlayer::Started)
        return;

    if (m_videoAvailable)
        m_videoOutput->setVolume(gain);
}

void QWasmMediaPlayer::mutedChanged(bool muted)
{
    if (m_State != QWasmMediaPlayer::Started)
        return;

    if (m_videoAvailable)
        m_videoOutput->setMuted(muted);
}

float QWasmMediaPlayer::bufferProgress() const
{
    return qBound(0.0, (m_bufferPercent * .01), 1.0);
}

bool QWasmMediaPlayer::isAudioAvailable() const
{
    return m_audioAvailable;
}

bool QWasmMediaPlayer::isVideoAvailable() const
{
    return m_videoAvailable;
}

QMediaTimeRange QWasmMediaPlayer::availablePlaybackRanges() const
{
    return m_availablePlaybackRange;
}

void QWasmMediaPlayer::updateAvailablePlaybackRanges()
{
    if (m_buffering) {
        const qint64 pos = position();
        const qint64 end = (duration() / 100) * m_bufferPercent;
        m_availablePlaybackRange.addInterval(pos, end);
    } else if (isSeekable()) {
        m_availablePlaybackRange = QMediaTimeRange(0, duration());
    } else {
        m_availablePlaybackRange = QMediaTimeRange();
    }
}

qreal QWasmMediaPlayer::playbackRate() const
{
    if (m_State != QWasmMediaPlayer::Started)
        return 0;

    if (isVideoAvailable())
        return m_videoOutput->playbackRate();
    return 0;
}

void QWasmMediaPlayer::setPlaybackRate(qreal rate)
{
    if (m_State != QWasmMediaPlayer::Started || !isVideoAvailable())
        return;

    m_videoOutput->setPlaybackRate(rate);
    emit playbackRateChanged(rate);
}

QUrl QWasmMediaPlayer::media() const
{
    return m_mediaContent;
}

const QIODevice *QWasmMediaPlayer::mediaStream() const
{
    return m_mediaStream;
}

void QWasmMediaPlayer::setMedia(const QUrl &mediaContent, QIODevice *stream)
{
    qDebug() << Q_FUNC_INFO << mediaContent << isVideoAvailable()
             << isAudioAvailable();
    if (mediaContent.isEmpty()) {
        if (stream) {
            m_mediaStream = stream;
            if (isVideoAvailable()) {
                m_videoOutput->setSource(m_mediaStream);
            }  else {
                m_audioOutput->setSource(m_mediaStream);
            }
        } else {

            setMediaStatus(QMediaPlayer::NoMedia);
        }
    } else {
        if (isVideoAvailable())
            m_videoOutput->setSource(mediaContent);
        else
            m_audioOutput->setSource(mediaContent);
    }

    resetBufferingProgress();
}

void QWasmMediaPlayer::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;

    if (!m_videoSink)
        return;

    initVideo();
    m_videoOutput->setSurface(sink);
}

void QWasmMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    if (m_audioOutput)
        m_audioOutput->q->disconnect(this);
    m_audioOutput = static_cast<QWasmAudioOutput *>(output);
}

void QWasmMediaPlayer::updateAudioDevice()
{
    if (m_audioOutput) {
       m_audioOutput->setAudioDevice(m_audioOutput->q->device());
    }
}

void QWasmMediaPlayer::play()
{
    resetCurrentLoop();

    if (isVideoAvailable()) {
        m_videoOutput->start();
        m_playWhenReady = true;
    } else {
        initAudio();
        if (isAudioAvailable()) {
            m_audioOutput->start();
        }
    }

#ifdef DEBUG_AUDIOENGINE
    QAudioEnginePrivate::checkNoError("play");
#endif
}

void QWasmMediaPlayer::pause()
{
    if ((m_State
         & (QWasmMediaPlayer::Started | QWasmMediaPlayer::Paused
            | QWasmMediaPlayer::PlaybackCompleted)) == 0) {
        return;
    }
    if (isVideoAvailable()) {
        m_videoOutput->pause();
    } else {
        m_audioOutput->pause();
        stateChanged(QMediaPlayer::PausedState);
    }
}

void QWasmMediaPlayer::stop()
{
    m_playWhenReady = false;

    if (m_State == QWasmMediaPlayer::Idle || m_State == QWasmMediaPlayer::PlaybackCompleted
        || m_State == QWasmMediaPlayer::Stopped) {
          qWarning() << Q_FUNC_INFO << __LINE__;
        return;
    }

    if (isVideoAvailable()) {
        m_videoOutput->stop();
    } else {
        m_audioOutput->stop();
    }

}

bool QWasmMediaPlayer::isSeekable() const
{
    return isVideoAvailable() && m_videoOutput->isVideoSeekable();
}

void QWasmMediaPlayer::errorOccured(qint32 code, const QString &message)
{
    QString errorString;
    QMediaPlayer::Error error = QMediaPlayer::ResourceError;

    switch (code) {
    case QWasmMediaNetworkState::NetworkEmpty: // no data
        break;
    case QWasmMediaNetworkState::NetworkIdle:
        break;
    case QWasmMediaNetworkState::NetworkLoading:
        break;
    case QWasmMediaNetworkState::NetworkNoSource: // no source
        error = QMediaPlayer::ResourceError;
        errorString = message;
        break;
    };

    emit QPlatformMediaPlayer::error(error, errorString);
}

void QWasmMediaPlayer::bufferingChanged(qint32 percent)
{
    m_buffering = percent != 100;
    m_bufferPercent = percent;

    updateAvailablePlaybackRanges();
    emit bufferProgressChanged(bufferProgress());
}

void QWasmMediaPlayer::videoSizeChanged(qint32 width, qint32 height)
{
    QSize newSize(width, height);

    if (width == 0 || height == 0 || newSize == m_videoSize)
        return;

    m_videoSize = newSize;
}

void QWasmMediaPlayer::mediaStateChanged(QWasmMediaPlayer::QWasmMediaPlayerState state)
{
    m_State = state;
    QMediaPlayer::PlaybackState m_mediaPlayerState;
    switch (m_State) {
    case QWasmMediaPlayer::Started:
        m_mediaPlayerState = QMediaPlayer::PlayingState;
        break;
    case QWasmMediaPlayer::Paused:
        m_mediaPlayerState = QMediaPlayer::PausedState;
        break;
    case QWasmMediaPlayer::Stopped:
        m_mediaPlayerState = QMediaPlayer::StoppedState;
        break;
    default:
        m_mediaPlayerState = QMediaPlayer::StoppedState;
        break;
    };

    QPlatformMediaPlayer::stateChanged(m_mediaPlayerState);
}

int QWasmMediaPlayer::trackCount(TrackType trackType)
{
    Q_UNUSED(trackType)
    // TODO QTBUG-108517
    return 0; // tracks.count();
}

void QWasmMediaPlayer::setPositionChanged(qint64 position)
{
    QPlatformMediaPlayer::positionChanged(position);
}

void QWasmMediaPlayer::setDurationChanged(qint64 duration)
{
    QPlatformMediaPlayer::durationChanged(duration);
}

void QWasmMediaPlayer::videoOutputReady(bool ready)
{
    setVideoAvailable(ready);

    if (m_playWhenReady && m_videoOutput->isReady())
        play();
}

void QWasmMediaPlayer::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    mediaStatusChanged(status);

    switch (status) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::InvalidMedia:
        emit durationChanged(0);
        break;
    case QMediaPlayer::EndOfMedia:
        setPositionChanged(position());
    default:
        break;
    };
}

void QWasmMediaPlayer::setAudioAvailable(bool available)
{
    if (m_audioAvailable == available)
        return;

    m_audioAvailable = available;
    emit audioAvailableChanged(m_audioAvailable);
}

void QWasmMediaPlayer::setVideoAvailable(bool available)
{
    if (m_videoAvailable == available)
        return;

    if (!available)
        m_videoSize = QSize();

    m_videoAvailable = available;
    emit videoAvailableChanged(m_videoAvailable);
}

void QWasmMediaPlayer::resetBufferingProgress()
{
    m_buffering = false;
    m_bufferPercent = 0;
    m_availablePlaybackRange = QMediaTimeRange();
}

void QWasmMediaPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    setMediaStatus(status);
}

void QWasmMediaPlayer::videoMetaDataChanged()
{
    metaDataChanged();
}

QT_END_NAMESPACE

#include "moc_qwasmmediaplayer_p.cpp"
