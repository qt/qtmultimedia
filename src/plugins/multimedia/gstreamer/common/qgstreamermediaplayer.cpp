// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <common/qgstreamermediaplayer_p.h>

#include <audio/qgstreameraudiodevice_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstappsource_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreameraudiooutput_p.h>
#include <common/qgstreamermessage_p.h>
#include <common/qgstreamermetadata_p.h>
#include <common/qgstreamervideooutput_p.h>
#include <common/qgstreamervideosink_p.h>
#include <qgstreamerformatinfo_p.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/quniquehandle_p.h>

#if QT_CONFIG(gstreamer_gl)
#  include <gst/gl/gl.h>
#endif

static Q_LOGGING_CATEGORY(qLcMediaPlayer, "qt.multimedia.player")

QT_BEGIN_NAMESPACE

QGstreamerMediaPlayer::TrackSelector::TrackSelector(TrackType type, QGstElement selector)
    : inputSelector(selector), type(type)
{
    selector.set("sync-streams", true);
    selector.set("sync-mode", 1 /*clock*/);

    if (type == SubtitleStream)
        selector.set("cache-buffers", true);
}

QGstPad QGstreamerMediaPlayer::TrackSelector::createInputPad()
{
    auto pad = inputSelector.getRequestPad("sink_%u");
    tracks.append(pad);
    return pad;
}

void QGstreamerMediaPlayer::TrackSelector::removeAllInputPads()
{
    for (auto &pad : tracks)
        inputSelector.releaseRequestPad(pad);
    tracks.clear();
}

void QGstreamerMediaPlayer::TrackSelector::removeInputPad(const QGstPad &pad)
{
    inputSelector.releaseRequestPad(pad);
    tracks.removeOne(pad);
}

QGstPad QGstreamerMediaPlayer::TrackSelector::inputPad(int index)
{
    if (index >= 0 && index < tracks.count())
        return tracks[index];
    return {};
}

int QGstreamerMediaPlayer::TrackSelector::activeInputIndex() const
{
    return isConnected ? tracks.indexOf(activeInputPad()) : -1;
}

QGstPad QGstreamerMediaPlayer::TrackSelector::activeInputPad() const
{
    return isConnected ? QGstPad{ inputSelector.getGstObject("active-pad") } : QGstPad{};
}

void QGstreamerMediaPlayer::TrackSelector::setActiveInputPad(const QGstPad &input)
{
    inputSelector.set("active-pad", input);
}

int QGstreamerMediaPlayer::TrackSelector::trackCount() const
{
    return tracks.count();
}

QGstreamerMediaPlayer::TrackSelector &QGstreamerMediaPlayer::trackSelector(TrackType type)
{
    auto &ts = trackSelectors[type];
    Q_ASSERT(ts.type == type);
    return ts;
}

void QGstreamerMediaPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status != QMediaPlayer::StalledMedia)
        m_stalledMediaNotifier.stop();

    QPlatformMediaPlayer::mediaStatusChanged(status);
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
    auto handlers = std::initializer_list<QGObjectHandlerScopedConnection *>{
        &padAdded,    &padRemoved,   &sourceSetup,    &uridecodebinElementAdded,
        &unknownType, &elementAdded, &elementRemoved,
    };
    for (QGObjectHandlerScopedConnection *handler : handlers)
        handler->disconnect();

    decodeBinQueues = 0;
}

QMaybe<QPlatformMediaPlayer *> QGstreamerMediaPlayer::create(QMediaPlayer *parent)
{
    auto videoOutput = QGstreamerVideoOutput::create();
    if (!videoOutput)
        return videoOutput.error();

    static const auto error =
            qGstErrorMessageIfElementsNotAvailable("input-selector", "decodebin", "uridecodebin");
    if (error)
        return *error;

    return new QGstreamerMediaPlayer(videoOutput.value(), parent);
}

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QGstreamerVideoOutput *videoOutput,
                                             QMediaPlayer *parent)
    : QObject(parent),
      QPlatformMediaPlayer(parent),
      trackSelectors{ {
              { VideoStream,
                QGstElement::createFromFactory("input-selector", "videoInputSelector") },
              { AudioStream,
                QGstElement::createFromFactory("input-selector", "audioInputSelector") },
              { SubtitleStream,
                QGstElement::createFromFactory("input-selector", "subTitleInputSelector") },
      } },
      playerPipeline(QGstPipeline::create("playerPipeline")),
      gstVideoOutput(videoOutput)
{
    gstVideoOutput->setParent(this);

    for (auto &ts : trackSelectors)
        playerPipeline.add(ts.inputSelector);

    playerPipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    playerPipeline.installMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(this));

    QGstClockHandle systemClock{
        gst_system_clock_obtain(),
    };

    gst_pipeline_use_clock(playerPipeline.pipeline(), systemClock.get());

    connect(&positionUpdateTimer, &QTimer::timeout, this, [this] {
        updatePositionFromPipeline();
    });

    m_stalledMediaNotifier.setSingleShot(true);
    connect(&m_stalledMediaNotifier, &QTimer::timeout, this, [this] {
        mediaStatusChanged(QMediaPlayer::StalledMedia);
    });
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    playerPipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    playerPipeline.removeMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(this));
    playerPipeline.setStateSync(GST_STATE_NULL);
}

std::chrono::nanoseconds QGstreamerMediaPlayer::pipelinePosition() const
{
    if (!hasMedia())
        return {};

    Q_ASSERT(playerPipeline);
    return playerPipeline.position();
}

void QGstreamerMediaPlayer::updatePositionFromPipeline()
{
    using namespace std::chrono;

    positionChanged(round<milliseconds>(pipelinePosition()));
}

void QGstreamerMediaPlayer::updateDurationFromPipeline()
{
    std::optional<std::chrono::milliseconds> duration = playerPipeline.durationInMs();
    if (!duration)
        duration = std::chrono::milliseconds{ -1 };

    if (duration != m_duration) {
        qCDebug(qLcMediaPlayer) << "updateDurationFromPipeline" << *duration;
        m_duration = *duration;
        durationChanged(m_duration);
    }
}

qint64 QGstreamerMediaPlayer::duration() const
{
    return m_duration.count();
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
    return m_rate;
}

void QGstreamerMediaPlayer::setPlaybackRate(qreal rate)
{
    if (rate == m_rate)
        return;

    m_rate = rate;

    // workaround for workaround for
    // https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/3545
    playerPipeline.waitForAsyncStateChangeComplete();
    if (playerPipeline.state() < GST_STATE_PLAYING)
        m_pendingRate = m_rate;
    else {

        // another workaround for workaround for
        // https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/3545
        // waiting until we can query position and duration before we set the playback rate.

        playerPipeline.queryPositionAndDuration();
        playerPipeline.setPlaybackRate(m_rate);
    }
    playbackRateChanged(rate);
}

void QGstreamerMediaPlayer::setPosition(qint64 pos)
{
    std::chrono::milliseconds posInMs{ pos };
    setPosition(posInMs);
}

void QGstreamerMediaPlayer::setPosition(std::chrono::milliseconds pos)
{
    if (pos == playerPipeline.position())
        return;
    playerPipeline.finishStateChange();
    playerPipeline.setPosition(pos);
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << pos << playerPipeline.positionInMs();
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    positionChanged(pos);
}

void QGstreamerMediaPlayer::play()
{
    QMediaPlayer::PlaybackState currentState = state();
    if (currentState == QMediaPlayer::PlayingState || !hasMedia())
        return;

    if (currentState != QMediaPlayer::PausedState)
        resetCurrentLoop();

    gstVideoOutput->setActive(true);
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        playerPipeline.setPosition({});
        positionChanged(0);
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    }

    qCDebug(qLcMediaPlayer) << "play()";
    bool success = playerPipeline.setStateSync(GST_STATE_PLAYING);
    if (!success) {
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the playing state.";
        return;
    }

    // workaround for workaround for
    // https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/3545
    if (m_pendingRate) {
        playerPipeline.setPlaybackRate(*m_pendingRate, /*forceFlushingSeek=*/true);
        m_pendingRate = std::nullopt;
    } else {
        playerPipeline.flush();
    }

    positionUpdateTimer.start(100);
    stateChanged(QMediaPlayer::PlayingState);
}

void QGstreamerMediaPlayer::pause()
{
    using namespace std::chrono_literals;

    if (state() == QMediaPlayer::PausedState || !hasMedia()
        || m_resourceErrorState != ResourceErrorState::NoError)
        return;

    positionUpdateTimer.stop();

    gstVideoOutput->setActive(true);
    int ret = playerPipeline.setStateSync(GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        playerPipeline.setPosition(0ms);
        positionChanged(0);
    } else {
        updatePositionFromPipeline();
    }
    stateChanged(QMediaPlayer::PausedState);

    if (m_bufferProgress > 0 || !canTrackProgress())
        mediaStatusChanged(QMediaPlayer::BufferedMedia);
    else
        mediaStatusChanged(QMediaPlayer::BufferingMedia);
}

void QGstreamerMediaPlayer::stop()
{
    using namespace std::chrono_literals;
    if (state() == QMediaPlayer::StoppedState) {
        if (position() != 0) {
            playerPipeline.setPosition({});
            positionChanged(0ms);
            mediaStatusChanged(QMediaPlayer::LoadedMedia);
        }
        return;
    }
    stopOrEOS(false);
}

const QGstPipeline &QGstreamerMediaPlayer::pipeline() const
{
    return playerPipeline;
}

void QGstreamerMediaPlayer::stopOrEOS(bool eos)
{
    using namespace std::chrono_literals;

    positionUpdateTimer.stop();
    gstVideoOutput->setActive(false);
    bool ret = playerPipeline.setStateSync(GST_STATE_PAUSED);
    if (!ret)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";
    if (!eos) {
        playerPipeline.setPosition(0ms);
        positionChanged(0ms);
    }
    stateChanged(QMediaPlayer::StoppedState);
    if (eos)
        mediaStatusChanged(QMediaPlayer::EndOfMedia);
    else
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    m_initialBufferProgressSent = false;
    bufferProgressChanged(0.f);
}

void QGstreamerMediaPlayer::detectPipelineIsSeekable()
{
    std::optional<bool> canSeek = playerPipeline.canSeek();
    if (canSeek) {
        qCDebug(qLcMediaPlayer) << "detectPipelineIsSeekable: pipeline is seekable:" << *canSeek;
        seekableChanged(*canSeek);
    } else {
        qCWarning(qLcMediaPlayer) << "detectPipelineIsSeekable: query for seekable failed.";
        seekableChanged(false);
    }
}

QGstElement QGstreamerMediaPlayer::getSinkElementForTrackType(TrackType trackType)
{
    switch (trackType) {
    case AudioStream:
        return gstAudioOutput ? gstAudioOutput->gstElement() : QGstElement{};
    case VideoStream:
        return gstVideoOutput->gstElement();
    case SubtitleStream:
        return gstVideoOutput->gstSubtitleElement();
        break;
    default:
        Q_UNREACHABLE_RETURN(QGstElement{});
    }
}

bool QGstreamerMediaPlayer::hasMedia() const
{
    return !m_url.isEmpty() || m_stream;
}

bool QGstreamerMediaPlayer::processBusMessage(const QGstreamerMessage &message)
{
    constexpr bool traceBusMessages = true;
    if (traceBusMessages)
        qCDebug(qLcMediaPlayer) << "received bus message:" << message;

    switch (message.type()) {
    case GST_MESSAGE_TAG:
        // #### This isn't ideal. We shouldn't catch stream specific tags here, rather the global
        // ones
        return processBusMessageTags(message);

    case GST_MESSAGE_DURATION_CHANGED:
        return processBusMessageDurationChanged(message);

    case GST_MESSAGE_EOS:
        return processBusMessageEOS(message);

    case GST_MESSAGE_BUFFERING:
        return processBusMessageBuffering(message);

    case GST_MESSAGE_STATE_CHANGED:
        return processBusMessageStateChanged(message);

    case GST_MESSAGE_ERROR:
        return processBusMessageError(message);

    case GST_MESSAGE_WARNING:
        return processBusMessageWarning(message);

    case GST_MESSAGE_INFO:
        return processBusMessageInfo(message);

    case GST_MESSAGE_SEGMENT_START:
        return processBusMessageSegmentStart(message);

    case GST_MESSAGE_ELEMENT:
        return processBusMessageElement(message);

    case GST_MESSAGE_ASYNC_DONE:
        return processBusMessageAsyncDone(message);

    case GST_MESSAGE_LATENCY:
        return processBusMessageLatency(message);

    default:
//        qCDebug(qLcMediaPlayer) << "    default message handler, doing nothing";
        break;
    }

    return false;
}

bool QGstreamerMediaPlayer::processBusMessageTags(const QGstreamerMessage &message)
{
    QGstTagListHandle tagList;
    gst_message_parse_tag(message.message(), &tagList);

    qCDebug(qLcMediaPlayer) << "    Got tags: " << tagList.get();

    QMediaMetaData originalMetaData = m_metaData;
    extendMetaDataFromTagList(m_metaData, tagList);
    if (originalMetaData != m_metaData)
        metaDataChanged();

    QVariant rotation = m_metaData.value(QMediaMetaData::Orientation);
    gstVideoOutput->setRotation(rotation.value<QtVideo::Rotation>());
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageDurationChanged(const QGstreamerMessage &)
{
    if (!prerolling)
        updateDurationFromPipeline();
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageBuffering(const QGstreamerMessage &message)
{
    int progress = 0;
    gst_message_parse_buffering(message.message(), &progress);

    if (state() != QMediaPlayer::StoppedState && !prerolling) {
        if (!m_initialBufferProgressSent) {
            mediaStatusChanged(QMediaPlayer::BufferingMedia);
            m_initialBufferProgressSent = true;
        }

        if (m_bufferProgress > 0 && progress == 0) {
            m_stalledMediaNotifier.start(stalledMediaDebouncePeriod);
        } else if (progress >= 50)
            // QTBUG-124517: rethink buffering
            mediaStatusChanged(QMediaPlayer::BufferedMedia);
        else
            mediaStatusChanged(QMediaPlayer::BufferingMedia);
    }

    updateBufferProgress(progress * 0.01);
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageEOS(const QGstreamerMessage &)
{
    positionChanged(m_duration);
    if (doLoop()) {
        setPosition(0);
        return false;
    }
    stopOrEOS(true);
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageStateChanged(const QGstreamerMessage &message)
{
    if (message.source() != playerPipeline)
        return false;

    GstState oldState;
    GstState newState;
    GstState pending;

    gst_message_parse_state_changed(message.message(), &oldState, &newState, &pending);
    qCDebug(qLcMediaPlayer) << "    state changed message from"
                            << QCompactGstMessageAdaptor(message);

    switch (newState) {
    case GST_STATE_VOID_PENDING:
    case GST_STATE_NULL:
    case GST_STATE_READY:
        break;
    case GST_STATE_PAUSED: {
        if (prerolling) {
            qCDebug(qLcMediaPlayer) << "Preroll done, setting status to Loaded";
            playerPipeline.dumpGraph("playerPipelinePrerollDone");

            prerolling = false;

            updateDurationFromPipeline();

            m_metaData.insert(QMediaMetaData::Duration, duration());
            if (!m_url.isEmpty())
                m_metaData.insert(QMediaMetaData::Url, m_url);
            parseStreamsAndMetadata();
            metaDataChanged();

            tracksChanged();
            mediaStatusChanged(QMediaPlayer::LoadedMedia);

            if (state() == QMediaPlayer::PlayingState) {
                Q_ASSERT(!m_initialBufferProgressSent);

                bool immediatelySendBuffered = !canTrackProgress() || m_bufferProgress > 0;
                mediaStatusChanged(QMediaPlayer::BufferingMedia);
                m_initialBufferProgressSent = true;
                if (immediatelySendBuffered)
                    mediaStatusChanged(QMediaPlayer::BufferedMedia);
            }
        }

        break;
    }
    case GST_STATE_PLAYING: {
        if (!m_initialBufferProgressSent) {
            bool immediatelySendBuffered = !canTrackProgress() || m_bufferProgress > 0;
            mediaStatusChanged(QMediaPlayer::BufferingMedia);
            m_initialBufferProgressSent = true;
            if (immediatelySendBuffered)
                mediaStatusChanged(QMediaPlayer::BufferedMedia);
        }
        break;
    }
    }
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageError(const QGstreamerMessage &message)
{
    qCDebug(qLcMediaPlayer) << "    error" << QCompactGstMessageAdaptor(message);

    QUniqueGErrorHandle err;
    QUniqueGStringHandle debug;
    gst_message_parse_error(message.message(), &err, &debug);
    GQuark errorDomain = err.get()->domain;
    gint errorCode = err.get()->code;

    if (errorDomain == GST_STREAM_ERROR) {
        if (errorCode == GST_STREAM_ERROR_CODEC_NOT_FOUND)
            error(QMediaPlayer::FormatError, tr("Cannot play stream of type: <unknown>"));
        else {
            error(QMediaPlayer::FormatError, QString::fromUtf8(err.get()->message));
        }
    } else if (errorDomain == GST_RESOURCE_ERROR) {
        if (errorCode == GST_RESOURCE_ERROR_NOT_FOUND) {
            if (m_resourceErrorState != ResourceErrorState::ErrorReported) {
                // gstreamer seems to deliver multiple GST_RESOURCE_ERROR_NOT_FOUND events
                error(QMediaPlayer::ResourceError, QString::fromUtf8(err.get()->message));
                m_resourceErrorState = ResourceErrorState::ErrorReported;
                m_url.clear();
                m_stream = nullptr;
            }
        } else {
            error(QMediaPlayer::ResourceError, QString::fromUtf8(err.get()->message));
        }
    } else {
        playerPipeline.dumpGraph("error");
    }
    mediaStatusChanged(QMediaPlayer::InvalidMedia);
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageWarning(const QGstreamerMessage &message)
{
    qCWarning(qLcMediaPlayer) << "Warning:" << QCompactGstMessageAdaptor(message);
    playerPipeline.dumpGraph("warning");
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageInfo(const QGstreamerMessage &message)
{
    if (qLcMediaPlayer().isDebugEnabled())
        qCDebug(qLcMediaPlayer) << "Info:" << QCompactGstMessageAdaptor(message);
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageSegmentStart(const QGstreamerMessage &message)
{
    using namespace std::chrono;
    gint64 pos;
    GstFormat fmt{};
    gst_message_parse_segment_start(message.message(), &fmt, &pos);

    switch (fmt) {
    case GST_FORMAT_TIME: {
        auto posNs = std::chrono::nanoseconds{ pos };
        qCDebug(qLcMediaPlayer) << "    segment start message, updating position" << posNs;
        positionChanged(round<milliseconds>(posNs));
        break;
    }
    default: {
        qWarning() << "GST_MESSAGE_SEGMENT_START with unknown format" << fmt << pos;
        break;
    }
    }

    return false;
}

bool QGstreamerMediaPlayer::processBusMessageElement(const QGstreamerMessage &message)
{
    QGstStructureView structure(gst_message_get_structure(message.message()));
    auto type = structure.name();
    if (type == "stream-topology")
        topology = structure.clone();
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageAsyncDone(const QGstreamerMessage &)
{
    if (playerPipeline.state() >= GST_STATE_PAUSED)
        detectPipelineIsSeekable();

    return false;
}

bool QGstreamerMediaPlayer::processBusMessageLatency(const QGstreamerMessage &)
{
    playerPipeline.recalculateLatency();
    return false;
}

bool QGstreamerMediaPlayer::processSyncMessage(const QGstreamerMessage &message)
{
    // GStreamer thread!

    constexpr bool traceSyncMessages = false;
    if (traceSyncMessages)
        qCDebug(qLcMediaPlayer) << "received sync message:" << message;

    switch (message.type()) {
    case GST_MESSAGE_NEED_CONTEXT:
        return processSyncMessageNeedsContext(message);

    default:
        return false;
    }
}

bool QGstreamerMediaPlayer::processSyncMessageNeedsContext(
        [[maybe_unused]] const QGstreamerMessage &message)
{
    // GStreamer thread!

#if QT_CONFIG(gstreamer_gl)
    const gchar *type = nullptr;
    gst_message_parse_context_type(message.message(), &type);
    if (type != std::string_view{ GST_GL_DISPLAY_CONTEXT_TYPE })
        return false;

    // CHECK: accessing gstVideoOutput from the gstreamer thread does not look thread safe
    QGstreamerVideoSink *sink = gstVideoOutput->gstreamerVideoSink();
    if (!sink)
        return false;
    auto *context = sink->gstGlDisplayContext();
    if (!context)
        return false;

    gst_element_set_context(GST_ELEMENT(message.source().object()), context);
    playerPipeline.dumpGraph("need_context");
    return true;
#endif
    return false;
}

QUrl QGstreamerMediaPlayer::media() const
{
    return m_url;
}

const QIODevice *QGstreamerMediaPlayer::mediaStream() const
{
    return m_stream;
}

void QGstreamerMediaPlayer::decoderPadAdded(const QGstElement &src, const QGstPad &pad)
{
    if (src != decoder)
        return;

    auto caps = pad.currentCaps();
    auto type = caps.at(0).name();
    qCDebug(qLcMediaPlayer) << "Received new pad" << pad.name() << "from" << src.name() << "type" << type;
    qCDebug(qLcMediaPlayer) << "    " << caps;

    TrackType streamType = NTrackTypes;
    if (type.startsWith("video/x-raw")) {
        streamType = VideoStream;
    } else if (type.startsWith("audio/x-raw")) {
        streamType = AudioStream;
    } else if (type.startsWith("text/")) {
        streamType = SubtitleStream;
    } else {
        qCWarning(qLcMediaPlayer) << "Ignoring unknown media stream:" << pad.name() << type;
        return;
    }

    auto &ts = trackSelector(streamType);
    QGstPad sinkPad = ts.createInputPad();
    if (!pad.link(sinkPad)) {
        qCWarning(qLcMediaPlayer) << "Failed to add track, cannot link pads";
        return;
    }
    qCDebug(qLcMediaPlayer) << "Adding track";

    if (ts.trackCount() == 1) {
        if (streamType == VideoStream) {
            ts.setActiveInputPad(sinkPad);
            connectTrackSelectorToOutput(ts);
            videoAvailableChanged(true);
        }
        else if (streamType == AudioStream) {
            ts.setActiveInputPad(sinkPad);
            connectTrackSelectorToOutput(ts);
            audioAvailableChanged(true);
        }
    }

    if (!prerolling)
        tracksChanged();

    decoderOutputMap.emplace(pad, sinkPad);
}

void QGstreamerMediaPlayer::decoderPadRemoved(const QGstElement &src, const QGstPad &pad)
{
    if (src != decoder)
        return;

    qCDebug(qLcMediaPlayer) << "Removed pad" << pad.name() << "from" << src.name();

    auto it = decoderOutputMap.find(pad);
    if (it == decoderOutputMap.end())
        return;
    QGstPad track = it->second;

    auto ts = std::find_if(std::begin(trackSelectors), std::end(trackSelectors),
                           [&](TrackSelector &ts) {
        return ts.inputSelector == track.parent();
    });
    if (ts == std::end(trackSelectors))
        return;

    qCDebug(qLcMediaPlayer) << "   was linked to pad" << track.name() << "from"
                            << ts->inputSelector.name();
    ts->removeInputPad(track);

    if (ts->trackCount() == 0) {
        disconnectTrackSelectorFromOutput(*ts);
        if (ts->type == AudioStream)
            audioAvailableChanged(false);
        else if (ts->type == VideoStream)
            videoAvailableChanged(false);
    }

    if (!prerolling)
        tracksChanged();
}

void QGstreamerMediaPlayer::disconnectAllTrackSelectors()
{
    for (auto &ts : trackSelectors) {
        disconnectTrackSelectorFromOutput(ts);
        ts.removeAllInputPads();
    }
    audioAvailableChanged(false);
    videoAvailableChanged(false);
}

void QGstreamerMediaPlayer::connectTrackSelectorToOutput(TrackSelector &ts)
{
    if (ts.isConnected)
        return;

    QGstElement e = getSinkElementForTrackType(ts.type);
    if (e) {
        qCDebug(qLcMediaPlayer) << "connecting output for track type" << ts.type;
        playerPipeline.add(e);
        qLinkGstElements(ts.inputSelector, e);
        e.syncStateWithParent();
    }

    ts.isConnected = true;
}

void QGstreamerMediaPlayer::disconnectTrackSelectorFromOutput(TrackSelector &ts)
{
    if (!ts.isConnected)
        return;

    QGstElement e = getSinkElementForTrackType(ts.type);
    if (e) {
        qCDebug(qLcMediaPlayer) << "removing output for track type" << ts.type;
        playerPipeline.stopAndRemoveElements(e);
    }

    ts.isConnected = false;
}

void QGstreamerMediaPlayer::uridecodebinElementAddedCallback(GstElement * /*uridecodebin*/,
                                                             GstElement *child,
                                                             QGstreamerMediaPlayer * /*self*/)
{
    QGstElement c(child, QGstElement::NeedsRef);
    qCDebug(qLcMediaPlayer) << "New element added to uridecodebin:" << c.name();

    static const GType decodeBinType = [] {
        QGstElementFactoryHandle factory = QGstElement::findFactory("decodebin");
        return gst_element_factory_get_element_type(factory.get());
    }();

    if (c.type() == decodeBinType) {
        qCDebug(qLcMediaPlayer) << "     -> setting post-stream-topology property";
        c.set("post-stream-topology", true);
    }
}

void QGstreamerMediaPlayer::sourceSetupCallback(GstElement *uridecodebin, GstElement *source,
                                                QGstreamerMediaPlayer *self)
{
    Q_UNUSED(uridecodebin)
    Q_UNUSED(self)

    const gchar *typeName = g_type_name_from_instance((GTypeInstance *)source);
    qCDebug(qLcMediaPlayer) << "Setting up source:" << typeName;

    if (typeName == std::string_view("GstAppSrc")) {
        // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks)
        // GstAppSrc takes ownership of QGstAppSource

        QGstAppSource *appSource =
                new QGstAppSource(qGstSafeCast<GstAppSrc>(source), self->m_stream);
        Q_ASSERT(appSource);
        return;

        // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
    }

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

void QGstreamerMediaPlayer::unknownTypeCallback(GstElement *decodebin, GstPad *pad, GstCaps *caps,
                                                QGstreamerMediaPlayer *self)
{
    Q_UNUSED(decodebin)
    Q_UNUSED(pad)
    Q_UNUSED(self)
    qCDebug(qLcMediaPlayer) << "Unknown type:" << caps;

    QMetaObject::invokeMethod(self, [self] {
        self->stop();
    });
}

static bool isQueue(const QGstElement &element)
{
    static const GType queueType = [] {
        QGstElementFactoryHandle factory = QGstElement::findFactory("queue");
        return gst_element_factory_get_element_type(factory.get());
    }();

    static const GType multiQueueType = [] {
        QGstElementFactoryHandle factory = QGstElement::findFactory("multiqueue");
        return gst_element_factory_get_element_type(factory.get());
    }();

    return element.type() == queueType || element.type() == multiQueueType;
}

void QGstreamerMediaPlayer::decodebinElementAddedCallback(GstBin * /*decodebin*/,
                                                          GstBin * /*sub_bin*/, GstElement *child,
                                                          QGstreamerMediaPlayer *self)
{
    // gstreamer thread!

    QGstElement c(child, QGstElement::NeedsRef);
    qCDebug(qLcMediaPlayer) << "decodebinElementAddedCallback:" << c.name() << c.typeName();

    if (isQueue(c))
        self->decodeBinQueues += 1;
}

void QGstreamerMediaPlayer::decodebinElementRemovedCallback(GstBin * /*decodebin*/,
                                                            GstBin * /*sub_bin*/, GstElement *child,
                                                            QGstreamerMediaPlayer *self)
{
    using namespace Qt::Literals;
    QGstElement c(child, QGstElement::NeedsRef);
    qCDebug(qLcMediaPlayer) << "decodebinElementRemovedCallback:" << c.name() << c.typeName();

    if (isQueue(c))
        self->decodeBinQueues -= 1;
}

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    using namespace std::chrono_literals;

    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << "setting location to" << content;

    prerolling = true;
    m_resourceErrorState = ResourceErrorState::NoError;

    bool ret = playerPipeline.setStateSync(GST_STATE_NULL);
    if (!ret)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";

    m_url = content;
    m_stream = stream;

    if (decoder) {
        playerPipeline.stopAndRemoveElements(decoder);
        decoder = QGstElement{};
    }

    disconnectDecoderHandlers();
    disconnectAllTrackSelectors();
    seekableChanged(false);

    if (m_duration != 0ms) {
        m_duration = 0ms;
        durationChanged(0ms);
    }
    stateChanged(QMediaPlayer::StoppedState);
    if (position() != 0)
        positionChanged(0ms);
    if (!m_metaData.isEmpty()) {
        m_metaData.clear();
        metaDataChanged();
    }

    if (content.isEmpty() && !stream) {
        mediaStatusChanged(QMediaPlayer::NoMedia);
        return;
    }

    decoder = QGstElement::createFromFactory("uridecodebin", "decoder");
    if (!decoder) {
        error(QMediaPlayer::ResourceError, qGstErrorMessageCannotFindElement("uridecodebin"));
        return;
    }

    playerPipeline.add(decoder);

    constexpr bool hasPostStreamTopology = GST_CHECK_VERSION(1, 22, 0);
    if constexpr (hasPostStreamTopology) {
        decoder.set("post-stream-topology", true);
    } else {
        // can't set post-stream-topology to true, as uridecodebin doesn't have the property.
        // Use a hack
        uridecodebinElementAdded =
                decoder.connect("element-added", GCallback(uridecodebinElementAddedCallback), this);
    }

    sourceSetup = decoder.connect("source-setup", GCallback(sourceSetupCallback), this);
    unknownType = decoder.connect("unknown-type", GCallback(unknownTypeCallback), this);

    decoder.set("use-buffering", true);

    constexpr int mb = 1024 * 1024;
    decoder.set("ring-buffer-max-size", 2 * mb);

    updateBufferProgress(0.f);

    elementAdded =
            decoder.connect("deep-element-added", GCallback(decodebinElementAddedCallback), this);
    elementRemoved = decoder.connect("deep-element-removed",
                                     GCallback(decodebinElementRemovedCallback), this);

    padAdded = decoder.onPadAdded<&QGstreamerMediaPlayer::decoderPadAdded>(this);
    padRemoved = decoder.onPadRemoved<&QGstreamerMediaPlayer::decoderPadRemoved>(this);

    if (m_stream) {
        decoder.set("uri", "appsrc://");
        seekableChanged(!m_stream->isSequential());
    } else {
        decoder.set("uri", content.toEncoded().constData());
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);
    if (!playerPipeline.setStateSync(GST_STATE_PAUSED)) {
        qCWarning(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
        // Note: no further error handling: errors will be delivered via a GstMessage
        return;
    }

    playerPipeline.setPosition(0ms);
}

void QGstreamerMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    auto &ts = trackSelector(AudioStream);

    playerPipeline.modifyPipelineWhileNotRunning([&] {
        if (gstAudioOutput)
            disconnectTrackSelectorFromOutput(ts);

        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
        if (gstAudioOutput)
            connectTrackSelectorToOutput(ts);
    });
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
    return m_metaData;
}

void QGstreamerMediaPlayer::setVideoSink(QVideoSink *sink)
{
    using namespace std::chrono_literals;
    gstVideoOutput->setVideoSink(sink);

    if (playerPipeline.state(1s) == GstState::GST_STATE_PAUSED)
        playerPipeline.flush(); // ensure that we send the current video frame to the new sink
}

static QGstStructureView endOfChain(const QGstStructureView &s)
{
    QGstStructureView e = s;
    while (1) {
        auto next = e["next"].toStructure();
        if (!next.isNull())
            e = next;
        else
            break;
    }
    return e;
}

void QGstreamerMediaPlayer::parseStreamsAndMetadata()
{
    qCDebug(qLcMediaPlayer) << "============== parse topology ============";

    if (!topology) {
        qCDebug(qLcMediaPlayer) << "    null topology";
        return;
    }

    QGstStructureView topologyView{ topology };

    QGstCaps caps = topologyView.caps();
    extendMetaDataFromCaps(m_metaData, caps);

    QGstTagListHandle tagList = QGstStructureView{ topology }.tags();
    if (tagList)
        extendMetaDataFromTagList(m_metaData, tagList);

    QGstStructureView demux = endOfChain(topologyView);
    QGValue next = demux["next"];
    if (!next.isList()) {
        qCDebug(qLcMediaPlayer) << "    no additional streams";
        metaDataChanged();
        return;
    }

    // collect stream info
    int size = next.listSize();
    for (int i = 0; i < size; ++i) {
        auto val = next.at(i);
        caps = val.toStructure().caps();

        extendMetaDataFromCaps(m_metaData, caps);

        QGstStructureView structure = caps.at(0);

        if (structure.name().startsWith("video/")) {
            QSize nativeSize = structure.nativeSize();
            gstVideoOutput->setNativeSize(nativeSize);
        }
    }

    auto sinkPad = trackSelector(VideoStream).activeInputPad();
    if (sinkPad) {
        QGstTagListHandle tagList = sinkPad.tags();
        if (tagList)
            qCDebug(qLcMediaPlayer) << "    tags=" << tagList.get();
        else
            qCDebug(qLcMediaPlayer) << "    tags=(null)";
    }

    qCDebug(qLcMediaPlayer) << "============== end parse topology ============";
    playerPipeline.dumpGraph("playback");
}

int QGstreamerMediaPlayer::trackCount(QPlatformMediaPlayer::TrackType type)
{
    return trackSelector(type).trackCount();
}

QMediaMetaData QGstreamerMediaPlayer::trackMetaData(QPlatformMediaPlayer::TrackType type, int index)
{
    auto track = trackSelector(type).inputPad(index);
    if (!track)
        return {};

    QGstTagListHandle tagList = track.tags();
    return taglistToMetaData(tagList);
}

int QGstreamerMediaPlayer::activeTrack(TrackType type)
{
    return trackSelector(type).activeInputIndex();
}

void QGstreamerMediaPlayer::setActiveTrack(TrackType type, int index)
{
    TrackSelector &ts = trackSelector(type);
    QGstPad track = ts.inputPad(index);
    if (track.isNull() && index != -1) {
        qCWarning(qLcMediaPlayer) << "Attempt to set an incorrect index" << index
                                  << "for the track type" << type;
        return;
    }

    qCDebug(qLcMediaPlayer) << "Setting the index" << index << "for the track type" << type;
    if (type == QPlatformMediaPlayer::SubtitleStream)
        gstVideoOutput->flushSubtitles();

    setActivePad(ts, track);
}

void QGstreamerMediaPlayer::setActivePad(TrackSelector &ts, const QGstPad &pad)
{
    playerPipeline.modifyPipelineWhileNotRunning([&] {
        if (pad) {
            ts.setActiveInputPad(pad);
            connectTrackSelectorToOutput(ts);
        } else {
            disconnectTrackSelectorFromOutput(ts);
        }
    });

    // seek to force an immediate change of the stream
    if (playerPipeline.state() == GST_STATE_PLAYING)
        playerPipeline.flush();
}

QT_END_NAMESPACE
