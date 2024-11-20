// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreamermediaplayer_p.h>

#include <audio/qgstreameraudiodevice_p.h>
#include <common/qglist_helper_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgst_discoverer_p.h>
#include <common/qgst_play_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreameraudiooutput_p.h>
#include <common/qgstreamermessage_p.h>
#include <common/qgstreamermetadata_p.h>
#include <common/qgstreamervideooutput_p.h>
#include <common/qgstreamervideosink_p.h>
#include <uri_handler/qgstreamer_qiodevice_handler_p.h>
#include <qgstreamerformatinfo_p.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtCore/qdebug.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtCore/private/quniquehandle_p.h>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

static Q_LOGGING_CATEGORY(qLcMediaPlayer, "qt.multimedia.player")

QT_BEGIN_NAMESPACE

bool QGstreamerMediaPlayer::discover(const QUrl &url)
{
    QGst::QGstDiscoverer discoverer;

    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto discoveryResult = discoverer.discover(url);
    if (discoveryResult) {
        m_trackMetaData.fill({});
        seekableChanged(discoveryResult->isSeekable);
        if (discoveryResult->duration)
            m_duration = round<milliseconds>(*discoveryResult->duration);
        else
            m_duration = 0ms;
        durationChanged(m_duration);

        m_metaData = QGst::toContainerMetadata(*discoveryResult);

        videoAvailableChanged(!discoveryResult->videoStreams.empty());
        audioAvailableChanged(!discoveryResult->audioStreams.empty());

        m_nativeSize.clear();
        for (const auto &videoInfo : discoveryResult->videoStreams) {
            m_trackMetaData[0].emplace_back(QGst::toStreamMetadata(videoInfo));
            QGstStructureView structure = videoInfo.caps.at(0);
            m_nativeSize.emplace_back(structure.nativeSize());
        }
        for (const auto &audioInfo : discoveryResult->audioStreams)
            m_trackMetaData[1].emplace_back(QGst::toStreamMetadata(audioInfo));
        for (const auto &subtitleInfo : discoveryResult->subtitleStreams)
            m_trackMetaData[2].emplace_back(QGst::toStreamMetadata(subtitleInfo));

        using Key = QMediaMetaData::Key;
        auto copyKeysToRootMetadata = [&](const QMediaMetaData &reference, QSpan<const Key> keys) {
            for (QMediaMetaData::Key key : keys) {
                QVariant referenceValue = reference.value(key);
                if (referenceValue.isValid())
                    m_metaData.insert(key, referenceValue);
            }
        };

        // FIXME: we duplicate some metadata for the first audio / video track
        // in future we will want to use e.g. the currently selected track
        if (!m_trackMetaData[0].empty())
            copyKeysToRootMetadata(m_trackMetaData[0].front(),
                                   {
                                           Key::HasHdrContent,
                                           Key::Orientation,
                                           Key::Resolution,
                                           Key::VideoBitRate,
                                           Key::VideoCodec,
                                           Key::VideoFrameRate,
                                   });

        if (!m_trackMetaData[1].empty())
            copyKeysToRootMetadata(m_trackMetaData[1].front(),
                                   {
                                           Key::AudioBitRate,
                                           Key::AudioCodec,
                                   });

        if (!m_url.isEmpty())
            m_metaData.insert(QMediaMetaData::Key::Url, m_url);

        qCDebug(qLcMediaPlayer) << "metadata:" << m_metaData;
        qCDebug(qLcMediaPlayer) << "video metadata:" << m_trackMetaData[0];
        qCDebug(qLcMediaPlayer) << "audio metadata:" << m_trackMetaData[1];
        qCDebug(qLcMediaPlayer) << "subtitle metadata:" << m_trackMetaData[2];

        metaDataChanged();
        tracksChanged();
        m_activeTrack = {
            isVideoAvailable() ? 0 : -1,
            isAudioAvailable() ? 0 : -1,
            -1,
        };
        updateVideoTrackEnabled();
        updateAudioTrackEnabled();
        updateNativeSizeOnVideoOutput();
    }

    return bool(discoveryResult);
}

void QGstreamerMediaPlayer::resetStateForEmptyOrInvalidMedia()
{
    using namespace std::chrono_literals;
    m_nativeSize.clear();

    bool metadataNeedsSignal = !m_metaData.isEmpty();
    bool tracksNeedsSignal =
            std::any_of(m_trackMetaData.begin(), m_trackMetaData.end(), [](const auto &container) {
        return !container.empty();
    });

    m_metaData.clear();
    m_trackMetaData.fill({});
    m_duration = 0ms;
    seekableChanged(false);

    videoAvailableChanged(false);
    audioAvailableChanged(false);

    m_activeTrack.fill(-1);

    if (metadataNeedsSignal)
        metaDataChanged();
    if (tracksNeedsSignal)
        tracksChanged();
}

void QGstreamerMediaPlayer::updateNativeSizeOnVideoOutput()
{
    int activeVideoTrack = activeTrack(TrackType::VideoStream);
    bool hasVideoTrack = activeVideoTrack != -1;

    QSize nativeSize = hasVideoTrack ? m_nativeSize[activeTrack(TrackType::VideoStream)] : QSize{};

    QVariant orientation = hasVideoTrack
            ? m_trackMetaData[TrackType::VideoStream][activeTrack(TrackType::VideoStream)].value(
                    QMediaMetaData::Key::Orientation)
            : QVariant{};

    if (orientation.isValid()) {
        auto rotation = orientation.value<QtVideo::Rotation>();
        gstVideoOutput->setRotation(rotation);
    }
    gstVideoOutput->setNativeSize(nativeSize);
}

void QGstreamerMediaPlayer::seekToCurrentPosition()
{
    gst_play_seek(m_gstPlay.get(), gst_play_get_position(m_gstPlay.get()));
}

void QGstreamerMediaPlayer::updateVideoTrackEnabled()
{
    bool hasTrack = m_activeTrack[TrackType::VideoStream] != -1;
    bool hasSink = gstVideoOutput->gstreamerVideoSink() != nullptr;

    gstVideoOutput->setActive(hasTrack);
    gst_play_set_video_track_enabled(m_gstPlay.get(), hasTrack && hasSink);
}

void QGstreamerMediaPlayer::updateAudioTrackEnabled()
{
    bool hasTrack = m_activeTrack[TrackType::AudioStream] != -1;
    bool hasAudioOut = gstAudioOutput;

    gst_play_set_audio_track_enabled(m_gstPlay.get(), hasTrack && hasAudioOut);
}

void QGstreamerMediaPlayer::updateBufferProgress(float newProgress)
{
    if (qFuzzyIsNull(newProgress - m_bufferProgress))
        return;

    m_bufferProgress = newProgress;
    bufferProgressChanged(m_bufferProgress);
}

void QGstreamerMediaPlayer::disconnectDecoderHandlers()
{
    auto handlers = std::initializer_list<QGObjectHandlerScopedConnection *>{ &sourceSetup };
    for (QGObjectHandlerScopedConnection *handler : handlers)
        handler->disconnect();
}

QMaybe<QPlatformMediaPlayer *> QGstreamerMediaPlayer::create(QMediaPlayer *parent)
{
    auto videoOutput = QGstreamerVideoOutput::create();
    if (!videoOutput)
        return videoOutput.error();

    return new QGstreamerMediaPlayer(videoOutput.value(), parent);
}

template <typename T>
void setSeekAccurate(T *config, gboolean accurate)
{
    gst_play_config_set_seek_accurate(config, accurate);
}

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QGstreamerVideoOutput *videoOutput,
                                             QMediaPlayer *parent)
    : QObject(parent),
      QPlatformMediaPlayer(parent),
      gstVideoOutput(videoOutput),
      m_gstPlay{
          gst_play_new(nullptr),
      },
      m_playbin{
          GST_PIPELINE_CAST(gst_play_get_pipeline(m_gstPlay.get())),
          QGstPipeline::HasRef,
      },
      m_gstPlayBus{
          QGstBusHandle{ gst_play_get_message_bus(m_gstPlay.get()), QGstBusHandle::HasRef },
      }
{
#if 1
    // LATER: remove this hack after meta-freescale decides not to pull in outdated APIs

    // QTBUG-131300: nxp deliberately reverted to an old gst-play API before the gst-play API
    // stabilized. compare:
    // https://github.com/nxp-imx/gst-plugins-bad/commit/ff04fa9ca1b79c98e836d8cdb26ac3502dafba41
    constexpr bool useNxpWorkaround = std::is_same_v<decltype(&gst_play_config_set_seek_accurate),
                                                     void (*)(GstPlay *, gboolean)>;

    QUniqueGstStructureHandle config{
        gst_play_get_config(m_gstPlay.get()),
    };

    if constexpr (useNxpWorkaround)
        setSeekAccurate(m_gstPlay.get(), true);
    else
        setSeekAccurate(config.get(), true);

    gst_play_set_config(m_gstPlay.get(), config.release());
#else
    QUniqueGstStructureHandle config{
        gst_play_get_config(m_gstPlay.get()),
    };
    gst_play_config_set_seek_accurate(config.get(), true);
    gst_play_set_config(m_gstPlay.get(), config.release());
#endif

    gstVideoOutput->setParent(this);

    m_playbin.set("video-sink", gstVideoOutput->gstElement());
    m_playbin.set("text-sink", gstVideoOutput->gstSubtitleElement());

    m_playbin.set("audio-sink", QGstElement::createFromPipelineDescription("fakesink"));

    m_gstPlayBus.installMessageFilter(this);

    // we start without subtitles
    gst_play_set_subtitle_track_enabled(m_gstPlay.get(), false);

    sourceSetup = m_playbin.connect("source-setup", GCallback(sourceSetupCallback), this);

    m_activeTrack.fill(-1);

    // TODO: how to detect stalled media?
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    m_gstPlayBus.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    gst_bus_set_flushing(m_gstPlayBus.get(), TRUE);
    gst_play_stop(m_gstPlay.get());

    // NOTE: gst_play_stop is not sufficient, un-reffing m_gstPlay can deadlock
    m_playbin.setStateSync(GST_STATE_NULL);

    m_playbin.set("video-sink", QGstElement::createFromPipelineDescription("fakesink"));
    m_playbin.set("text-sink", QGstElement::createFromPipelineDescription("fakesink"));
    m_playbin.set("audio-sink", QGstElement::createFromPipelineDescription("fakesink"));
}

void QGstreamerMediaPlayer::updatePositionFromPipeline()
{
    using namespace std::chrono;

    positionChanged(round<milliseconds>(nanoseconds{
            gst_play_get_position(m_gstPlay.get()),
    }));
}

bool QGstreamerMediaPlayer::processBusMessage(const QGstreamerMessage &message)
{
    switch (message.type()) {
    case GST_MESSAGE_APPLICATION:
        if (gst_play_is_play_message(message.message()))
            return processBusMessageApplication(message);
        return false;

    default:
        qCDebug(qLcMediaPlayer) << message;

        return false;
    }

    return false;
}

bool QGstreamerMediaPlayer::processBusMessageApplication(const QGstreamerMessage &message)
{
    using namespace std::chrono;
    GstPlayMessage type;
    gst_play_message_parse_type(message.message(), &type);
    qCDebug(qLcMediaPlayer) << QGstPlayMessageAdaptor{ message };

    switch (type) {
    case GST_PLAY_MESSAGE_URI_LOADED: {
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
        return false;
    }

    case GST_PLAY_MESSAGE_POSITION_UPDATED: {
        if (state() == QMediaPlayer::PlaybackState::PlayingState) {

            constexpr bool usePayload = false;
            if constexpr (usePayload) {
                GstClockTime position;
                gst_play_message_parse_position_updated(message.message(), &position);
                positionChanged(round<milliseconds>(nanoseconds{ position }));
            } else {
                GstClockTime position = gst_play_get_position(m_gstPlay.get());
                positionChanged(round<milliseconds>(nanoseconds{ position }));
            }
        }
        return false;
    }
    case GST_PLAY_MESSAGE_DURATION_CHANGED: {
        GstClockTime duration;
        gst_play_message_parse_duration_updated(message.message(), &duration);
        milliseconds durationInMs = round<milliseconds>(nanoseconds{ duration });
        durationChanged(durationInMs);

        m_metaData.insert(QMediaMetaData::Duration, int(durationInMs.count()));
        metaDataChanged();

        return false;
    }
    case GST_PLAY_MESSAGE_BUFFERING: {
        guint percent;
        gst_play_message_parse_buffering_percent(message.message(), &percent);
        updateBufferProgress(percent * 0.01f);
        return false;
    }
    case GST_PLAY_MESSAGE_STATE_CHANGED: {
        GstPlayState state;
        gst_play_message_parse_state_changed(message.message(), &state);

        switch (state) {
        case GstPlayState::GST_PLAY_STATE_STOPPED:
            if (stateChangeToSkip) {
                qCDebug(qLcMediaPlayer) << "    skipping StoppedState transition";

                stateChangeToSkip -= 1;
                return false;
            }
            stateChanged(QMediaPlayer::StoppedState);
            updateBufferProgress(0);
            return false;

        case GstPlayState::GST_PLAY_STATE_PAUSED:
            stateChanged(QMediaPlayer::PausedState);
            mediaStatusChanged(QMediaPlayer::BufferedMedia);
            gstVideoOutput->setActive(true);
            updateBufferProgress(1);
            return false;
        case GstPlayState::GST_PLAY_STATE_BUFFERING:
            mediaStatusChanged(QMediaPlayer::BufferingMedia);
            return false;
        case GstPlayState::GST_PLAY_STATE_PLAYING:
            stateChanged(QMediaPlayer::PlayingState);
            mediaStatusChanged(QMediaPlayer::BufferedMedia);
            gstVideoOutput->setActive(true);
            updateBufferProgress(1);

            return false;
        default:
            return false;
        }
    }
    case GST_PLAY_MESSAGE_MEDIA_INFO_UPDATED: {
        using namespace QGstPlaySupport;

        QUniqueGstPlayMediaInfoHandle info{};
        gst_play_message_parse_media_info_updated(message.message(), &info);

        seekableChanged(gst_play_media_info_is_seekable(info.get()));

        const gchar *title = gst_play_media_info_get_title(info.get());
        m_metaData.insert(QMediaMetaData::Title, QString::fromUtf8(title));

        metaDataChanged();
        tracksChanged();

        return false;
    }
    case GST_PLAY_MESSAGE_END_OF_STREAM: {
        if (doLoop()) {
            positionChanged(m_duration);
            qCDebug(qLcMediaPlayer) << "EOS: restarting loop";
            gst_play_play(m_gstPlay.get());
            positionChanged(0ms);

            // we will still get a GST_PLAY_MESSAGE_STATE_CHANGED message, which we will just ignore
            // for now
            stateChangeToSkip += 1;
        } else {
            qCDebug(qLcMediaPlayer) << "EOS: done";
            positionChanged(m_duration);
            mediaStatusChanged(QMediaPlayer::EndOfMedia);
            stateChanged(QMediaPlayer::StoppedState);
            gstVideoOutput->setActive(false);
        }

        return false;
    }
    case GST_PLAY_MESSAGE_ERROR:
    case GST_PLAY_MESSAGE_WARNING:
    case GST_PLAY_MESSAGE_VIDEO_DIMENSIONS_CHANGED:
    case GST_PLAY_MESSAGE_VOLUME_CHANGED:
    case GST_PLAY_MESSAGE_MUTE_CHANGED:
    case GST_PLAY_MESSAGE_SEEK_DONE:
        return false;

    default:
        Q_UNREACHABLE_RETURN(false);
    }
}

qint64 QGstreamerMediaPlayer::duration() const
{
    return m_duration.count();
}

bool QGstreamerMediaPlayer::hasMedia() const
{
    return !m_url.isEmpty() || m_stream;
}

bool QGstreamerMediaPlayer::hasValidMedia() const
{
    if (!hasMedia())
        return false;

    switch (mediaStatus()) {
    case QMediaPlayer::MediaStatus::NoMedia:
    case QMediaPlayer::MediaStatus::InvalidMedia:
        return false;

    default:
        return true;
    }
}

float QGstreamerMediaPlayer::bufferProgress() const
{
    return m_bufferProgress;
}

QMediaTimeRange QGstreamerMediaPlayer::availablePlaybackRanges() const
{
    return QMediaTimeRange();
}

qreal QGstreamerMediaPlayer::playbackRate() const
{
    return gst_play_get_rate(m_gstPlay.get());
}

void QGstreamerMediaPlayer::setPlaybackRate(qreal rate)
{
    if (rate == playbackRate())
        return;

    qCDebug(qLcMediaPlayer) << "gst_play_set_rate" << rate;
    gst_play_set_rate(m_gstPlay.get(), rate);
    playbackRateChanged(rate);
}

void QGstreamerMediaPlayer::setPosition(qint64 pos)
{
    std::chrono::milliseconds posInMs{ pos };

    setPosition(posInMs);
}

void QGstreamerMediaPlayer::setPosition(std::chrono::milliseconds pos)
{
    using namespace std::chrono;

    qCDebug(qLcMediaPlayer) << "gst_play_seek" << pos;
    gst_play_seek(m_gstPlay.get(), nanoseconds(pos).count());

    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    positionChanged(pos);
}

void QGstreamerMediaPlayer::play()
{
    QMediaPlayer::PlaybackState currentState = state();
    if (currentState == QMediaPlayer::PlayingState || !hasValidMedia())
        return;

    if (currentState != QMediaPlayer::PausedState)
        resetCurrentLoop();

    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        positionChanged(0);
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    }

    if (m_pendingSeek) {
        gst_play_seek(m_gstPlay.get(), m_pendingSeek->count());
        m_pendingSeek = std::nullopt;
    }

    qCDebug(qLcMediaPlayer) << "gst_play_play";
    gstVideoOutput->setActive(true);
    gst_play_play(m_gstPlay.get());
    stateChanged(QMediaPlayer::PlayingState);
}

void QGstreamerMediaPlayer::pause()
{
    if (state() == QMediaPlayer::PausedState || !hasMedia()
        || m_resourceErrorState != ResourceErrorState::NoError)
        return;

    gstVideoOutput->setActive(true);

    qCDebug(qLcMediaPlayer) << "gst_play_pause";
    gst_play_pause(m_gstPlay.get());

    mediaStatusChanged(QMediaPlayer::BufferedMedia);
    stateChanged(QMediaPlayer::PausedState);
}

void QGstreamerMediaPlayer::stop()
{
    using namespace std::chrono_literals;
    if (state() == QMediaPlayer::StoppedState) {
        if (position() != 0) {
            m_pendingSeek = 0ms;
            positionChanged(0ms);
            mediaStatusChanged(QMediaPlayer::LoadedMedia);
        }
        return;
    }

    qCDebug(qLcMediaPlayer) << "gst_play_stop";
    gstVideoOutput->setActive(false);
    gst_play_stop(m_gstPlay.get());

    stateChanged(QMediaPlayer::StoppedState);

    mediaStatusChanged(QMediaPlayer::LoadedMedia);
    positionChanged(0ms);
}

const QGstPipeline &QGstreamerMediaPlayer::pipeline() const
{
    return m_playbin;
}

bool QGstreamerMediaPlayer::canPlayQrc() const
{
    return true;
}

QUrl QGstreamerMediaPlayer::media() const
{
    return m_url;
}

const QIODevice *QGstreamerMediaPlayer::mediaStream() const
{
    return m_stream;
}

void QGstreamerMediaPlayer::sourceSetupCallback([[maybe_unused]] GstElement *playbin,
                                                GstElement *source, QGstreamerMediaPlayer *)
{
    // gst_play thread

    const gchar *typeName = g_type_name_from_instance((GTypeInstance *)source);
    qCDebug(qLcMediaPlayer) << "Setting up source:" << typeName;

    if (typeName == std::string_view("GstRTSPSrc")) {
        QGstElement s(source, QGstElement::NeedsRef);
        int latency{40};
        bool ok{false};
        int v = qEnvironmentVariableIntValue("QT_MEDIA_RTSP_LATENCY", &ok);
        if (ok)
            latency = v;
        qCDebug(qLcMediaPlayer) << "    -> setting source latency to:" << latency << "ms";
        s.set("latency", latency);

        bool drop{true};
        v = qEnvironmentVariableIntValue("QT_MEDIA_RTSP_DROP_ON_LATENCY", &ok);
        if (ok && v == 0)
            drop = false;
        qCDebug(qLcMediaPlayer) << "    -> setting drop-on-latency to:" << drop;
        s.set("drop-on-latency", drop);

        bool retrans{false};
        v = qEnvironmentVariableIntValue("QT_MEDIA_RTSP_DO_RETRANSMISSION", &ok);
        if (ok && v != 0)
            retrans = true;
        qCDebug(qLcMediaPlayer) << "    -> setting do-retransmission to:" << retrans;
        s.set("do-retransmission", retrans);
    }
}

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    using namespace Qt::Literals;
    using namespace std::chrono;
    using namespace std::chrono_literals;

    m_resourceErrorState = ResourceErrorState::NoError;
    m_url = content;
    m_stream = stream;
    QUrl streamURL;
    if (stream)
        streamURL = qGstRegisterQIODevice(stream);

    const QUrl &playUrl = stream ? streamURL : content;

    if (content.isEmpty() && !stream) {
        mediaStatusChanged(QMediaPlayer::NoMedia);
        resetStateForEmptyOrInvalidMedia();
        return;
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);

    // LATER: discover is synchronous, but we would be way more friendly to make it asynchronous.
    bool mediaDiscovered = discover(playUrl);
    if (!mediaDiscovered) {
        m_resourceErrorState = ResourceErrorState::ErrorOccurred;
        error(QMediaPlayer::Error::ResourceError, u"Resource cannot be discovered"_s);
        mediaStatusChanged(QMediaPlayer::InvalidMedia);
        resetStateForEmptyOrInvalidMedia();
        return;
    }

    positionChanged(0ms);

    gst_play_set_uri(m_gstPlay.get(), playUrl.toEncoded().constData());
}

void QGstreamerMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    auto *gstOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstOutput)
        gstOutput->setAsync(true);

    gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstAudioOutput)
        m_playbin.set("audio-sink", gstAudioOutput->gstElement());
    else
        m_playbin.set("audio-sink", QGstElement::createFromPipelineDescription("fakesink"));
    updateAudioTrackEnabled();

    // FIXME: we need to have a gst_play API to change the sinks on the fly.
    // finishStateChange a hack to avoid assertion failures in gstreamer
    m_playbin.finishStateChange();
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
    return m_metaData;
}

void QGstreamerMediaPlayer::setVideoSink(QVideoSink *sink)
{
    auto *gstSink = sink ? static_cast<QGstreamerVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (gstSink)
        gstSink->setAsync(false);

    gstVideoOutput->setVideoSink(sink);
    updateVideoTrackEnabled();

    if (sink && state() == QMediaPlayer::PausedState) {
        // FIXME: we want to get a the existing frame, but gst_play does not have such capabilities.
        // seeking to the current position is a rather bad hack, but it's the best we can do for now
        seekToCurrentPosition();
    }
}

int QGstreamerMediaPlayer::trackCount(QPlatformMediaPlayer::TrackType type)
{
    QSpan<const QMediaMetaData> tracks = m_trackMetaData[type];
    return tracks.size();
}

QMediaMetaData QGstreamerMediaPlayer::trackMetaData(QPlatformMediaPlayer::TrackType type, int index)
{
    QSpan<const QMediaMetaData> tracks = m_trackMetaData[type];
    if (index < tracks.size())
        return tracks[index];
    return {};
}

int QGstreamerMediaPlayer::activeTrack(TrackType type)
{
    return m_activeTrack[type];
}

void QGstreamerMediaPlayer::setActiveTrack(TrackType type, int index)
{
    if (m_activeTrack[type] == index)
        return;

    int formerTrack = m_activeTrack[type];
    m_activeTrack[type] = index;

    switch (type) {
    case TrackType::VideoStream: {
        if (index != -1)
            gst_play_set_video_track(m_gstPlay.get(), index);
        updateVideoTrackEnabled();
        updateNativeSizeOnVideoOutput();
        break;
    }
    case TrackType::AudioStream: {
        if (index != -1)
            gst_play_set_audio_track(m_gstPlay.get(), index);
        updateAudioTrackEnabled();
        break;
    }
    case TrackType::SubtitleStream: {
        if (index != -1)
            gst_play_set_subtitle_track(m_gstPlay.get(), index);
        gst_play_set_subtitle_track_enabled(m_gstPlay.get(), index != -1);
        break;
    }
    default:
        Q_UNREACHABLE();
    };

    if (formerTrack != -1 && index != -1)
        // it can take several seconds for gstreamer to switch the track. so we seek to the current
        // position
        seekToCurrentPosition();
}

QT_END_NAMESPACE
