/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidmediaplayercontrol.h"
#include "jmediaplayer.h"
#include "qandroidvideooutput.h"

QT_BEGIN_NAMESPACE

static void textureReadyCallback(void *context)
{
    if (context)
        reinterpret_cast<QAndroidMediaPlayerControl *>(context)->onSurfaceTextureReady();
}

QAndroidMediaPlayerControl::QAndroidMediaPlayerControl(QObject *parent)
    : QMediaPlayerControl(parent),
      mMediaPlayer(new JMediaPlayer),
      mCurrentState(QMediaPlayer::StoppedState),
      mCurrentMediaStatus(QMediaPlayer::NoMedia),
      mMediaStream(0),
      mVideoOutput(0),
      mSeekable(true),
      mBufferPercent(-1),
      mAudioAvailable(false),
      mVideoAvailable(false),
      mBuffering(false),
      mMediaPlayerReady(false),
      mPendingPosition(-1),
      mPendingSetMedia(false)
{
    connect(mMediaPlayer, SIGNAL(bufferingUpdate(qint32)),
            this, SLOT(onBufferChanged(qint32)));
    connect(mMediaPlayer, SIGNAL(info(qint32, qint32)),
            this, SLOT(onInfo(qint32, qint32)));
    connect(mMediaPlayer, SIGNAL(error(qint32, qint32)),
            this, SLOT(onError(qint32, qint32)));
    connect(mMediaPlayer, SIGNAL(mediaPlayerInfo(qint32, qint32)),
            this, SLOT(onMediaPlayerInfo(qint32, qint32)));
    connect(mMediaPlayer, SIGNAL(videoSizeChanged(qint32,qint32)),
            this, SLOT(onVideoSizeChanged(qint32,qint32)));
}

QAndroidMediaPlayerControl::~QAndroidMediaPlayerControl()
{
    mMediaPlayer->release();
    delete mMediaPlayer;
}

QMediaPlayer::State QAndroidMediaPlayerControl::state() const
{
    return mCurrentState;
}

QMediaPlayer::MediaStatus QAndroidMediaPlayerControl::mediaStatus() const
{
    return mCurrentMediaStatus;
}

qint64 QAndroidMediaPlayerControl::duration() const
{
    return (mCurrentMediaStatus == QMediaPlayer::InvalidMedia
            || mCurrentMediaStatus == QMediaPlayer::NoMedia) ? 0
                                                             : mMediaPlayer->getDuration();
}

qint64 QAndroidMediaPlayerControl::position() const
{
    if (!mMediaPlayerReady)
        return mPendingPosition < 0 ? 0 : mPendingPosition;

    return mMediaPlayer->getCurrentPosition();
}

void QAndroidMediaPlayerControl::setPosition(qint64 position)
{
    if (!mSeekable)
        return;

    const int seekPosition = (position > INT_MAX) ? INT_MAX : position;

    if (!mMediaPlayerReady) {
        mPendingPosition = seekPosition;
        Q_EMIT positionChanged(seekPosition);
        return;
    }

    mMediaPlayer->seekTo(seekPosition);
    mPendingPosition = -1;
}

int QAndroidMediaPlayerControl::volume() const
{
    return mMediaPlayer->volume();
}

void QAndroidMediaPlayerControl::setVolume(int volume)
{
    mMediaPlayer->setVolume(volume);
    Q_EMIT volumeChanged(volume);
}

bool QAndroidMediaPlayerControl::isMuted() const
{
    return mMediaPlayer->isMuted();
}

void QAndroidMediaPlayerControl::setMuted(bool muted)
{
    mMediaPlayer->setMuted(muted);
    Q_EMIT mutedChanged(muted);
}

int QAndroidMediaPlayerControl::bufferStatus() const
{
    return mBufferPercent;
}

bool QAndroidMediaPlayerControl::isAudioAvailable() const
{
    return mAudioAvailable;
}

bool QAndroidMediaPlayerControl::isVideoAvailable() const
{
    return mVideoAvailable;
}

bool QAndroidMediaPlayerControl::isSeekable() const
{
    return mSeekable;
}

QMediaTimeRange QAndroidMediaPlayerControl::availablePlaybackRanges() const
{
    return mAvailablePlaybackRange;
}

void QAndroidMediaPlayerControl::updateAvailablePlaybackRanges()
{
    if (mBuffering) {
        const qint64 pos = position();
        const qint64 end = (duration() / 100) * mBufferPercent;
        mAvailablePlaybackRange.addInterval(pos, end);
    } else if (mSeekable) {
        mAvailablePlaybackRange = QMediaTimeRange(0, duration());
    } else {
        mAvailablePlaybackRange = QMediaTimeRange();
    }

    Q_EMIT availablePlaybackRangesChanged(mAvailablePlaybackRange);
}

qreal QAndroidMediaPlayerControl::playbackRate() const
{
    return 1.0f;
}

void QAndroidMediaPlayerControl::setPlaybackRate(qreal rate)
{
    Q_UNUSED(rate);
}

QMediaContent QAndroidMediaPlayerControl::media() const
{
    return mMediaContent;
}

const QIODevice *QAndroidMediaPlayerControl::mediaStream() const
{
    return mMediaStream;
}

void QAndroidMediaPlayerControl::setMedia(const QMediaContent &mediaContent,
                                          QIODevice *stream)
{
    mMediaContent = mediaContent;
    mMediaStream = stream;

    if (mVideoOutput && !mMediaPlayer->display()) {
        // if a video output is set but the video texture is not ready, delay loading the media
        // since it can cause problems on some hardware
        mPendingSetMedia = true;
        return;
    }

    const QString uri = mediaContent.canonicalUrl().toString();

    if (!uri.isEmpty())
        mMediaPlayer->setDataSource(uri);
    else
        setMediaStatus(QMediaPlayer::NoMedia);

    Q_EMIT mediaChanged(mMediaContent);

    resetBufferingProgress();

    // reset some properties
    setAudioAvailable(false);
    setVideoAvailable(false);
    setSeekable(true);
}

void QAndroidMediaPlayerControl::setVideoOutput(QAndroidVideoOutput *videoOutput)
{
    if (mVideoOutput)
        mVideoOutput->stop();

    mVideoOutput = videoOutput;

    if (mVideoOutput && !mMediaPlayer->display()) {
        if (mVideoOutput->isTextureReady())
            mMediaPlayer->setDisplay(mVideoOutput->surfaceHolder());
        else
            mVideoOutput->setTextureReadyCallback(textureReadyCallback, this);
    }
}

void QAndroidMediaPlayerControl::play()
{
    if (!mMediaPlayerReady) {
        mPendingState = QMediaPlayer::PlayingState;
        if (mCurrentState == QMediaPlayer::StoppedState
            && !mMediaContent.isNull()
            && mCurrentMediaStatus != QMediaPlayer::LoadingMedia
            && !mPendingSetMedia) {
            setMedia(mMediaContent, 0);
        }
        return;
    }

    mMediaPlayer->play();
    setState(QMediaPlayer::PlayingState);
}

void QAndroidMediaPlayerControl::pause()
{
    if (!mMediaPlayerReady) {
        mPendingState = QMediaPlayer::PausedState;
        return;
    }

    mMediaPlayer->pause();
    setState(QMediaPlayer::PausedState);
}

void QAndroidMediaPlayerControl::stop()
{
    mMediaPlayer->stop();
    setState(QMediaPlayer::StoppedState);
}

void QAndroidMediaPlayerControl::onInfo(qint32 what, qint32 extra)
{
    Q_UNUSED(extra);
    switch (what) {
    case JMediaPlayer::MEDIA_INFO_UNKNOWN:
        break;
    case JMediaPlayer::MEDIA_INFO_VIDEO_TRACK_LAGGING:
        // IGNORE
        break;
    case JMediaPlayer::MEDIA_INFO_VIDEO_RENDERING_START:
        break;
    case JMediaPlayer::MEDIA_INFO_BUFFERING_START:
        mPendingState = mCurrentState;
        setState(QMediaPlayer::PausedState);
        setMediaStatus(QMediaPlayer::StalledMedia);
        break;
    case JMediaPlayer::MEDIA_INFO_BUFFERING_END:
        setMediaStatus(mBufferPercent == 100 ? QMediaPlayer::BufferedMedia : QMediaPlayer::BufferingMedia);
        flushPendingStates();
        break;
    case JMediaPlayer::MEDIA_INFO_BAD_INTERLEAVING:
        break;
    case JMediaPlayer::MEDIA_INFO_NOT_SEEKABLE:
        setSeekable(false);
        break;
    case JMediaPlayer::MEDIA_INFO_METADATA_UPDATE:
        Q_EMIT metaDataUpdated();
        break;
    }
}

void QAndroidMediaPlayerControl::onMediaPlayerInfo(qint32 what, qint32 extra)
{
    switch (what) {
    case JMediaPlayer::MEDIA_PLAYER_INVALID_STATE:
        setError(what, QStringLiteral("Error: Invalid state"));
        break;
    case JMediaPlayer::MEDIA_PLAYER_PREPARING:
        setMediaStatus(QMediaPlayer::LoadingMedia);
        setState(QMediaPlayer::StoppedState);
        break;
    case JMediaPlayer::MEDIA_PLAYER_READY:
        if (mBuffering) {
            setMediaStatus(mBufferPercent == 100 ? QMediaPlayer::BufferedMedia
                                                 : QMediaPlayer::BufferingMedia);
        } else {
            setMediaStatus(QMediaPlayer::LoadedMedia);
            mBufferPercent = 100;
            Q_EMIT bufferStatusChanged(mBufferPercent);
            updateAvailablePlaybackRanges();
        }
        setAudioAvailable(true);
        mMediaPlayerReady = true;
        flushPendingStates();
        break;
    case JMediaPlayer::MEDIA_PLAYER_DURATION:
        Q_EMIT durationChanged(extra);
        break;
    case JMediaPlayer::MEDIA_PLAYER_PROGRESS:
        Q_EMIT positionChanged(extra);
        break;
    case JMediaPlayer::MEDIA_PLAYER_FINISHED:
        setState(QMediaPlayer::StoppedState);
        setMediaStatus(QMediaPlayer::EndOfMedia);
        break;
    }
}

void QAndroidMediaPlayerControl::onError(qint32 what, qint32 extra)
{
    QString errorString;
    QMediaPlayer::Error error = QMediaPlayer::ResourceError;

    switch (what) {
    case JMediaPlayer::MEDIA_ERROR_UNKNOWN:
        errorString = QLatin1String("Error:");
        break;
    case JMediaPlayer::MEDIA_ERROR_SERVER_DIED:
        errorString = QLatin1String("Error: Server died");
        error = QMediaPlayer::ServiceMissingError;
        break;
    }

    switch (extra) {
    case JMediaPlayer::MEDIA_ERROR_IO: // Network OR file error
        errorString += QLatin1String(" (I/O operation failed)");
        error = QMediaPlayer::NetworkError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case JMediaPlayer::MEDIA_ERROR_MALFORMED:
        errorString += QLatin1String(" (Malformed bitstream)");
        error = QMediaPlayer::FormatError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case JMediaPlayer::MEDIA_ERROR_UNSUPPORTED:
        errorString += QLatin1String(" (Unsupported media)");
        error = QMediaPlayer::FormatError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    case JMediaPlayer::MEDIA_ERROR_TIMED_OUT:
        errorString += QLatin1String(" (Timed out)");
        break;
    case JMediaPlayer::MEDIA_ERROR_NOT_VALID_FOR_PROGRESSIVE_PLAYBACK:
        errorString += QLatin1String(" (Unable to start progressive playback')");
        error = QMediaPlayer::FormatError;
        setMediaStatus(QMediaPlayer::InvalidMedia);
        break;
    }

    setError(error, errorString);
}

void QAndroidMediaPlayerControl::onBufferChanged(qint32 percent)
{
    mBuffering = true;
    mBufferPercent = percent;
    Q_EMIT bufferStatusChanged(mBufferPercent);

    updateAvailablePlaybackRanges();

    if (mBufferPercent == 100)
        setMediaStatus(QMediaPlayer::BufferedMedia);
}

void QAndroidMediaPlayerControl::onVideoSizeChanged(qint32 width, qint32 height)
{
    QSize newSize(width, height);

    if (width == 0 || height == 0 || newSize == mVideoSize)
        return;

    setVideoAvailable(true);
    mVideoSize = newSize;

    if (mVideoOutput)
        mVideoOutput->setVideoSize(mVideoSize);
}

void QAndroidMediaPlayerControl::onSurfaceTextureReady()
{
    if (!mMediaPlayer->display() && mVideoOutput) {
        mMediaPlayer->setDisplay(mVideoOutput->surfaceHolder());
        flushPendingStates();
    }
}

void QAndroidMediaPlayerControl::setState(QMediaPlayer::State state)
{
    if (mCurrentState == state)
        return;

    if (state == QMediaPlayer::StoppedState) {
        if (mVideoOutput)
            mVideoOutput->stop();
        resetBufferingProgress();
        mMediaPlayerReady = false;
        mPendingPosition = -1;
        Q_EMIT positionChanged(0);
    }

    mCurrentState = state;
    Q_EMIT stateChanged(mCurrentState);
}

void QAndroidMediaPlayerControl::setMediaStatus(QMediaPlayer::MediaStatus status)
{
    if (mCurrentMediaStatus == status)
        return;

    if (status == QMediaPlayer::NoMedia || status == QMediaPlayer::InvalidMedia)
        Q_EMIT durationChanged(0);

    mCurrentMediaStatus = status;
    Q_EMIT mediaStatusChanged(mCurrentMediaStatus);
}

void QAndroidMediaPlayerControl::setError(int error,
                                          const QString &errorString)
{
    setState(QMediaPlayer::StoppedState);
    Q_EMIT QMediaPlayerControl::error(error, errorString);
}

void QAndroidMediaPlayerControl::setSeekable(bool seekable)
{
    if (mSeekable == seekable)
        return;

    mSeekable = seekable;
    Q_EMIT seekableChanged(mSeekable);
}

void QAndroidMediaPlayerControl::setAudioAvailable(bool available)
{
    if (mAudioAvailable == available)
        return;

    mAudioAvailable = available;
    Q_EMIT audioAvailableChanged(mAudioAvailable);
}

void QAndroidMediaPlayerControl::setVideoAvailable(bool available)
{
    if (mVideoAvailable == available)
        return;

    if (!available)
        mVideoSize = QSize();

    mVideoAvailable = available;
    Q_EMIT videoAvailableChanged(mVideoAvailable);
}

void QAndroidMediaPlayerControl::resetBufferingProgress()
{
    mBuffering = false;
    mBufferPercent = 0;
    mAvailablePlaybackRange = QMediaTimeRange();
    Q_EMIT bufferStatusChanged(mBufferPercent);
}

void QAndroidMediaPlayerControl::flushPendingStates()
{
    if (mPendingSetMedia) {
        setMedia(mMediaContent, 0);
        mPendingSetMedia = false;
        return;
    }

    switch (mPendingState) {
    case QMediaPlayer::PlayingState:
        if (mPendingPosition > -1)
            setPosition(mPendingPosition);
        play();
        break;
    case QMediaPlayer::PausedState:
        pause();
        break;
    case QMediaPlayer::StoppedState:
        stop();
        break;
    }
}

QT_END_NAMESPACE
