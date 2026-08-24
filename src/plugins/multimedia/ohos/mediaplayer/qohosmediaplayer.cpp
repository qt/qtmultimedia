// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosmediaplayer_p.h"

#include "qohosglobal_p.h"
#include "common/qohosvideooutput_p.h"
#include "common/qohosvideosink_p.h"

#include <private/qplatformaudiooutput_p.h>

#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qmediatimerange.h>
#include <QtMultimedia/qvideosink.h>

#include <QtCore/qfile.h>
#include <QtCore/qtimer.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qpointer.h>

#include <multimedia/player_framework/native_averrors.h>

QT_BEGIN_NAMESPACE

static constexpr int surfaceWaitTimeoutMs = 500;

namespace {

QMediaPlayer::MediaStatus mediaStatusFor(AVPlayerState state, bool atEnd)
{
    if (atEnd)
        return QMediaPlayer::EndOfMedia;

    switch (state) {
    case AV_IDLE:
        return QMediaPlayer::NoMedia;
    case AV_INITIALIZED:
        return QMediaPlayer::LoadingMedia;
    case AV_PREPARED:
    case AV_STOPPED:
        return QMediaPlayer::LoadedMedia;
    case AV_PLAYING:
    case AV_PAUSED:
        return QMediaPlayer::BufferedMedia;
    case AV_COMPLETED:
        return QMediaPlayer::EndOfMedia;
    case AV_RELEASED:
    case AV_ERROR:
        return QMediaPlayer::InvalidMedia;
    }
    return QMediaPlayer::NoMedia;
}

QMediaPlayer::PlaybackState playbackStateFor(AVPlayerState state)
{
    switch (state) {
    case AV_PLAYING:
        return QMediaPlayer::PlayingState;
    case AV_PAUSED:
        return QMediaPlayer::PausedState;
    case AV_IDLE:
    case AV_INITIALIZED:
    case AV_PREPARED:
    case AV_STOPPED:
    case AV_COMPLETED:
    case AV_RELEASED:
    case AV_ERROR:
        break;
    }
    return QMediaPlayer::StoppedState;
}

AVPlaybackSpeed playbackSpeedFor(qreal rate)
{
    // Map onto the closest discrete OH speed enum (SetPlaybackRate is API 20+).
    if (rate <= 0.1875)
        return AV_SPEED_FORWARD_0_125_X;
    if (rate <= 0.375)
        return AV_SPEED_FORWARD_0_25_X;
    if (rate <= 0.625)
        return AV_SPEED_FORWARD_0_50_X;
    if (rate <= 0.875)
        return AV_SPEED_FORWARD_0_75_X;
    if (rate < 1.125)
        return AV_SPEED_FORWARD_1_00_X;
    if (rate < 1.375)
        return AV_SPEED_FORWARD_1_25_X;
    if (rate < 1.625)
        return AV_SPEED_FORWARD_1_50_X;
    if (rate < 1.875)
        return AV_SPEED_FORWARD_1_75_X;
    if (rate < 2.5)
        return AV_SPEED_FORWARD_2_00_X;
    return AV_SPEED_FORWARD_3_00_X;
}

} // namespace

QOhosMediaPlayer::QOhosMediaPlayer(QMediaPlayer *parent)
    : QObject(parent), QPlatformMediaPlayer(parent)
{
}

QOhosMediaPlayer::~QOhosMediaPlayer()
{
    releasePlayer();
}

qint64 QOhosMediaPlayer::duration() const
{
    return m_duration;
}

qint64 QOhosMediaPlayer::position() const
{
    return m_position.load();
}

void QOhosMediaPlayer::setPosition(qint64 position)
{
    if (!m_player)
        return;
    OH_AVPlayer_Seek(m_player, static_cast<int32_t>(position), AV_SEEK_NEXT_SYNC);
}

float QOhosMediaPlayer::bufferProgress() const
{
    return m_bufferProgress;
}

bool QOhosMediaPlayer::isSeekable() const
{
    return m_seekable;
}

QMediaTimeRange QOhosMediaPlayer::availablePlaybackRanges() const
{
    if (m_duration <= 0)
        return {};
    return QMediaTimeRange{ 0, m_duration };
}

qreal QOhosMediaPlayer::playbackRate() const
{
    return m_playbackRate;
}

void QOhosMediaPlayer::setPlaybackRate(qreal rate)
{
    if (qFuzzyCompare(m_playbackRate, rate))
        return;
    m_playbackRate = rate;
    if (m_player)
        OH_AVPlayer_SetPlaybackSpeed(m_player, playbackSpeedFor(rate));
    playbackRateChanged(rate);
}

QUrl QOhosMediaPlayer::media() const
{
    return m_media;
}

const QIODevice *QOhosMediaPlayer::mediaStream() const
{
    return m_stream;
}

void QOhosMediaPlayer::setMedia(const QUrl &media, QIODevice *stream)
{
    m_media = media;
    m_stream = stream;

    if (m_player)
        OH_AVPlayer_Reset(m_player);
    clearSource();

    if (media.isEmpty()) {
        m_duration = 0;
        m_position.store(0);
        m_pendingSetMedia = false;
        mediaStatusChanged(QMediaPlayer::NoMedia);
        stateChanged(QMediaPlayer::StoppedState);
        return;
    }

    // Reject obviously bad file-like sources synchronously so the test
    // QMediaPlayer-style "invalid file" cases see ResourceError immediately
    // instead of waiting forever on the deferred surface. QMediaPlayer's
    // observable contract is "Loading -> Invalid", so emit Loading first even
    // when the failure is immediate.
    const QString scheme = media.scheme();
    if (media.isLocalFile() || scheme.isEmpty() || scheme == QLatin1String("qrc")) {
        QString openPath;
        if (media.isLocalFile())
            openPath = media.toLocalFile();
        else if (scheme == QLatin1String("qrc"))
            openPath = QLatin1Char(':') + media.path();
        else
            openPath = media.toString();
        QFile probe(openPath);
        if (!probe.open(QIODevice::ReadOnly)) {
            mediaStatusChanged(QMediaPlayer::LoadingMedia);
            setInvalidMediaWithError(QMediaPlayer::ResourceError,
                                     QStringLiteral("Cannot open '%1'").arg(openPath));
            return;
        }
    }

    m_pendingSetMedia = false;
    m_sourceApplied = false;
    m_surfaceWaitStarted = false;
    // Apply on the next event-loop cycle so a videoOutput assigned right after
    // the source is in place before we pick a prepare path.
    QMetaObject::invokeMethod(this, &QOhosMediaPlayer::tryApplyPendingSource,
                              Qt::QueuedConnection);
}

void QOhosMediaPlayer::tryApplyPendingSource()
{
    if (m_sourceApplied || m_media.isEmpty())
        return;

    // The surface can only be attached between SetSource and Prepare, so wait
    // for it. A sink with no renderer never gets an RHI: give up and prepare
    // without video rather than stall.
    if (m_videoOutput && m_videoSink && !m_videoSink->rhi()) {
        if (!m_surfaceWaitStarted) {
            m_surfaceWaitStarted = true;
            QTimer::singleShot(surfaceWaitTimeoutMs, this, [this] {
                if (!m_sourceApplied && m_pendingSetMedia && !m_media.isEmpty()) {
                    m_pendingSetMedia = false;
                    m_sourceApplied = true;
                    applyPendingSource();
                }
            });
        }
        m_pendingSetMedia = true;
        mediaStatusChanged(QMediaPlayer::LoadingMedia);
        return;
    }

    m_sourceApplied = true;
    applyPendingSource();
}

void QOhosMediaPlayer::applyPendingSource()
{
    if (!ensurePlayer()) {
        setInvalidMediaWithError(QMediaPlayer::ResourceError,
                                 QStringLiteral("Failed to create OH_AVPlayer"));
        return;
    }

    OH_AVPlayer_SetAudioRendererInfo(m_player, AUDIOSTREAM_USAGE_MOVIE);

    m_videoSurfaceAttached = false;

    // Surface the LoadingMedia status before we hand the source to OH_AVPlayer
    // so any synchronous SetURLSource/SetFDSource rejection still produces the
    // observable Loading -> Invalid transition QMediaPlayer guarantees.
    mediaStatusChanged(QMediaPlayer::LoadingMedia);

    OH_AVErrCode result = AV_ERR_OK;
    const QString scheme = m_media.scheme();
    const bool isQrc = scheme == QLatin1String("qrc");
    const bool isFileLike = m_media.isLocalFile() || scheme.isEmpty() || isQrc;
    if (isFileLike) {
        QString openPath;
        if (m_media.isLocalFile()) {
            openPath = m_media.toLocalFile();
        } else if (isQrc) {
            // QFile understands ":/path" for embedded resources.
            openPath = QLatin1Char(':') + m_media.path();
        } else {
            // Bare relative paths.
            openPath = m_media.toString();
        }
        auto file = std::make_unique<QFile>(openPath);
        if (!file->open(QIODevice::ReadOnly)) {
            setInvalidMediaWithError(QMediaPlayer::ResourceError,
                                     QStringLiteral("Cannot open '%1'").arg(openPath));
            return;
        }
        const int fd = file->handle();
        const qint64 size = file->size();
        if (fd < 0) {
            // Resource-backed QFile is mapped, not file-descriptor-backed —
            // AVPlayer needs a real fd, so spool to a temp file before handing
            // it off.
            auto temp = std::make_unique<QTemporaryFile>();
            if (!temp->open() || temp->write(file->readAll()) < 0) {
                setInvalidMediaWithError(QMediaPlayer::ResourceError,
                                         QStringLiteral("Cannot stage '%1'").arg(openPath));
                return;
            }
            temp->flush();
            temp->seek(0);
            const int tempFd = temp->handle();
            const qint64 tempSize = temp->size();
            result = OH_AVPlayer_SetFDSource(m_player, tempFd, 0, tempSize);
            m_sourceFile = std::move(temp);
        } else {
            result = OH_AVPlayer_SetFDSource(m_player, fd, 0, size);
            m_sourceFile = std::move(file);
        }
    } else {
        const QByteArray url = m_media.toString(QUrl::FullyEncoded).toUtf8();
        result = OH_AVPlayer_SetURLSource(m_player, url.constData());
    }

    if (result != AV_ERR_OK) {
        qCWarning(qLcOhosMediaPlugin) << "Source rejected, error" << int(result);
        setInvalidMediaWithError(QMediaPlayer::ResourceError,
                                 QStringLiteral("OH_AVPlayer rejected the source"));
        clearSource();
        return;
    }

    if (m_videoOutput) {
        if (auto *window = m_videoOutput->nativeWindow()) {
            if (OH_AVPlayer_SetVideoSurface(m_player, window) == AV_ERR_OK)
                m_videoSurfaceAttached = true;
            else
                qCWarning(qLcOhosMediaPlugin) << "OH_AVPlayer_SetVideoSurface failed";
        }
    }

    result = OH_AVPlayer_Prepare(m_player);
    if (result != AV_ERR_OK) {
        qCWarning(qLcOhosMediaPlugin) << "Prepare failed, error" << int(result);
        setInvalidMediaWithError(QMediaPlayer::ResourceError,
                                 QStringLiteral("OH_AVPlayer_Prepare failed"));
        clearSource();
    }
}

void QOhosMediaPlayer::play()
{
    if (!m_player) {
        m_pendingPlay = !m_media.isEmpty();
        return;
    }

    if (m_avState == AV_PREPARED || m_avState == AV_PAUSED || m_avState == AV_COMPLETED
        || m_avState == AV_STOPPED) {
        m_pendingPlay = false;
        OH_AVPlayer_Play(m_player);
    } else {
        // Prepare hasn't finished yet — defer play to AV_INFO_TYPE_STATE_CHANGE
        m_pendingPlay = true;
    }
}

void QOhosMediaPlayer::pause()
{
    m_pendingPlay = false;
    if (m_player && m_avState == AV_PLAYING)
        OH_AVPlayer_Pause(m_player);
}

void QOhosMediaPlayer::stop()
{
    m_pendingPlay = false;
    if (!m_player)
        return;
    if (m_avState == AV_PLAYING || m_avState == AV_PAUSED || m_avState == AV_PREPARED
        || m_avState == AV_COMPLETED)
        OH_AVPlayer_Stop(m_player);
}

void QOhosMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    m_audioOutput = output;
    applyVolume();
}

void QOhosMediaPlayer::setVideoSink(QVideoSink *sink)
{
    m_videoSink = sink;

    if (!sink) {
        m_videoOutput.reset();
        return;
    }

    if (!m_videoOutput) {
        m_videoOutput = std::make_unique<QOhosVideoOutput>(
                sink, QOhosVideoOutput::ContentSource::MediaPlayer, this);
        connect(m_videoOutput.get(), &QOhosVideoOutput::surfaceReady, this,
                &QOhosMediaPlayer::onVideoSurfaceReady);
    }
}

int QOhosMediaPlayer::trackCount(TrackType type)
{
    // OH_AVPlayer doesn't enumerate per-stream metadata up front; expose the
    // detected presence of audio/video as a single default track so QMediaPlayer
    // consumers can ask for activeTrack(0) and get a sensible answer.
    switch (type) {
    case VideoStream:
        return m_hasVideoTrack ? 1 : 0;
    case AudioStream:
        return m_hasAudioTrack ? 1 : 0;
    case SubtitleStream:
    case NTrackTypes:
        break;
    }
    return 0;
}

QMediaMetaData QOhosMediaPlayer::trackMetaData(TrackType type, int streamNumber)
{
    if (streamNumber != 0 || trackCount(type) == 0)
        return {};
    QMediaMetaData md;
    md.insert(QMediaMetaData::Language, QVariant::fromValue(QLocale::AnyLanguage));
    return md;
}

QMediaMetaData QOhosMediaPlayer::metaData() const
{
    QMediaMetaData md;
    if (m_duration > 0)
        md.insert(QMediaMetaData::Duration, m_duration);
    if (m_videoWidth > 0 && m_videoHeight > 0)
        md.insert(QMediaMetaData::Resolution, QSize{ m_videoWidth, m_videoHeight });
    return md;
}

int QOhosMediaPlayer::activeTrack(TrackType type)
{
    return trackCount(type) > 0 ? 0 : -1;
}

void QOhosMediaPlayer::onVideoSurfaceReady()
{
    if (m_pendingSetMedia && !m_media.isEmpty()) {
        m_pendingSetMedia = false;
        m_sourceApplied = true;
        applyPendingSource();
    }
}

void QOhosMediaPlayer::handleStateChange(AVPlayerState newState)
{
    if (m_avState == newState)
        return;
    m_avState = newState;

    // AV_IDLE is a transient state that OH_AVPlayer enters during Reset() as
    // setMedia() is replacing the source. The user-facing NoMedia status is
    // already emitted from setMedia(QUrl{}); leaking another NoMedia between a
    // LoadingMedia/Invalid transition breaks observable status sequences.
    if (newState == AV_IDLE)
        return;

    if (newState == AV_PREPARED) {
        int32_t durationMs = 0;
        if (OH_AVPlayer_GetDuration(m_player, &durationMs) == AV_ERR_OK) {
            if (m_duration != durationMs) {
                m_duration = durationMs;
                durationChanged(m_duration);
            }
        }
        m_seekable = m_duration > 0;
        seekableChanged(m_seekable);

        int32_t videoWidth = 0;
        int32_t videoHeight = 0;
        OH_AVPlayer_GetVideoWidth(m_player, &videoWidth);
        OH_AVPlayer_GetVideoHeight(m_player, &videoHeight);
        const bool hasVideo = videoWidth > 0 && videoHeight > 0;
        if (hasVideo && m_videoOutput)
            m_videoOutput->setVideoSize(QSize{ videoWidth, videoHeight });
        videoAvailableChanged(hasVideo);
        audioAvailableChanged(true);
        const bool prevHasVideo = m_hasVideoTrack;
        const bool prevHasAudio = m_hasAudioTrack;
        m_hasVideoTrack = hasVideo;
        m_hasAudioTrack = true;
        if (prevHasVideo != m_hasVideoTrack || prevHasAudio != m_hasAudioTrack) {
            tracksChanged();
            activeTracksChanged();
        }
        m_videoWidth = videoWidth;
        m_videoHeight = videoHeight;
        metaDataChanged();
    }

    mediaStatusChanged(mediaStatusFor(newState, false));
    stateChanged(playbackStateFor(newState));

    if (newState == AV_PREPARED && m_pendingPlay) {
        m_pendingPlay = false;
        OH_AVPlayer_Play(m_player);
    }
}

void QOhosMediaPlayer::handleEndOfStream()
{
    mediaStatusChanged(QMediaPlayer::EndOfMedia);
    stateChanged(QMediaPlayer::StoppedState);
}

void QOhosMediaPlayer::handleSeekDone()
{
    // Position will refresh via AV_INFO_TYPE_POSITION_UPDATE.
}

void QOhosMediaPlayer::handleResolutionChange()
{
    if (!m_player)
        return;
    int32_t width = 0;
    int32_t height = 0;
    OH_AVPlayer_GetVideoWidth(m_player, &width);
    OH_AVPlayer_GetVideoHeight(m_player, &height);
    m_videoWidth = width;
    m_videoHeight = height;
    if (m_videoOutput && width > 0 && height > 0)
        m_videoOutput->setVideoSize(QSize{ width, height });
    videoAvailableChanged(width > 0 && height > 0);
}

void QOhosMediaPlayer::handleBufferingUpdate(int bufferingPercent)
{
    const float progress = std::clamp(bufferingPercent / 100.0f, 0.0f, 1.0f);
    if (qFuzzyCompare(m_bufferProgress, progress))
        return;
    m_bufferProgress = progress;
    bufferProgressChanged(progress);
}

void QOhosMediaPlayer::handleError(int32_t errorCode, const QString &errorMsg)
{
    qCWarning(qLcOhosMediaPlugin) << "OH_AVPlayer error" << errorCode << errorMsg;
    setInvalidMediaWithError(QMediaPlayer::ResourceError,
                             errorMsg.isEmpty() ? QStringLiteral("AVPlayer error %1").arg(errorCode)
                                                : errorMsg);
}

void QOhosMediaPlayer::releasePlayer()
{
    if (!m_player)
        return;
    OH_AVPlayer_SetOnInfoCallback(m_player, nullptr, nullptr);
    OH_AVPlayer_SetOnErrorCallback(m_player, nullptr, nullptr);
    OH_AVPlayer_ReleaseSync(m_player);
    m_player = nullptr;
    clearSource();
}

void QOhosMediaPlayer::clearSource()
{
    m_sourceFile.reset();
    const bool changed = m_hasVideoTrack || m_hasAudioTrack;
    m_hasVideoTrack = false;
    m_hasAudioTrack = false;
    const bool hadMetadata = m_duration > 0 || m_videoWidth > 0 || m_videoHeight > 0;
    m_videoWidth = 0;
    m_videoHeight = 0;
    if (changed) {
        tracksChanged();
        activeTracksChanged();
    }
    if (hadMetadata)
        metaDataChanged();
}

bool QOhosMediaPlayer::ensurePlayer()
{
    if (m_player)
        return true;
    m_player = OH_AVPlayer_Create();
    if (!m_player) {
        qCWarning(qLcOhosMediaPlugin) << "OH_AVPlayer_Create failed";
        return false;
    }
    OH_AVPlayer_SetOnInfoCallback(m_player, &QOhosMediaPlayer::onInfoTrampoline, this);
    OH_AVPlayer_SetOnErrorCallback(m_player, &QOhosMediaPlayer::onErrorTrampoline, this);
    return true;
}

void QOhosMediaPlayer::applyVolume()
{
    if (!m_player)
        return;

    float volume = 1.0f;
    if (m_audioOutput)
        volume = m_audioOutput->muted ? 0.0f : m_audioOutput->volume;

    OH_AVPlayer_SetVolume(m_player, volume, volume);
}

void QOhosMediaPlayer::onInfoTrampoline(OH_AVPlayer *player, AVPlayerOnInfoType type,
                                        OH_AVFormat *body, void *userData)
{
    Q_UNUSED(body)
    Q_UNUSED(player)
    auto *self = reinterpret_cast<QOhosMediaPlayer *>(userData);
    if (!self)
        return;

    switch (type) {
    case AV_INFO_TYPE_STATE_CHANGE: {
        AVPlayerState state{ AV_IDLE };
        OH_AVPlayer_GetState(player, &state);
        QMetaObject::invokeMethod(
                self, [self, state]() { self->handleStateChange(state); }, Qt::QueuedConnection);
        break;
    }
    case AV_INFO_TYPE_POSITION_UPDATE: {
        int32_t pos = 0;
        OH_AVPlayer_GetCurrentTime(player, &pos);
        self->m_position.store(pos);
        QMetaObject::invokeMethod(
                self, [self, pos]() { self->positionChanged(qint64{ pos }); },
                Qt::QueuedConnection);
        break;
    }
    case AV_INFO_TYPE_DURATION_UPDATE: {
        int32_t dur = 0;
        OH_AVPlayer_GetDuration(player, &dur);
        QMetaObject::invokeMethod(
                self,
                [self, dur]() {
                    if (self->m_duration == dur)
                        return;
                    self->m_duration = dur;
                    self->durationChanged(qint64{ dur });
                },
                Qt::QueuedConnection);
        break;
    }
    case AV_INFO_TYPE_EOS:
        QMetaObject::invokeMethod(
                self, [self]() { self->handleEndOfStream(); }, Qt::QueuedConnection);
        break;
    case AV_INFO_TYPE_SEEKDONE:
        QMetaObject::invokeMethod(
                self, [self]() { self->handleSeekDone(); }, Qt::QueuedConnection);
        break;
    case AV_INFO_TYPE_RESOLUTION_CHANGE:
        QMetaObject::invokeMethod(
                self, [self]() { self->handleResolutionChange(); }, Qt::QueuedConnection);
        break;
    default:
        break;
    }
}

void QOhosMediaPlayer::onErrorTrampoline(OH_AVPlayer *, int32_t errorCode, const char *errorMsg,
                                         void *userData)
{
    auto *self = reinterpret_cast<QOhosMediaPlayer *>(userData);
    if (!self)
        return;
    QString message = errorMsg ? QString::fromUtf8(errorMsg) : QString{};
    QMetaObject::invokeMethod(
            self, [self, errorCode, message]() { self->handleError(errorCode, message); },
            Qt::QueuedConnection);
}

QT_END_NAMESPACE

#include "moc_qohosmediaplayer_p.cpp"
