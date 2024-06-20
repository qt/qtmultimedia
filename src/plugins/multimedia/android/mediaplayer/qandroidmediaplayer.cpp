// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidmediaplayer_p.h"
#include "androidmediaplayer_p.h"
#include "qandroidvideooutput_p.h"
#include "qandroidmetadata_p.h"
#include "qandroidaudiooutput_p.h"
#include "qaudiooutput.h"

#include <private/qplatformvideosink_p.h>
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcMediaPlayer, "qt.multimedia.mediaplayer.android");

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
    // Set seekable to True by default. It changes if MEDIA_INFO_NOT_SEEKABLE is received
    seekableChanged(true);
    connect(mMediaPlayer, &AndroidMediaPlayer::bufferingChanged, this,
            &QAndroidMediaPlayer::onBufferingChanged);
    connect(mMediaPlayer, &AndroidMediaPlayer::info, this, &QAndroidMediaPlayer::onInfo);
    connect(mMediaPlayer, &AndroidMediaPlayer::error, this, &QAndroidMediaPlayer::onError);
    connect(mMediaPlayer, &AndroidMediaPlayer::stateChanged, this,
            &QAndroidMediaPlayer::onStateChanged);
    connect(mMediaPlayer, &AndroidMediaPlayer::videoSizeChanged, this,
            &QAndroidMediaPlayer::onVideoSizeChanged);
    connect(mMediaPlayer, &AndroidMediaPlayer::progressChanged, this,
            &QAndroidMediaPlayer::positionChanged);
    connect(mMediaPlayer, &AndroidMediaPlayer::durationChanged, this,
            &QAndroidMediaPlayer::durationChanged);
    connect(mMediaPlayer, &AndroidMediaPlayer::tracksInfoChanged, this,
            &QAndroidMediaPlayer::updateTrackInfo);
}

QAndroidMediaPlayer::~QAndroidMediaPlayer()
{
    if (m_videoSink)
        disconnect(m_videoSink->platformVideoSink(), nullptr, this, nullptr);

    mMediaPlayer->disconnect();
    mMediaPlayer->release();
    delete mMediaPlayer;
}

qint64 QAndroidMediaPlayer::duration() const
{
    if (mediaStatus() == QMediaPlayer::NoMedia)
        return 0;

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

    qint64 currentPosition = mMediaPlayer->getCurrentPosition();
    if (seekPosition == currentPosition) {
        // update position - will send a new frame of this position
        // for consistency with other platforms
        mMediaPlayer->seekTo(seekPosition);
        return;
    }
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
    return mBufferFilled ? 1. : mBufferPercent;
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
    return mCurrentPlaybackRate;
}

void QAndroidMediaPlayer::setPlaybackRate(qreal rate)
{
    if (mState != AndroidMediaPlayer::Started) {
        // If video isn't playing, changing speed rate may start it automatically
        // It need to be postponed
        if (mCurrentPlaybackRate != rate) {
            mCurrentPlaybackRate = rate;
            mHasPendingPlaybackRate = true;
            Q_EMIT playbackRateChanged(rate);
        }
        return;
    }

    if (mMediaPlayer->setPlaybackRate(rate)) {
        mCurrentPlaybackRate = rate;
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

        if (mVideoOutput &&
                        (mMediaPlayer->display() == 0 || mVideoOutput->shouldTextureBeUpdated()))
            mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture());
        mMediaPlayer->setDataSource(QNetworkRequest(mediaContent));
        mMediaPlayer->prepareAsync();

        if (!mReloadingMedia)
            setMediaStatus(QMediaPlayer::LoadingMedia);
    }

    resetBufferingProgress();

    mReloadingMedia = false;
}

void QAndroidMediaPlayer::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;

    if (m_videoSink)
        disconnect(m_videoSink->platformVideoSink(), nullptr, this, nullptr);

    m_videoSink = sink;

    if (!m_videoSink) {
        return;
    }

    if (mVideoOutput) {
        delete mVideoOutput;
        mVideoOutput = nullptr;
        mMediaPlayer->setDisplay(nullptr);
    }

    mVideoOutput = new QAndroidTextureVideoOutput(sink, this);
    connect(mVideoOutput, &QAndroidTextureVideoOutput::readyChanged, this,
            &QAndroidMediaPlayer::onVideoOutputReady);
    connect(mMediaPlayer, &AndroidMediaPlayer::timedTextChanged, mVideoOutput,
            &QAndroidTextureVideoOutput::setSubtitle);

    if (mVideoOutput->isReady())
        mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture());

    connect(m_videoSink->platformVideoSink(), &QPlatformVideoSink::rhiChanged, this, [&]()
            { mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture()); });
}

void QAndroidMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    if (m_audioOutput)
        m_audioOutput->q->disconnect(this);
    m_audioOutput = static_cast<QAndroidAudioOutput *>(output);
    if (m_audioOutput) {
        connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this, &QAndroidMediaPlayer::updateAudioDevice);
        connect(m_audioOutput->q, &QAudioOutput::volumeChanged, this, &QAndroidMediaPlayer::setVolume);
        connect(m_audioOutput->q, &QAudioOutput::mutedChanged, this, &QAndroidMediaPlayer::setMuted);
        updateAudioDevice();
    }
}

void QAndroidMediaPlayer::updateAudioDevice()
{
    if (m_audioOutput)
        mMediaPlayer->setAudioOutput(m_audioOutput->device.id());
}

void QAndroidMediaPlayer::play()
{
    StateChangeNotifier notifier(this);

    resetCurrentLoop();

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

    if (mVideoOutput)
        mVideoOutput->start();

    updateAudioDevice();

    if (mHasPendingPlaybackRate) {
        mHasPendingPlaybackRate = false;
        if (mMediaPlayer->setPlaybackRate(mCurrentPlaybackRate))
            return;
        mCurrentPlaybackRate = mMediaPlayer->playbackRate();
        Q_EMIT playbackRateChanged(mCurrentPlaybackRate);
    }

    mMediaPlayer->play();
}

void QAndroidMediaPlayer::pause()
{
    // cannot pause without media
    if (mediaStatus() == QMediaPlayer::NoMedia)
        return;

    StateChangeNotifier notifier(this);

    stateChanged(QMediaPlayer::PausedState);

    if ((mState & (AndroidMediaPlayer::Started
                   | AndroidMediaPlayer::Paused
                   | AndroidMediaPlayer::PlaybackCompleted
                   | AndroidMediaPlayer::Prepared
                   | AndroidMediaPlayer::Stopped)) == 0) {
        mPendingState = QMediaPlayer::PausedState;
        return;
    }

    const qint64 currentPosition = mMediaPlayer->getCurrentPosition();
    setPosition(currentPosition);

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

    if (mCurrentPlaybackRate != 1.)
        // Playback rate need to by reapplied
        mHasPendingPlaybackRate = true;

    if (mVideoOutput)
        mVideoOutput->stop();

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
        errorString += mMediaContent.scheme() == QLatin1String("rtsp")
            ? QLatin1String(" (Unknown error/Insufficient resources or RTSP may not be supported)")
            : QLatin1String(" (Unknown error/Insufficient resources)");
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

    updateBufferStatus();
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
        setPosition(0);
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
        if (mediaStatus() == QMediaPlayer::EndOfMedia) {
            setPosition(0);
            setMediaStatus(QMediaPlayer::BufferedMedia);
        } else {
            Q_EMIT positionChanged(position());
        }
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
        if (doLoop()) {
            setPosition(0);
            mMediaPlayer->play();
            break;
        }
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
        }
    }
}

int QAndroidMediaPlayer::trackCount(TrackType trackType)
{
    if (!mTracksMetadata.contains(trackType))
        return -1;

    auto tracks = mTracksMetadata.value(trackType);
    return tracks.count();
}

QMediaMetaData QAndroidMediaPlayer::trackMetaData(TrackType trackType, int streamNumber)
{
    if (!mTracksMetadata.contains(trackType))
        return QMediaMetaData();

    auto tracks = mTracksMetadata.value(trackType);
    if (tracks.count() < streamNumber)
        return QMediaMetaData();

    QAndroidMetaData trackInfo = tracks.at(streamNumber);
    return static_cast<QMediaMetaData>(trackInfo);
}

QPlatformMediaPlayer::TrackType convertTrackType(AndroidMediaPlayer::TrackType type)
{
    switch (type) {
    case AndroidMediaPlayer::TrackType::Video:
        return QPlatformMediaPlayer::TrackType::VideoStream;
    case AndroidMediaPlayer::TrackType::Audio:
        return QPlatformMediaPlayer::TrackType::AudioStream;
    case AndroidMediaPlayer::TrackType::TimedText:
        return QPlatformMediaPlayer::TrackType::SubtitleStream;
    case AndroidMediaPlayer::TrackType::Subtitle:
        return QPlatformMediaPlayer::TrackType::SubtitleStream;
    case AndroidMediaPlayer::TrackType::Unknown:
    case AndroidMediaPlayer::TrackType::Metadata:
        return QPlatformMediaPlayer::TrackType::NTrackTypes;
    }

    return QPlatformMediaPlayer::TrackType::NTrackTypes;
}

int QAndroidMediaPlayer::convertTrackNumber(int androidTrackNumber)
{
    int trackNumber = androidTrackNumber;

    int videoTrackCount = trackCount(QPlatformMediaPlayer::TrackType::VideoStream);
    if (trackNumber <= videoTrackCount)
        return trackNumber;

    trackNumber = trackNumber - videoTrackCount;

    int audioTrackCount = trackCount(QPlatformMediaPlayer::TrackType::AudioStream);
    if (trackNumber <= audioTrackCount)
        return trackNumber;

    trackNumber = trackNumber - audioTrackCount;

    auto subtitleTracks = mTracksMetadata.value(QPlatformMediaPlayer::TrackType::SubtitleStream);
    int timedTextCount = 0;
    int subtitleTextCount = 0;
    for (const auto &track : subtitleTracks) {
        if (track.androidTrackType() == 3) // 3 == TimedText
            timedTextCount++;

        if (track.androidTrackType() == 4) // 4 == Subtitle
            subtitleTextCount++;
    }

    if (trackNumber <= timedTextCount)
        return trackNumber;

    trackNumber = trackNumber - timedTextCount;

    if (trackNumber <= subtitleTextCount)
        return trackNumber;

    return -1;
}

int QAndroidMediaPlayer::activeTrack(TrackType trackType)
{
    int androidTrackNumber = -1;

    switch (trackType) {
    case QPlatformMediaPlayer::TrackType::VideoStream: {
        if (!mIsVideoTrackEnabled)
            return -1;
        androidTrackNumber = mMediaPlayer->activeTrack(AndroidMediaPlayer::TrackType::Video);
    }
    case QPlatformMediaPlayer::TrackType::AudioStream: {
        if (!mIsAudioTrackEnabled)
            return -1;

        androidTrackNumber = mMediaPlayer->activeTrack(AndroidMediaPlayer::TrackType::Audio);
    }
    case QPlatformMediaPlayer::TrackType::SubtitleStream: {
        int timedTextSelectedTrack =
                mMediaPlayer->activeTrack(AndroidMediaPlayer::TrackType::TimedText);

        if (timedTextSelectedTrack > -1) {
            androidTrackNumber = timedTextSelectedTrack;
            break;
        }

        int subtitleSelectedTrack =
                mMediaPlayer->activeTrack(AndroidMediaPlayer::TrackType::Subtitle);
        if (subtitleSelectedTrack > -1) {
            androidTrackNumber = subtitleSelectedTrack;
            break;
        }

        return -1;
    }
    case QPlatformMediaPlayer::TrackType::NTrackTypes:
        return -1;
    }

    return convertTrackNumber(androidTrackNumber);
}

void QAndroidMediaPlayer::disableTrack(TrackType trackType)
{
    const auto track = activeTrack(trackType);

    switch (trackType) {
    case VideoStream: {
        if (track > -1) {
            mMediaPlayer->setDisplay(nullptr);
            mIsVideoTrackEnabled = false;
        }
        break;
    }
    case AudioStream: {
        if (track > -1) {
            mMediaPlayer->setMuted(true);
            mMediaPlayer->blockAudio();
            mIsAudioTrackEnabled = false;
        }
        break;
    }
    case SubtitleStream: {
        // subtitles and timedtext tracks can be selected at the same time so deselect both
        int subtitleSelectedTrack =
                mMediaPlayer->activeTrack(AndroidMediaPlayer::TrackType::Subtitle);
        if (subtitleSelectedTrack > -1)
            mMediaPlayer->deselectTrack(subtitleSelectedTrack);

        int timedTextSelectedTrack =
                mMediaPlayer->activeTrack(AndroidMediaPlayer::TrackType::TimedText);
        if (timedTextSelectedTrack > -1)
            mMediaPlayer->deselectTrack(timedTextSelectedTrack);

        break;
    }
    case NTrackTypes:
        break;
    }
}

void QAndroidMediaPlayer::setActiveTrack(TrackType trackType, int streamNumber)
{

    if (!mTracksMetadata.contains(trackType)) {
        qCWarning(lcMediaPlayer)
                << "Trying to set a active track which type has no available tracks.";
        return;
    }

    const auto &tracks = mTracksMetadata.value(trackType);
    if (streamNumber > tracks.count()) {
        qCWarning(lcMediaPlayer) << "Trying to set a active track that does not exist.";
        return;
    }

    // in case of < 0 deselect tracktype
    if (streamNumber < 0) {
        disableTrack(trackType);
        return;
    }

    const auto currentTrack = activeTrack(trackType);
    if (streamNumber == currentTrack) {
        return;
    }

    if (trackType == TrackType::VideoStream && !mIsVideoTrackEnabled) {
        // enable video stream
        mMediaPlayer->setDisplay(mVideoOutput->surfaceTexture());
        mIsVideoTrackEnabled = true;
    }

    if (trackType == TrackType::AudioStream && !mIsAudioTrackEnabled) {
        // enable audio stream
        mMediaPlayer->unblockAudio();
        mMediaPlayer->setMuted(false);
        mIsAudioTrackEnabled = true;
    }

    if (trackType == TrackType::SubtitleStream) {
        // subtitles and timedtext tracks can be selected at the same time so deselect both before
        // selecting a new one
        disableTrack(TrackType::SubtitleStream);
    }

    const auto &trackInfo = tracks.at(streamNumber);
    const auto &trackNumber = trackInfo.androidTrackNumber();
    mMediaPlayer->selectTrack(trackNumber);

    emit activeTracksChanged();
}

void QAndroidMediaPlayer::positionChanged(qint64 position)
{
    QPlatformMediaPlayer::positionChanged(position);
}

void QAndroidMediaPlayer::durationChanged(qint64 duration)
{
    QPlatformMediaPlayer::durationChanged(duration);
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

    if (status == QMediaPlayer::NoMedia || status == QMediaPlayer::InvalidMedia) {
        Q_EMIT durationChanged(0);
        Q_EMIT metaDataChanged();
        setAudioAvailable(false);
        setVideoAvailable(false);
    }

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
    const auto &status = mediaStatus();
    bool bufferFilled = (status == QMediaPlayer::BufferedMedia || status == QMediaPlayer::BufferingMedia);

    if (mBufferFilled != bufferFilled)
        mBufferFilled = bufferFilled;

    emit bufferProgressChanged(bufferProgress());
}

void QAndroidMediaPlayer::updateTrackInfo()
{
    const auto &androidTracksInfo = mMediaPlayer->tracksInfo();

    // prepare mTracksMetadata
    mTracksMetadata[TrackType::VideoStream] = QList<QAndroidMetaData>();
    mTracksMetadata[TrackType::AudioStream] = QList<QAndroidMetaData>();
    mTracksMetadata[TrackType::SubtitleStream] = QList<QAndroidMetaData>();
    mTracksMetadata[TrackType::NTrackTypes] = QList<QAndroidMetaData>();

    for (const auto &androidTrackInfo : androidTracksInfo) {

        const auto &mediaPlayerType = convertTrackType(androidTrackInfo.trackType);
        auto &tracks = mTracksMetadata[mediaPlayerType];

        const QAndroidMetaData metadata(mediaPlayerType, androidTrackInfo.trackType,
                                        androidTrackInfo.trackNumber, androidTrackInfo.mimeType,
                                        androidTrackInfo.language);
        tracks.append(metadata);
    }

    emit tracksChanged();
}

QT_END_NAMESPACE

#include "moc_qandroidmediaplayer_p.cpp"
