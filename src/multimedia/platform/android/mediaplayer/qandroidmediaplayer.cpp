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

#include "qandroidmediaplayer_p.h"
#include "androidmediaplayer_p.h"
#include "qandroidvideooutput_p.h"
#include "qandroidmetadata_p.h"
#include "qandroidaudiooutput_p.h"
#include "qaudiooutput.h"

QT_BEGIN_NAMESPACE

class StateChangeNotifier
{
public:
    StateChangeNotifier(QAndroidMediaPlayer *mp)
        : mControl(mp)
        , mPreviousState(mp->state())
        , mPreviousMediaStatus(mp->mediaStatus())
    {
        ++mControl->mActiveStateChangeNotifiers;
    }

    ~StateChangeNotifier()
    {
        if (--mControl->mActiveStateChangeNotifiers)
            return;

        if (mPreviousMediaStatus != mControl->mediaStatus())
            Q_EMIT mControl->mediaStatusChanged(mControl->mediaStatus());

        if (mPreviousState != mControl->state())
            Q_EMIT mControl->stateChanged(mControl->state());
    }

private:
    QAndroidMediaPlayer *mControl;
    QMediaPlayer::PlaybackState mPreviousState;
    QMediaPlayer::MediaStatus mPreviousMediaStatus;
};


QAndroidMediaPlayer::QAndroidMediaPlayer(QMediaPlayer *parent)
    : QPlatformMediaPlayer(parent),
      mMediaPlayer(new AndroidMediaPlayer),
      mState(AndroidMediaPlayer::Uninitialized)
{
    connect(mMediaPlayer,SIGNAL(bufferingChanged(qint32)),
            this,SLOT(onBufferingChanged(qint32)));
    connect(mMediaPlayer,SIGNAL(info(qint32,qint32)),
            this,SLOT(onInfo(qint32,qint32)));
    connect(mMediaPlayer,SIGNAL(error(qint32,qint32)),
            this,SLOT(onError(qint32,qint32)));
    connect(mMediaPlayer,SIGNAL(stateChanged(qint32)),
            this,SLOT(onStateChanged(qint32)));
    connect(mMediaPlayer,SIGNAL(videoSizeChanged(qint32,qint32)),
            this,SLOT(onVideoSizeChanged(qint32,qint32)));
    connect(mMediaPlayer,SIGNAL(progressChanged(qint64)),
            this,SIGNAL(positionChanged(qint64)));
    connect(mMediaPlayer,SIGNAL(durationChanged(qint64)),
            this,SIGNAL(durationChanged(qint64)));
}

QAndroidMediaPlayer::~QAndroidMediaPlayer()
{
    mMediaPlayer->release();
    delete mMediaPlayer;
}

qint64 QAndroidMediaPlayer::duration() const
{
    if ((mState & (AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::Stopped
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        return 0;
    }

    return mMediaPlayer->getDuration();
}

qint64 QAndroidMediaPlayer::position() const
{
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        return duration();

    if ((mState & (AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted))) {
        return mMediaPlayer->getCurrentPosition();
    }

    return (mPendingPosition == -1) ? 0 : mPendingPosition;
}

void QAndroidMediaPlayer::setPosition(qint64 position)
{
    if (!isSeekable())
        return;

    const int seekPosition = (position > INT_MAX) ? INT_MAX : position;

    if (seekPosition == this->position())
        return;

    StateChangeNotifier notifier(this);

    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        setMediaStatus(QMediaPlayer::LoadedMedia);

    if ((mState & (AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        mPendingPosition = seekPosition;
    } else {
        mMediaPlayer->seekTo(seekPosition);

        if (mPendingPosition != -1) {
            mPendingPosition = -1;
        }
    }

    Q_EMIT positionChanged(seekPosition);
}

void QAndroidMediaPlayer::setVolume(float volume)
{
    if ((mState & (AndroidMediaPlayer::Idle
                   | AndroidMediaPlayer::Initialized
                   | AndroidMediaPlayer::Stopped
                   | AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        mPendingVolume = volume;
        return;
    }

    mMediaPlayer->setVolume(qRound(volume*100.));
    mPendingVolume = -1;
}

void QAndroidMediaPlayer::setMuted(bool muted)
{
    if ((mState & (AndroidMediaPlayer::Idle
                   | AndroidMediaPlayer::Initialized
                   | AndroidMediaPlayer::Stopped
                   | AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        mPendingMute = muted;
        return;
    }

    mMediaPlayer->setMuted(muted);
    mPendingMute = -1;
}

QMediaMetaData QAndroidMediaPlayer::metaData() const
{
    return QAndroidMetaData::extractMetadata(mMediaContent);
}

float QAndroidMediaPlayer::bufferProgress() const
{
    return mBufferFilled ? 1. : 0;
}

bool QAndroidMediaPlayer::isAudioAvailable() const
{
    return mAudioAvailable;
}

bool QAndroidMediaPlayer::isVideoAvailable() const
{
    return mVideoAvailable;
}

QMediaTimeRange QAndroidMediaPlayer::availablePlaybackRanges() const
{
    return mAvailablePlaybackRange;
}

void QAndroidMediaPlayer::updateAvailablePlaybackRanges()
{
    if (mBuffering) {
        const qint64 pos = position();
        const qint64 end = (duration() / 100) * mBufferPercent;
        mAvailablePlaybackRange.addInterval(pos, end);
    } else if (isSeekable()) {
        mAvailablePlaybackRange = QMediaTimeRange(0, duration());
    } else {
        mAvailablePlaybackRange = QMediaTimeRange();
    }

// ####    Q_EMIT availablePlaybackRangesChanged(mAvailablePlaybackRange);
}

qreal QAndroidMediaPlayer::playbackRate() const
{
    if (mHasPendingPlaybackRate ||
            (mState & (AndroidMediaPlayer::Initialized
                       | AndroidMediaPlayer::Prepared
                       | AndroidMediaPlayer::Started
                       | AndroidMediaPlayer::Paused
                       | AndroidMediaPlayer::PlaybackCompleted
                       | AndroidMediaPlayer::Error)) == 0) {
        return mPendingPlaybackRate;
    }

    return mMediaPlayer->playbackRate();
}

void QAndroidMediaPlayer::setPlaybackRate(qreal rate)
{
   if ((mState & (AndroidMediaPlayer::Initialized
                   | AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted
                   | AndroidMediaPlayer::Error)) == 0) {
        if (mPendingPlaybackRate != rate) {
            mPendingPlaybackRate = rate;
            mHasPendingPlaybackRate = true;
            Q_EMIT playbackRateChanged(rate);
        }
        return;
    }

    bool succeeded = mMediaPlayer->setPlaybackRate(rate);

    if (mHasPendingPlaybackRate) {
        mHasPendingPlaybackRate = false;
        mPendingPlaybackRate = qreal(1.0);
        if (!succeeded)
             Q_EMIT playbackRateChanged(playbackRate());
    } else if (succeeded) {
        Q_EMIT playbackRateChanged(rate);
    }
}

QUrl QAndroidMediaPlayer::media() const
{
    return mMediaContent;
}

const QIODevice *QAndroidMediaPlayer::mediaStream() const
{
    return mMediaStream;
}

void QAndroidMediaPlayer::setMedia(const QUrl &mediaContent,
                                          QIODevice *stream)
{
    StateChangeNotifier notifier(this);

    mReloadingMedia = (mMediaContent == mediaContent) && !mPendingSetMedia;

    if (!mReloadingMedia) {
        mMediaContent = mediaContent;
        mMediaStream = stream;
    }

    // Release the mediaplayer if it's not in in Idle or Uninitialized state
    if ((mState & (AndroidMediaPlayer::Idle | AndroidMediaPlayer::Uninitialized)) == 0)
        mMediaPlayer->release();

    if (mediaContent.isEmpty()) {
        setMediaStatus(QMediaPlayer::NoMedia);
    } else {
        if (mVideoOutput && !mVideoOutput->isReady()) {
            // if a video output is set but the video texture is not ready, delay loading the media
            // since it can cause problems on some hardware
            mPendingSetMedia = true;
            return;
        }

        if (mVideoSize.isValid() && mVideoOutput)
            mVideoOutput->setVideoSize(mVideoSize);

        if ((mMediaPlayer->display() == 0) && mVideoOutput)
            mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture());
        mMediaPlayer->setDataSource(QNetworkRequest(mediaContent));
        mMediaPlayer->prepareAsync();
    }

    resetBufferingProgress();

    mReloadingMedia = false;
}

void QAndroidMediaPlayer::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    m_videoSink = sink;

    if (!m_videoSink) {
        if (mVideoOutput) {
            delete mVideoOutput;
            mVideoOutput = nullptr;
        }
        return;
    }

    if (!mVideoOutput) {
        mVideoOutput = new QAndroidTextureVideoOutput(this);
        connect(mVideoOutput, SIGNAL(readyChanged(bool)), this, SLOT(onVideoOutputReady(bool)));
    }

    mVideoOutput->setSurface(sink);
    if (mVideoOutput->isReady())
        mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture());
}

void QAndroidMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    if (m_audioOutput)
        m_audioOutput->q->disconnect(this);
    m_audioOutput = static_cast<QAndroidAudioOutput *>(output);
    if (m_audioOutput) {
        // #### Implement device changes: connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this, XXXX);
        connect(m_audioOutput->q, &QAudioOutput::volumeChanged, this, &QAndroidMediaPlayer::setVolume);
        connect(m_audioOutput->q, &QAudioOutput::mutedChanged, this, &QAndroidMediaPlayer::setMuted);
    }
}

void QAndroidMediaPlayer::play()
{
    StateChangeNotifier notifier(this);

    // We need to prepare the mediaplayer again.
    if ((mState & AndroidMediaPlayer::Stopped) && !mMediaContent.isEmpty()) {
        setMedia(mMediaContent, mMediaStream);
    }

    if (!mMediaContent.isEmpty())
        stateChanged(QMediaPlayer::PlayingState);

    if ((mState & (AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        mPendingState = QMediaPlayer::PlayingState;
        return;
    }

    mMediaPlayer->play();
}

void QAndroidMediaPlayer::pause()
{
    StateChangeNotifier notifier(this);

    stateChanged(QMediaPlayer::PausedState);

    if ((mState & (AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        mPendingState = QMediaPlayer::PausedState;
        return;
    }

    mMediaPlayer->pause();
}

void QAndroidMediaPlayer::stop()
{
    StateChangeNotifier notifier(this);

    stateChanged(QMediaPlayer::StoppedState);

    if ((mState & (AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Stopped
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted)) == 0) {
        if ((mState & (AndroidMediaPlayer::Idle | AndroidMediaPlayer::Uninitialized | AndroidMediaPlayer::Error)) == 0)
            mPendingState = QMediaPlayer::StoppedState;
        return;
    }

    mMediaPlayer->stop();
}

void QAndroidMediaPlayer::onInfo(qint32 what, qint32 extra)
{
    StateChangeNotifier notifier(this);

    Q_UNUSED(extra);
    switch (what) {
    case AndroidMediaPlayer::MEDIA_INFO_UNKNOWN:
        break;
    case AndroidMediaPlayer::MEDIA_INFO_VIDEO_TRACK_LAGGING:
        // IGNORE
        break;
    case AndroidMediaPlayer::MEDIA_INFO_VIDEO_RENDERING_START:
        break;
    case AndroidMediaPlayer::MEDIA_INFO_BUFFERING_START:
        mPendingState = state();
        stateChanged(QMediaPlayer::PausedState);
        setMediaStatus(QMediaPlayer::StalledMedia);
        break;
    case AndroidMediaPlayer::MEDIA_INFO_BUFFERING_END:
        if (state() != QMediaPlayer::StoppedState)
            flushPendingStates();
        break;
    case AndroidMediaPlayer::MEDIA_INFO_BAD_INTERLEAVING:
        break;
    case AndroidMediaPlayer::MEDIA_INFO_NOT_SEEKABLE:
        seekableChanged(false);
        break;
    case AndroidMediaPlayer::MEDIA_INFO_METADATA_UPDATE:
        Q_EMIT metaDataChanged();
        break;
    }
}

void QAndroidMediaPlayer::onError(qint32 what, qint32 extra)
{
    StateChangeNotifier notifier(this);

    QString errorString;
    QMediaPlayer::Error error = QMediaPlayer::ResourceError;

    switch (what) {
    case AndroidMediaPlayer::MEDIA_ERROR_UNKNOWN:
        errorString = QLatin1String("Error:");
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_SERVER_DIED:
        errorString = QLatin1String("Error: Server died");
        error = QMediaPlayer::ResourceError;
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_INVALID_STATE:
        errorString = QLatin1String("Error: Invalid state");
        error = QMediaPlayer::ResourceError;
        break;
    }

    switch (extra) {
    case AndroidMediaPlayer::MEDIA_ERROR_IO: // Network OR file error
        errorString += QLatin1String(" (I/O operation failed)");
        error = QMediaPlayer::NetworkError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_MALFORMED:
        errorString += QLatin1String(" (Malformed bitstream)");
        error = QMediaPlayer::FormatError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_UNSUPPORTED:
        errorString += QLatin1String(" (Unsupported media)");
        error = QMediaPlayer::FormatError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_TIMED_OUT:
        errorString += QLatin1String(" (Timed out)");
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK:
        errorString += QLatin1String(" (Unable to start progressive playback')");
        error = QMediaPlayer::FormatError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case AndroidMediaPlayer::MEDIA_ERROR_BAD_THINGS_ARE_GOING_TO_HAPPEN:
        errorString += QLatin1String(" (Unknown error/Insufficient resources)");
        error = QMediaPlayer::ResourceError;
        break;
    }

    Q_EMIT QPlatformMediaPlayer::error(error, errorString);
}

void QAndroidMediaPlayer::onBufferingChanged(qint32 percent)
{
    StateChangeNotifier notifier(this);

    mBuffering = percent != 100;
    mBufferPercent = percent;

    updateAvailablePlaybackRanges();

    if (state() != QMediaPlayer::StoppedState)
        setMediaStatus(mBuffering ? QMediaPlayer::BufferingMedia : QMediaPlayer::BufferedMedia);
}

void QAndroidMediaPlayer::onVideoSizeChanged(qint32 width, qint32 height)
{
    QSize newSize(width, height);

    if (width == 0 || height == 0 || newSize == mVideoSize)
        return;

    setVideoAvailable(true);
    mVideoSize = newSize;

    if (mVideoOutput)
        mVideoOutput->setVideoSize(mVideoSize);
}

void QAndroidMediaPlayer::onStateChanged(qint32 state)
{
    // If reloading, don't report state changes unless the new state is Prepared or Error.
    if ((mState & AndroidMediaPlayer::Stopped)
        && (state & (AndroidMediaPlayer::Prepared | AndroidMediaPlayer::Error | AndroidMediaPlayer::Uninitialized)) == 0) {
        return;
    }

    StateChangeNotifier notifier(this);

    mState = state;
    switch (mState) {
    case AndroidMediaPlayer::Idle:
        break;
    case AndroidMediaPlayer::Initialized:
        break;
    case AndroidMediaPlayer::Preparing:
        if (!mReloadingMedia)
            setMediaStatus(QMediaPlayer::LoadingMedia);
        break;
    case AndroidMediaPlayer::Prepared:
        setMediaStatus(QMediaPlayer::LoadedMedia);
        if (mBuffering) {
            setMediaStatus(mBufferPercent == 100 ? QMediaPlayer::BufferedMedia
                                                 : QMediaPlayer::BufferingMedia);
        } else {
            onBufferingChanged(100);
        }
        Q_EMIT metaDataChanged();
        setAudioAvailable(true);
        flushPendingStates();
        break;
    case AndroidMediaPlayer::Started:
        stateChanged(QMediaPlayer::PlayingState);
        if (mBuffering) {
            setMediaStatus(mBufferPercent == 100 ? QMediaPlayer::BufferedMedia
                                                 : QMediaPlayer::BufferingMedia);
        } else {
            setMediaStatus(QMediaPlayer::BufferedMedia);
        }
        Q_EMIT positionChanged(position());
        break;
    case AndroidMediaPlayer::Paused:
        stateChanged(QMediaPlayer::PausedState);
        break;
    case AndroidMediaPlayer::Error:
        stateChanged(QMediaPlayer::StoppedState);
        setMediaStatus(QMediaPlayer::InvalidMedia);
        mMediaPlayer->release();
        Q_EMIT positionChanged(0);
        break;
    case AndroidMediaPlayer::Stopped:
        stateChanged(QMediaPlayer::StoppedState);
        setMediaStatus(QMediaPlayer::LoadedMedia);
        Q_EMIT positionChanged(0);
        break;
    case AndroidMediaPlayer::PlaybackCompleted:
        stateChanged(QMediaPlayer::StoppedState);
        setMediaStatus(QMediaPlayer::EndOfMedia);
        break;
    case AndroidMediaPlayer::Uninitialized:
        // reset some properties (unless we reload the same media)
        if (!mReloadingMedia) {
            resetBufferingProgress();
            mPendingPosition = -1;
            mPendingSetMedia = false;
            mPendingState = -1;

            Q_EMIT durationChanged(0);
            Q_EMIT positionChanged(0);

            setAudioAvailable(false);
            setVideoAvailable(false);
            seekableChanged(true);
        }
        break;
    default:
        break;
    }

    if ((mState & (AndroidMediaPlayer::Stopped | AndroidMediaPlayer::Uninitialized)) != 0) {
        mMediaPlayer->setDisplay(0);
        if (mVideoOutput) {
            mVideoOutput->stop();
            mVideoOutput->reset();
        }
    }
}

void QAndroidMediaPlayer::onVideoOutputReady(bool ready)
{
    if ((mMediaPlayer->display() == 0) && mVideoOutput && ready)
        mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture());

    flushPendingStates();
}

void QAndroidMediaPlayer::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    mediaStatusChanged(status);

    if (status == QMediaPlayer::NoMedia || status == QMediaPlayer::InvalidMedia)
        Q_EMIT durationChanged(0);

    if (status == QMediaPlayer::EndOfMedia)
        Q_EMIT positionChanged(position());

    updateBufferStatus();
}

void QAndroidMediaPlayer::setAudioAvailable(bool available)
{
    if (mAudioAvailable == available)
        return;

    mAudioAvailable = available;
    Q_EMIT audioAvailableChanged(mAudioAvailable);
}

void QAndroidMediaPlayer::setVideoAvailable(bool available)
{
    if (mVideoAvailable == available)
        return;

    if (!available)
        mVideoSize = QSize();

    mVideoAvailable = available;
    Q_EMIT videoAvailableChanged(mVideoAvailable);
}

void QAndroidMediaPlayer::resetBufferingProgress()
{
    mBuffering = false;
    mBufferPercent = 0;
    mAvailablePlaybackRange = QMediaTimeRange();
}

void QAndroidMediaPlayer::flushPendingStates()
{
    if (mPendingSetMedia) {
        setMedia(mMediaContent, 0);
        mPendingSetMedia = false;
        return;
    }

    const int newState = mPendingState;
    mPendingState = -1;

    if (mPendingPosition != -1)
        setPosition(mPendingPosition);
    if (mPendingVolume >= 0)
        setVolume(mPendingVolume);
    if (mPendingMute != -1)
        setMuted((mPendingMute == 1));
    if (mHasPendingPlaybackRate)
        setPlaybackRate(mPendingPlaybackRate);

    switch (newState) {
    case QMediaPlayer::PlayingState:
        play();
        break;
    case QMediaPlayer::PausedState:
        pause();
        break;
    case QMediaPlayer::StoppedState:
        stop();
        break;
    default:
        break;
    }
}

void QAndroidMediaPlayer::updateBufferStatus()
{
    auto status = mediaStatus();
    bool bufferFilled = (status == QMediaPlayer::BufferedMedia || status == QMediaPlayer::BufferingMedia);

    if (mBufferFilled != bufferFilled) {
        mBufferFilled = bufferFilled;
        Q_EMIT bufferProgressChanged(bufferProgress());
    }
}

QT_END_NAMESPACE
