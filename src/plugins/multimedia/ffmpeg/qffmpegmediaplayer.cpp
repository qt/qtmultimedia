// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediaplayer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qvideosink.h"
#include "qaudiooutput.h"

#include "qffmpegplaybackengine_p.h"
#include <qiodevice.h>
#include <qvideosink.h>
#include <qtimer.h>
#include <QtConcurrent/QtConcurrent>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class CancelToken : public ICancelToken
{
public:

    bool isCancelled() const override { return m_cancelled.load(std::memory_order_acquire); }

    void cancel() { m_cancelled.store(true, std::memory_order_release); }

private:
    std::atomic_bool m_cancelled = false;
};

} // namespace QFFmpeg

using namespace QFFmpeg;

QFFmpegMediaPlayer::QFFmpegMediaPlayer(QMediaPlayer *player)
    : QPlatformMediaPlayer(player)
{
    m_positionUpdateTimer.setInterval(50);
    m_positionUpdateTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_positionUpdateTimer, &QTimer::timeout, this, &QFFmpegMediaPlayer::updatePosition);
}

QFFmpegMediaPlayer::~QFFmpegMediaPlayer()
{
    if (m_cancelToken)
        m_cancelToken->cancel();

    m_loadMedia.waitForFinished();
};

qint64 QFFmpegMediaPlayer::duration() const
{
    return m_playbackEngine ? m_playbackEngine->duration() / 1000 : 0;
}

void QFFmpegMediaPlayer::setPosition(qint64 position)
{
    if (mediaStatus() == QMediaPlayer::LoadingMedia)
        return;

    if (m_playbackEngine) {
        m_playbackEngine->seek(position * 1000);
        updatePosition();
    }
    if (state() == QMediaPlayer::StoppedState)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

void QFFmpegMediaPlayer::updatePosition()
{
    positionChanged(m_playbackEngine ? m_playbackEngine->currentPosition() / 1000 : 0);
}

void QFFmpegMediaPlayer::endOfStream()
{
    // start update timer and report end position anyway
    m_positionUpdateTimer.stop();
    positionChanged(duration());

    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::EndOfMedia);
}

void QFFmpegMediaPlayer::onLoopChanged()
{
    // report about finish and start
    // reporting both signals is a bit contraversial
    // but it eshures the idea of notifications about
    // imporatant position points.
    // Also, it ensures more predictable flow for testing.
    positionChanged(duration());
    positionChanged(0);
    m_positionUpdateTimer.stop();
    m_positionUpdateTimer.start();
}

float QFFmpegMediaPlayer::bufferProgress() const
{
    return 1.;
}

QMediaTimeRange QFFmpegMediaPlayer::availablePlaybackRanges() const
{
    return {};
}

qreal QFFmpegMediaPlayer::playbackRate() const
{
    return m_playbackRate;
}

void QFFmpegMediaPlayer::setPlaybackRate(qreal rate)
{
    const float effectiveRate = std::max(static_cast<float>(rate), 0.0f);

    if (qFuzzyCompare(m_playbackRate, effectiveRate))
        return;

    m_playbackRate = effectiveRate;

    if (m_playbackEngine)
        m_playbackEngine->setPlaybackRate(effectiveRate);

    emit playbackRateChanged(effectiveRate);
}

QUrl QFFmpegMediaPlayer::media() const
{
    return m_url;
}

const QIODevice *QFFmpegMediaPlayer::mediaStream() const
{
    return m_device;
}

void QFFmpegMediaPlayer::handleIncorrectMedia(QMediaPlayer::MediaStatus status)
{
    seekableChanged(false);
    audioAvailableChanged(false);
    videoAvailableChanged(false);
    metaDataChanged();
    mediaStatusChanged(status);
    m_playbackEngine = nullptr;
};

void QFFmpegMediaPlayer::setMedia(const QUrl &media, QIODevice *stream)
{
    // Wait for previous unfinished load attempts.
    if (m_cancelToken)
        m_cancelToken->cancel();

    m_loadMedia.waitForFinished();

    m_url = media;
    m_device = stream;
    m_playbackEngine = nullptr;

    if (media.isEmpty() && !stream) {
        handleIncorrectMedia(QMediaPlayer::NoMedia);
        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);

    m_requestedStatus = QMediaPlayer::StoppedState;

    m_cancelToken = std::make_shared<CancelToken>();

    // Load media asynchronously to keep GUI thread responsive while loading media
    m_loadMedia = QtConcurrent::run([this, media, stream, cancelToken = m_cancelToken] {
        // On worker thread
        const MediaDataHolder::Maybe mediaHolder =
                MediaDataHolder::create(media, stream, cancelToken);

        // Transition back to calling thread using invokeMethod because
        // QFuture continuations back on calling thread may deadlock (QTBUG-117918)
        QMetaObject::invokeMethod(this, [this, mediaHolder, cancelToken] {
            setMediaAsync(mediaHolder, cancelToken);
        });
    });
}

void QFFmpegMediaPlayer::setMediaAsync(QFFmpeg::MediaDataHolder::Maybe mediaDataHolder,
                                       const std::shared_ptr<QFFmpeg::CancelToken> &cancelToken)
{
    Q_ASSERT(mediaStatus() == QMediaPlayer::LoadingMedia);

    // If loading was cancelled, we do not emit any signals about failing
    // to load media (or any other events). The rationale is that cancellation
    // either happens during destruction, where the signals are no longer
    // of interest, or it happens as a response to user requesting to load
    // another media file. In the latter case, we don't want to risk popping
    // up error dialogs or similar.
    if (cancelToken->isCancelled()) {
        return;
    }

    if (!mediaDataHolder) {
        const auto [code, description] = mediaDataHolder.error();
        error(code, description);
        handleIncorrectMedia(QMediaPlayer::MediaStatus::InvalidMedia);
        return;
    }

    m_playbackEngine = std::make_unique<PlaybackEngine>();

    connect(m_playbackEngine.get(), &PlaybackEngine::endOfStream, this,
            &QFFmpegMediaPlayer::endOfStream);
    connect(m_playbackEngine.get(), &PlaybackEngine::errorOccured, this,
            &QFFmpegMediaPlayer::error);
    connect(m_playbackEngine.get(), &PlaybackEngine::loopChanged, this,
            &QFFmpegMediaPlayer::onLoopChanged);

    m_playbackEngine->setMedia(std::move(*mediaDataHolder.value()));

    m_playbackEngine->setAudioSink(m_audioOutput);
    m_playbackEngine->setVideoSink(m_videoSink);
    m_playbackEngine->setLoops(loops());
    m_playbackEngine->setPlaybackRate(m_playbackRate);

    durationChanged(duration());
    tracksChanged();
    metaDataChanged();
    seekableChanged(m_playbackEngine->isSeekable());

    audioAvailableChanged(
            !m_playbackEngine->streamInfo(QPlatformMediaPlayer::AudioStream).isEmpty());
    videoAvailableChanged(
            !m_playbackEngine->streamInfo(QPlatformMediaPlayer::VideoStream).isEmpty());

    mediaStatusChanged(QMediaPlayer::LoadedMedia);

    if (m_requestedStatus != QMediaPlayer::StoppedState) {
        if (m_requestedStatus == QMediaPlayer::PlayingState)
            play();
        else if (m_requestedStatus == QMediaPlayer::PausedState)
            pause();
    }
}

void QFFmpegMediaPlayer::play()
{
    if (mediaStatus() == QMediaPlayer::LoadingMedia) {
        m_requestedStatus = QMediaPlayer::PlayingState;
        return;
    }

    if (!m_playbackEngine)
        return;

    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState) {
        m_playbackEngine->seek(0);
        positionChanged(0);
    }

    runPlayback();
}

void QFFmpegMediaPlayer::runPlayback()
{
    m_playbackEngine->play();
    m_positionUpdateTimer.start();
    stateChanged(QMediaPlayer::PlayingState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::pause()
{
    if (mediaStatus() == QMediaPlayer::LoadingMedia) {
        m_requestedStatus = QMediaPlayer::PausedState;
        return;
    }

    if (!m_playbackEngine)
        return;

    if (mediaStatus() == QMediaPlayer::EndOfMedia && state() == QMediaPlayer::StoppedState) {
        m_playbackEngine->seek(0);
        positionChanged(0);
    }
    m_playbackEngine->pause();
    m_positionUpdateTimer.stop();
    stateChanged(QMediaPlayer::PausedState);
    mediaStatusChanged(QMediaPlayer::BufferedMedia);
}

void QFFmpegMediaPlayer::stop()
{
    if (mediaStatus() == QMediaPlayer::LoadingMedia) {
        m_requestedStatus = QMediaPlayer::StoppedState;
        return;
    }

    if (!m_playbackEngine)
        return;

    m_playbackEngine->stop();
    m_positionUpdateTimer.stop();
    positionChanged(0);
    stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(QMediaPlayer::LoadedMedia);
}

void QFFmpegMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;

    m_audioOutput = output;
    if (m_playbackEngine)
        m_playbackEngine->setAudioSink(output);
}

QMediaMetaData QFFmpegMediaPlayer::metaData() const
{
    return m_playbackEngine ? m_playbackEngine->metaData() : QMediaMetaData{};
}

void QFFmpegMediaPlayer::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;
    if (m_playbackEngine)
        m_playbackEngine->setVideoSink(sink);
}

QVideoSink *QFFmpegMediaPlayer::videoSink() const
{
    return m_videoSink;
}

int QFFmpegMediaPlayer::trackCount(TrackType type)
{
    return m_playbackEngine ? m_playbackEngine->streamInfo(type).count() : 0;
}

QMediaMetaData QFFmpegMediaPlayer::trackMetaData(TrackType type, int streamNumber)
{
    if (!m_playbackEngine || streamNumber < 0
        || streamNumber >= m_playbackEngine->streamInfo(type).count())
        return {};
    return m_playbackEngine->streamInfo(type).at(streamNumber).metaData;
}

int QFFmpegMediaPlayer::activeTrack(TrackType type)
{
    return m_playbackEngine ? m_playbackEngine->activeTrack(type) : -1;
}

void QFFmpegMediaPlayer::setActiveTrack(TrackType type, int streamNumber)
{
    if (m_playbackEngine)
        m_playbackEngine->setActiveTrack(type, streamNumber);
    else
        qWarning() << "Cannot set active track without open source";
}

void QFFmpegMediaPlayer::setLoops(int loops)
{
    if (m_playbackEngine)
        m_playbackEngine->setLoops(loops);

    QPlatformMediaPlayer::setLoops(loops);
}

QT_END_NAMESPACE

#include "moc_qffmpegmediaplayer_p.cpp"
