// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include <common/qgstreamermediaplayer_p.h>

#include <audio/qgstreameraudiodevice_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreameraudiooutput_p.h>
#include <common/qgstreamermessage_p.h>
#include <common/qgstreamermetadata_p.h>
#include <common/qgstreamervideooutput_p.h>
#include <common/qgstreamervideosink_p.h>
#include <uri_handler/qgstreamer_qiodevice_handler_p.h>
#include <qgstreamerformatinfo_p.h>

#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>
#include <QtCore/qurl.h>
#include <QtMultimedia/qaudiodevice.h>

#include <thread>

// NOLINTBEGIN(readability-convert-member-functions-to-static)

static Q_LOGGING_CATEGORY(qLcMediaPlayer, "qt.multimedia.player")

QT_BEGIN_NAMESPACE

namespace {

constexpr std::optional<QGstreamerMediaPlayer::TrackType> toTrackType(GstStreamType streamType)
{
    using TrackType = QGstreamerMediaPlayer::TrackType;

    switch (streamType) {
    case GST_STREAM_TYPE_TEXT:
        return TrackType::SubtitleStream;
    case GST_STREAM_TYPE_AUDIO:
        return TrackType::AudioStream;
    case GST_STREAM_TYPE_VIDEO:
        return TrackType::VideoStream;
    default:
        return std::nullopt;
    }
}

std::optional<QGstreamerMediaPlayer::TrackType> toTrackType(GstStream *stream)
{
    if (stream)
        return toTrackType(gst_stream_get_stream_type(stream));

    return std::nullopt;
}

} // namespace

QGstreamerMediaPlayer::TrackSelector::TrackSelector(TrackType type, QGstElement selector)
    : inputSelector(selector), type(type)
{
    selector.set("sync-mode", 1 /*clock*/);

    if (type == SubtitleStream)
        selector.set("cache-buffers", true);

    switch (type) {
    case TrackType::VideoStream:
        dummySink = QGstElement::createFromFactory("fakesink", "dummyVideoSink");
        break;
    case TrackType::AudioStream:
        dummySink = QGstElement::createFromFactory("fakesink", "dummyAudioSink");
        break;
    case TrackType::SubtitleStream:
        dummySink = QGstElement::createFromFactory("fakesink", "dummyTextSink");
        break;
    default:
        Q_UNREACHABLE();
    };

    dummySink.set("sync", true); // consume buffers in real-time
}

QByteArrayView QGstreamerMediaPlayer::TrackSelector::streamIdAtIndex(int index)
{
    if (index >= 0 && index < streams.count()) {
        return streams[index];
    }

    return {};
}

void QGstreamerMediaPlayer::TrackSelector::removeAllInputPads()
{
    for (auto &pad : pads)
        inputSelector.releaseRequestPad(pad.second);
    pads.clear();
}

QGstPad QGstreamerMediaPlayer::TrackSelector::inputPad(int index)
{
    QByteArrayView streamId = streamIdAtIndex(index);
    auto foundIterator = pads.find(streamId);
    if (foundIterator != pads.end())
        return foundIterator->second;

    return {};
}

void QGstreamerMediaPlayer::TrackSelector::setActiveInputPad(const QGstPad &input)
{
    inputSelector.set("active-pad", input);
}

QGstPad QGstreamerMediaPlayer::TrackSelector::getSinkPadForDecoderPad(const QGstPad &decoderPad)
{
    Q_ASSERT(decoderPad.parent() != inputSelector);

    auto iterator = connectionMap.find(decoderPad);
    if (iterator != connectionMap.end()) {
        Q_ASSERT(iterator->second.parent() == inputSelector);
        return iterator->second;
    }

    return {};
}

void QGstreamerMediaPlayer::TrackSelector::addAndConnectInputSelector(QGstPipeline &pipeline,
                                                                      QGstElement sink)
{
    connectedSink = sink ? sink : dummySink;

    pipeline.add(inputSelector, connectedSink);
    qLinkGstElements(inputSelector, connectedSink);
    inputSelector.syncStateWithParent();
    connectedSink.syncStateWithParent();
    inputSelectorInPipeline = true;
}

void QGstreamerMediaPlayer::TrackSelector::removeInputSelector(QGstPipeline &pipeline)
{
    inputSelector.setState(GstState::GST_STATE_READY);
    connectedSink.setState(GstState::GST_STATE_READY);
    qUnlinkGstElements(inputSelector, connectedSink);
    pipeline.stopAndRemoveElements(inputSelector, connectedSink);
    inputSelectorInPipeline = false;
}

void QGstreamerMediaPlayer::TrackSelector::connectInputSelector(QGstPipeline &pipeline,
                                                                QGstElement sink, bool inHandler)
{
    if (inHandler) {
        QGstElement oldElement = inputSelector.src().peer().parent();
        qUnlinkGstElements(inputSelector, oldElement);
        pipeline.stopAndRemoveElements(oldElement);

        QGstElement &newSink = sink ? sink : dummySink;
        pipeline.add(newSink);
        qLinkGstElements(inputSelector, newSink);
        newSink.syncStateWithParent();
        return;
    }

    inputSelector.src().modifyPipelineInIdleProbe([&] {
        connectInputSelector(pipeline, std::move(sink), true);
    });
}

QGstreamerMediaPlayer::TrackSelector &QGstreamerMediaPlayer::trackSelector(TrackType type)
{
    auto &ts = trackSelectors[type];
    Q_ASSERT(ts.type == type);
    return ts;
}

void QGstreamerMediaPlayer::updateTrackMetadata(const QGstStreamCollectionHandle &collection)
{
    // Application Thread

    // CAVEAT: only at the time of GST_MESSAGE_STREAMS_SELECTED the metadata are fully available.

    qCDebug(qLcMediaPlayer) << "QGstreamerMediaPlayer::updateTrackMetadata";

    std::unique_lock lock{
        trackSelectorsMutex,
    };

    for (TrackSelector &selector : trackSelectors)
        selector.metaData = {};

    trackSelector(VideoStream).nativeSize.clear();

    qForeachStreamInCollection(collection, [&](GstStream *stream) {
        std::optional<TrackType> type = toTrackType(stream);
        if (!type) {
            qWarning() << "Unknown track type for stream:" << stream;
            return;
        }

        QGstTagListHandle tagList{
            gst_stream_get_tags(stream),
            QGstTagListHandle::HasRef,
        };

        QByteArray streamId{
            gst_stream_get_stream_id(stream),
        };

        QMediaMetaData metadataFromTags = taglistToMetaData(tagList);

        if (type == TrackType::VideoStream) {
            // GST_TAG_BITRATE is mapped to AudioBitRate. We need to repair the metadata
            if (metadataFromTags.keys().contains(QMediaMetaData::AudioBitRate)) {
                auto audioBitRate = metadataFromTags.value(QMediaMetaData::AudioBitRate);
                metadataFromTags.remove(QMediaMetaData::AudioBitRate);
                metadataFromTags.insert(QMediaMetaData::VideoBitRate, audioBitRate);
            }
        }

        trackSelector(*type).metaData.emplace(streamId, std::move(metadataFromTags));

        QGstCaps caps{
            gst_stream_get_caps(stream),
            QGstCaps::HasRef,
        };
        extendMetaDataFromCaps(m_metaData, caps);

        if (*type == TrackType::VideoStream) {
            Q_ASSERT(caps.size() > 0);
            QGstStructureView structure = caps.at(0);

            if (structure.name().startsWith("video/")) {
                QSize nativeSize = structure.nativeSize();
                trackSelector(*type).nativeSize.emplace(streamId, nativeSize);
            }
        }
    });

    bool hasVideoStream = !trackSelector(VideoStream).streams.empty();
    bool hasAudioStream = !trackSelector(AudioStream).streams.empty();
    trackSelector(VideoStream).selectedInputIndex = hasVideoStream ? 0 : -1;
    trackSelector(AudioStream).selectedInputIndex = hasAudioStream ? 0 : -1;
    trackSelector(SubtitleStream).selectedInputIndex = -1;

    if (hasVideoStream) {
        QSize nativeSize = [&]() -> QSize {
            QByteArrayView streamId = trackSelector(VideoStream).streamIdAtIndex(0);
            auto it = trackSelector(VideoStream).nativeSize.find(streamId);
            if (it != trackSelector(VideoStream).nativeSize.end())
                return it->second;
            return {};
        }();

        gstVideoOutput->setNativeSize(nativeSize);
    }

    lock.unlock();

    // emit signals
    videoAvailableChanged(hasVideoStream);
    audioAvailableChanged(hasAudioStream);

    tracksChanged();
    metaDataChanged();
    activeTracksChanged();
}

void QGstreamerMediaPlayer::prepareTrackMetadata(const QGstStreamCollectionHandle &collection,
                                                 const LockType &lock)
{
    // GStreamer thread!
    qCDebug(qLcMediaPlayer) << "QGstreamerMediaPlayer::prepareTrackMetadata";
    Q_ASSERT(lock.mutex() == &trackSelectorsMutex);

    for (TrackSelector &selector : trackSelectors) {
        selector.metaData = {};
        selector.streams = {};
    }

    trackSelector(VideoStream).nativeSize.clear();

    // we only record the stream IDs at this point
    qForeachStreamInCollection(collection, [&](GstStream *stream) {
        std::optional<TrackType> type = toTrackType(stream);
        if (!type) {
            qWarning() << "Unknown track type for stream:" << stream;
            return;
        }
        QByteArray streamId{
            gst_stream_get_stream_id(stream),
        };
        trackSelector(*type).streams.emplace_back(streamId);
    });
}

void QGstreamerMediaPlayer::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status != QMediaPlayer::StalledMedia)
        m_stalledMediaNotifier.stop();

    qCDebug(qLcMediaPlayer) << "mediaStatusChanged" << status;

    QPlatformMediaPlayer::mediaStatusChanged(status);
}

void QGstreamerMediaPlayer::applyPendingOperations(bool inTimer)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    qCDebug(qLcMediaPlayer) << "applyPendingOperations" << (inTimer ? "in Timer" : "not in timer");

    if (inTimer && playerPipeline.hasAsyncStateChange())
        return;

    if (m_seekRateLimiter.isActive())
        return;

    if (m_pendingSeekPosition || m_pendingRate) {
        if (!inTimer && m_seekTimer.isValid()) {
            // we rate-limit seeks to only one operation per 250ms. this is the same heuristics used
            // in gst_play and prevents some decoder hickups
            constexpr auto seekRateLimit = 250ms;

            auto timeSinceLastSeek = std::chrono::nanoseconds{m_seekTimer.nsecsElapsed()};
            if (timeSinceLastSeek < seekRateLimit) {
                milliseconds remain = round<milliseconds>(seekRateLimit - timeSinceLastSeek);
                m_seekRateLimiter.start(remain);
                return;
            }
        }

        if (m_pendingSeekPosition && m_pendingRate)
            playerPipeline.setPositionAndRate(*m_pendingSeekPosition, *m_pendingRate);
        else if (m_pendingSeekPosition)
            playerPipeline.setPosition(*m_pendingSeekPosition);
        else if (m_pendingRate)
            playerPipeline.setPlaybackRate(*m_pendingRate);

        m_seekTimer.restart();
        m_pendingRate = std::nullopt;
        m_pendingSeekPosition = std::nullopt;
    }

    if (m_pendingState) {
        gstVideoOutput->setActive(m_pendingState.value() > PlaybackState::StoppedState);

        switch (*m_pendingState) {
        case PlaybackState::StoppedState:
            playerPipeline.setState(GstState::GST_STATE_PAUSED);
            positionUpdateTimer.stop();
            break;
        case PlaybackState::PausedState:
            playerPipeline.setState(GstState::GST_STATE_PAUSED);
            if (m_bufferProgress > 0)
                mediaStatusChanged(QMediaPlayer::BufferedMedia);
            else
                mediaStatusChanged(QMediaPlayer::BufferingMedia);
            positionUpdateTimer.stop();
            break;
        case PlaybackState::PlayingState:
            playerPipeline.setState(GstState::GST_STATE_PLAYING);
            positionUpdateTimer.start(100);
            break;

        default:
            Q_UNREACHABLE();
        }

        m_pendingState = std::nullopt;
    }
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
        &padAdded,
        &padRemoved,
        &sourceSetup,
        &selectStream,
    };
    for (QGObjectHandlerScopedConnection *handler : handlers)
        handler->disconnect();
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
    fakeAudioSink.set("sync", true);

    gstVideoOutput->setParent(this);

    playerPipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    playerPipeline.installMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(this));

    constexpr bool useSystemClock = true;
    if constexpr (useSystemClock) {
        // TODO: can we avoid using the system clock?
        QGstClockHandle systemClock{
            gst_system_clock_obtain(),
        };

        gst_pipeline_use_clock(playerPipeline.pipeline(), systemClock.get());
    }

    connect(&positionUpdateTimer, &QTimer::timeout, this, [this] {
        updatePositionFromPipeline();
    });

    m_stalledMediaNotifier.setSingleShot(true);
    connect(&m_stalledMediaNotifier, &QTimer::timeout, this, [this] {
        mediaStatusChanged(QMediaPlayer::StalledMedia);
    });

    m_seekRateLimiter.setSingleShot(true);
    connect(&m_seekRateLimiter, &QTimer::timeout, this, [this] {
        applyPendingOperations(true);
    });
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    playerPipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    playerPipeline.removeMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(this));

    playerPipeline.setStateSync(GST_STATE_READY);
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

std::optional<std::chrono::milliseconds> QGstreamerMediaPlayer::updateDurationFromPipeline()
{
    std::optional<std::chrono::milliseconds> duration = playerPipeline.durationInMs();

    if (duration != m_duration) {
        qCDebug(qLcMediaPlayer) << "updateDurationFromPipeline" << (*duration).count();
        m_duration = *duration;
        durationChanged(m_duration);
    }
    return duration;
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
    using namespace std::chrono_literals;

    if (rate == m_rate)
        return;

    m_rate = rate;
    m_pendingRate = rate;
    playbackRateChanged(rate);
    applyPendingOperations();
}

void QGstreamerMediaPlayer::setPosition(qint64 pos)
{
    std::chrono::milliseconds posInMs{ pos };
    setPosition(posInMs);
}

void QGstreamerMediaPlayer::setPosition(std::chrono::milliseconds pos)
{
    if (state() == QMediaPlayer::StoppedState) {
        // don't seek if we're not playing, yet
        m_pendingSeekPosition = pos;
        positionChanged(pos);
        if (mediaStatus() == QMediaPlayer::EndOfMedia)
            mediaStatusChanged(QMediaPlayer::LoadedMedia);
        return;
    }

    m_pendingSeekPosition = pos;
    positionChanged(pos);
    applyPendingOperations();
}

void QGstreamerMediaPlayer::play()
{
    using namespace std::chrono;

    QMediaPlayer::PlaybackState currentState = state();
    if (currentState == QMediaPlayer::PlayingState || !hasMedia())
        return;

    if (currentState != QMediaPlayer::PausedState)
        resetCurrentLoop();

    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
        m_pendingSeekPosition = 0ns;
        positionChanged(0);
    }

    stateChanged(QMediaPlayer::PlayingState);
    m_pendingState = QMediaPlayer::PlayingState;

    if (!m_playerReady)
        return;

    applyPendingOperations();
}

void QGstreamerMediaPlayer::pause()
{
    using namespace std::chrono_literals;

    if (state() == QMediaPlayer::PausedState || !hasMedia()
        || m_resourceErrorState != ResourceErrorState::NoError)
        return;

    positionUpdateTimer.stop();

    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        positionChanged(0);
        m_pendingSeekPosition = 0ms;
    }

    stateChanged(QMediaPlayer::PausedState);
    m_pendingState = QMediaPlayer::PausedState;
    applyPendingOperations();
}

void QGstreamerMediaPlayer::stop()
{
    using namespace std::chrono_literals;
    if (state() == QMediaPlayer::StoppedState) {
        if (!hasMedia())
            return;

        m_pendingSeekPosition = 0ms;
        positionChanged(0ms);
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
        m_pendingState = QMediaPlayer::StoppedState;
        applyPendingOperations();
        return;
    }
    stopOrEOS(false);
}

const QGstPipeline &QGstreamerMediaPlayer::pipeline() const
{
    return playerPipeline;
}

bool QGstreamerMediaPlayer::canPlayQrc() const
{
    return true;
}

void QGstreamerMediaPlayer::stopOrEOS(bool eos)
{
    using namespace std::chrono_literals;

    m_pendingState = QMediaPlayer::StoppedState;
    if (!eos) {
        m_pendingSeekPosition = 0ms;
        positionChanged(0ms);
    }

    applyPendingOperations();

    stateChanged(QMediaPlayer::StoppedState);
    if (eos)
        mediaStatusChanged(QMediaPlayer::EndOfMedia);
    else
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    m_initialBufferProgressSent = false;
    bufferProgressChanged(0.f);
}

void QGstreamerMediaPlayer::detectPipelineIsSeekable(
        std::optional<std::chrono::nanoseconds> timeout)
{
    // Caveat: seek detection seems to fail until the decoder is added to the decodebin. So we need
    // to perform some busy waiting/explicit polling.
    // we could hook into decodebinElementAddedCallback and wait until all decoder elements are
    // added, though it adds some complexity.

    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto propagateResult = [&](bool canSeek) {
        qCDebug(qLcMediaPlayer) << "detectPipelineIsSeekable: pipeline is seekable:" << canSeek;
        seekableChanged(canSeek);
    };

    if (!timeout) {
        std::optional<bool> canSeek = playerPipeline.canSeek();
        if (canSeek) {
            propagateResult(*canSeek);
        } else {
            qCWarning(qLcMediaPlayer) << "detectPipelineIsSeekable: query for seekable failed.";
            seekableChanged(false);
        }
    } else {
        auto startTime = steady_clock::now();

        for (;;) {
            std::optional<bool> canSeek = playerPipeline.canSeek();
            if (canSeek) {
                propagateResult(*canSeek);
                return;
            }

            if (steady_clock::now() - startTime > *timeout) {
                qCWarning(qLcMediaPlayer)
                    << "detectPipelineIsSeekable: query for seekable failed after"
                    << timeout->count();
                seekableChanged(false);
            }

            qCWarning(qLcMediaPlayer)
                    << "detectPipelineIsSeekable: query for seekable failed ... retrying";

            // backoff and try again
            // unfortunately we cannot poll the gstreamer pipeline here, as it's called from the
            // GstMessage handler.
            std::this_thread::sleep_for(20ms);
        }
    }
}

QGstElement QGstreamerMediaPlayer::getSinkElementForTrackType(TrackType trackType)
{
    switch (trackType) {
    case AudioStream:
        return gstAudioOutput ? gstAudioOutput->gstElement() : fakeAudioSink;
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

    case GST_MESSAGE_SEGMENT_DONE:
        return processBusMessageSegmentDone(message);

    case GST_MESSAGE_STREAM_START:
        return processBusMessageStreamStart(message);

    case GST_MESSAGE_ELEMENT:
        return processBusMessageElement(message);

    case GST_MESSAGE_ASYNC_DONE:
        return processBusMessageAsyncDone(message);

    case GST_MESSAGE_RESET_TIME:
    case GST_MESSAGE_LATENCY:
        return processBusMessageLatency(message);

    case GST_MESSAGE_STREAM_COLLECTION:
        return processBusMessageStreamCollection(message);

    case GST_MESSAGE_STREAMS_SELECTED:
        return processBusMessageStreamsSelected(message);

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

bool QGstreamerMediaPlayer::processBusMessageDurationChanged(const QGstreamerMessage &msg)
{
    if (m_prerolling)
        return false;

    if (msg.source() != playerPipeline)
        return false;

    std::optional<std::chrono::milliseconds> pipelineDuration = updateDurationFromPipeline();
    Q_ASSERT(pipelineDuration); // CHECK: can this happen?

    if (pipelineDuration) {
        bool ok = false;
        qint64 durationInMetadata = m_metaData.value(QMediaMetaData::Duration).toLongLong(&ok);
        if (ok && pipelineDuration->count() != durationInMetadata) {
            m_metaData.insert(QMediaMetaData::Duration, qint64(pipelineDuration->count()));
            metaDataChanged();
        }
    } else {
        qWarning() << "QGstreamerMediaPlayer: GST_MESSAGE_DURATION_CHANGED received, but cannot "
                      "obtain duration from pipeliine";
    }
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageBuffering(const QGstreamerMessage &message)
{
    int progress = 0;
    gst_message_parse_buffering(message.message(), &progress);

    if (state() != QMediaPlayer::StoppedState && !(m_prerolling || m_waitingForStreams)) {
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
    using namespace std::chrono_literals;
    positionChanged(m_duration);
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

    playerPipeline.dumpGraph("processBusMessageStateChanged");

    switch (newState) {
    case GST_STATE_VOID_PENDING:
    case GST_STATE_NULL:
    case GST_STATE_READY:
    case GST_STATE_PAUSED: {
        if (m_prerolling && !m_playerReady) {
            m_prerolling = false;
            finalizePreroll();
        }

        break;
    }
    case GST_STATE_PLAYING: {
        if (!m_prerolling && !m_waitingForStreams) {
            bool eosReached = mediaStatus() == QMediaPlayer::EndOfMedia;
            // GStreamer can deliver GST_MESSAGE_EOS before the pipeline is GST_STATE_PLAYING
            // trying to protect against this

            if (!eosReached) {
                if (!m_initialBufferProgressSent) {
                    bool immediatelySendBuffered = m_bufferProgress > 0;
                    mediaStatusChanged(QMediaPlayer::BufferingMedia);
                    m_initialBufferProgressSent = true;
                    if (immediatelySendBuffered)
                        mediaStatusChanged(QMediaPlayer::BufferedMedia);
                }
            }
        }
        qCDebug(qLcMediaPlayer) << (m_prerolling ? "    prerolling" : "prerolled");
        qCDebug(qLcMediaPlayer) << (m_waitingForStreams ? "    waitingForStreams"
                                                        : "stream selection complete");

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
        qCDebug(qLcMediaPlayer) << "    segment start message, updating position" << posNs.count();
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

bool QGstreamerMediaPlayer::processBusMessageSegmentDone(const QGstreamerMessage &message)
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    if (doLoop()) {
        playerPipeline.setPosition(0ns, /*flush=*/false); // non-flushing seek do start
    } else {
        gint64 pos;
        GstFormat fmt{};
        gst_message_parse_segment_done(message.message(), &fmt, &pos);

        switch (fmt) {
        case GST_FORMAT_TIME: {
            auto posNs = std::chrono::nanoseconds{ pos };
            positionChanged(round<milliseconds>(posNs));
            break;
        }
        default: {
            qWarning() << "GST_MESSAGE_SEGMENT_DONE with unknown format" << fmt << pos;
            break;
        }
        }

        // when the last segment is played, we queue an artificial "null" loop, which will result in
        // an EOS once all buffers are delivered to the sinks.

        playerPipeline.seekToEndWithEOS(); // artificial "null" loop to receive EOS
    }

    return false;
}

bool QGstreamerMediaPlayer::processBusMessageElement(const QGstreamerMessage &message)
{
    QGstStructureView structure(gst_message_get_structure(message.message()));
    auto type = structure.name();
    Q_ASSERT(type != "stream-topology");
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageAsyncDone(const QGstreamerMessage &)
{
    applyPendingOperations();
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageStreamStart(const QGstreamerMessage &)
{
    updateDurationFromPipeline();
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageStreamCollection(const QGstreamerMessage &message)
{
    // CAVEAT: at the time when GST_MESSAGE_STREAM_COLLECTION is emitted, the metadata are not fully
    // available. Since we enable all streames, we parse the metadata while handling
    // GST_MESSAGE_STREAMS_SELECTED

    constexpr bool traceStreamCollection = false;
    if constexpr (traceStreamCollection) {
        QGstStreamCollectionHandle collection;
        gst_message_parse_stream_collection(const_cast<GstMessage *>(message.message()),
                                            &collection);

        qCDebug(qLcMediaPlayer) << "processBusMessageStreamCollection";
        qForeachStreamInCollection(collection, [](GstStream *stream) {
            qCDebug(qLcMediaPlayer) << "    stream" << stream
                                    << QGstTagListHandle{
                                           gst_stream_get_tags(stream),
                                           QGstTagListHandle::HasRef,
                                       };
        });
    }
    return false;
}

bool QGstreamerMediaPlayer::processBusMessageStreamsSelected(const QGstreamerMessage &message)
{
    QGstStreamCollectionHandle collection;
    gst_message_parse_streams_selected(const_cast<GstMessage *>(message.message()), &collection);

    constexpr bool traceStreamCollection = false;
    if constexpr (traceStreamCollection) {
        qCDebug(qLcMediaPlayer) << "processBusMessageStreamsSelected";
        qForeachStreamInCollection(collection, [](GstStream *stream) {
            qCDebug(qLcMediaPlayer) << "    stream" << stream
                                    << QGstTagListHandle{
                                           gst_stream_get_tags(stream),
                                           QGstTagListHandle::HasRef,
                                       };
        });
    }

    updateTrackMetadata(collection);

    m_waitingForStreams = false;
    mediaStatusChanged(QMediaPlayer::LoadedMedia);

    finalizePreroll();

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
    case GST_MESSAGE_STREAM_COLLECTION:
        return processSyncMessageStreamCollection(message);

    default:
        return false;
    }
}

bool QGstreamerMediaPlayer::processSyncMessageStreamCollection(const QGstreamerMessage &message)
{
    // GStreamer thread!

    std::unique_lock lock{
        trackSelectorsMutex,
    };

    for (TrackSelector &selector : trackSelectors)
        selector.pads.clear();

    QGstStreamCollectionHandle collection;
    gst_message_parse_stream_collection(const_cast<GstMessage *>(message.message()), &collection);
    prepareTrackMetadata(collection, lock);
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

void QGstreamerMediaPlayer::decoderPadAdded(const QGstElement &src, const QGstPad &pad)
{
    // GStreamer thread!
    if (src != decoder)
        return;

    qCDebug(qLcMediaPlayer) << "Added pad" << pad.name() << "from" << src.name();

    std::optional<TrackType> type = pad.inferTrackTypeFromName();

    TrackType streamType = *type;

    std::unique_lock lock{
        trackSelectorsMutex,
    };

    auto &ts = trackSelector(streamType);

    qCDebug(qLcMediaPlayer) << ">>>>>>>>>>>>>>>>>> add input selector";

    if (ts.pads.empty())
        ts.addAndConnectInputSelector(playerPipeline, getSinkElementForTrackType(streamType));

    QGstPad sinkPad = ts.inputSelector.getRequestPad("sink_%u");
    if (!pad.link(sinkPad)) {
        qCWarning(qLcMediaPlayer) << "Failed to add track, cannot link pads";
        return;
    }
    qCDebug(qLcMediaPlayer) << "Adding track";

    ts.connectionMap.emplace(pad, sinkPad);

    QGString streamId = pad.streamId();
    ts.pads.emplace(streamId, sinkPad);

    switch (streamType) {
    case TrackType::VideoStream:
    case TrackType::AudioStream:
        if (streamId.asByteArrayView() == ts.streams.front())
            ts.setActiveInputPad(sinkPad);

        break;

    default:
        break;
    }

    if (streamType == TrackType::VideoStream)
        playerPipeline.dumpPipelineGraph("vsink");
}

void QGstreamerMediaPlayer::decoderPadRemoved(const QGstElement &src, const QGstPad &decoderPad)
{
    if (src != decoder)
        return;

    // application thread!

    qCDebug(qLcMediaPlayer) << "Removed pad" << decoderPad.name() << "from" << src.name()
                            << "for stream" << decoderPad.streamId();

    std::optional<TrackType> type = decoderPad.inferTrackTypeFromName();

    std::unique_lock lock{
        trackSelectorsMutex,
    };
    TrackSelector &ts = trackSelector(*type);

    QGstPad inputSelectorSinkPad = ts.getSinkPadForDecoderPad(decoderPad);
    if (inputSelectorSinkPad) {
        decoderPad.unlink(inputSelectorSinkPad);
        ts.inputSelector.releaseRequestPad(inputSelectorSinkPad);

        auto it = std::find_if(ts.pads.cbegin(), ts.pads.cend(), [&](auto &pair) {
            return pair.second == inputSelectorSinkPad;
        });

        ts.pads.erase(it);

    } else {
        playerPipeline.dumpPipelineGraph("decoderPadRemoved");
        Q_ASSERT(false); // can this happen??
    }

    if (ts.pads.empty()) {
        disconnectTrackSelectorFromOutput(ts, /*inHandler=*/true);
        ts.removeInputSelector(playerPipeline);
    }
}

void QGstreamerMediaPlayer::disconnectAllTrackSelectors()
{
    for (auto &ts : trackSelectors) {
        disconnectTrackSelectorFromOutput(ts);
        ts.removeAllInputPads();
        playerPipeline.stopAndRemoveElements(ts.dummySink);
    }

    audioAvailableChanged(false);
    videoAvailableChanged(false);
}

void QGstreamerMediaPlayer::connectTrackSelectorToOutput(TrackSelector &ts, bool inHandler)
{
    QGstElement e = getSinkElementForTrackType(ts.type);
    if (e) {
        qCDebug(qLcMediaPlayer) << "connecting output for track type" << ts.type;
        ts.connectInputSelector(playerPipeline, e, inHandler);
    }
}

void QGstreamerMediaPlayer::disconnectTrackSelectorFromOutput(TrackSelector &ts, bool inHandler)
{
    qCDebug(qLcMediaPlayer) << "removing output for track type" << ts.type;
    if (ts.inputSelectorInPipeline)
        ts.connectInputSelector(playerPipeline, {}, inHandler);
}

void QGstreamerMediaPlayer::sourceSetupCallback(GstElement *uridecodebin, GstElement *source,
                                                QGstreamerMediaPlayer *self)
{
    Q_UNUSED(uridecodebin)
    Q_UNUSED(self)

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

gint QGstreamerMediaPlayer::s_decodebin3SelectStream(GstElement * /*decodebin*/,
                                                     GstStreamCollection * /*collection*/,
                                                     GstStream *stream,
                                                     QGstreamerMediaPlayer * /*self*/)
{
    qCDebug(qLcMediaPlayer) << "decodebin3SelectStream" << stream;
    return 1; // we enable all streams
}

void QGstreamerMediaPlayer::finalizePreroll()
{
    using namespace std::chrono_literals;

    if (m_prerolling || m_waitingForStreams)
        return;

    qCDebug(qLcMediaPlayer) << "Preroll done, setting status to Loaded";
    playerPipeline.dumpGraph("playerPipelinePrerollDone");

    std::optional<bool> canSeek = playerPipeline.canSeek();
    if (canSeek)
        seekableChanged(*canSeek);

    std::optional<std::chrono::milliseconds> duration = updateDurationFromPipeline();
    if (duration)
        m_metaData.insert(QMediaMetaData::Duration, qint64(duration->count()));

    if (!m_url.isEmpty())
        m_metaData.insert(QMediaMetaData::Url, m_url);
    metaDataChanged();
    tracksChanged();
    mediaStatusChanged(QMediaPlayer::LoadedMedia);

    if (state() == QMediaPlayer::PlayingState) {
        if (!m_initialBufferProgressSent) {
            bool immediatelySendBuffered = m_bufferProgress > 0;
            mediaStatusChanged(QMediaPlayer::BufferingMedia);
            m_initialBufferProgressSent = true;
            if (immediatelySendBuffered)
                mediaStatusChanged(QMediaPlayer::BufferedMedia);
        }
    }

    m_playerReady = true;

    applyPendingOperations(/*inTimer=*/false);
}

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    using namespace std::chrono_literals;

    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << "setting location to" << content;

    bool ret = playerPipeline.setStateSync(GST_STATE_NULL);
    if (!ret)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";

    m_url = content;
    m_stream = stream;
    QUrl streamURL;
    if (stream)
        streamURL = qGstRegisterQIODevice(stream);

    if (decoder) {
        playerPipeline.stopAndRemoveElements(decoder);
        decoder = QGstElement{};
        QGstBusHandle bus = QGstBusHandle{
            gst_pipeline_get_bus(playerPipeline.getPipeline().pipeline()),
            QGstBusHandle::HasRef,
        };
        gst_bus_set_flushing(bus.get(), true);
        gst_bus_set_flushing(bus.get(), false);
    }
    m_playerReady = false;
    m_prerolling = true;
    m_waitingForStreams = true;
    m_resourceErrorState = ResourceErrorState::NoError;

    disconnectDecoderHandlers();
    disconnectAllTrackSelectors();
    seekableChanged(false);

    auto resetMetadata = QScopeGuard([&] {
        bool notifyMetaData = !m_metaData.isEmpty();
        bool notifyTrackMetadata = false;
        m_metaData.clear();

        for (TrackSelector &selector : trackSelectors) {
            notifyTrackMetadata = notifyTrackMetadata && selector.metaData.empty();
            selector.metaData.clear();
            selector.selectedInputIndex = -1;
        }

        if (notifyMetaData)
            this->metaDataChanged();

        if (notifyTrackMetadata)
            this->tracksChanged();
    });

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

    decoder = QGstElement::createFromFactory("uridecodebin3", "decoder");
    if (!decoder) {
        error(QMediaPlayer::ResourceError, qGstErrorMessageCannotFindElement("uridecodebin3"));
        return;
    }

    playerPipeline.add(decoder);
    playerPipeline.dumpGraph("decoderAdded");

    sourceSetup = decoder.connect("source-setup", GCallback(sourceSetupCallback), this);

    decoder.set("use-buffering", true);

    constexpr guint64 mb = 1024 * 1024;

    // Caveat: we need to make the ringbuffer "sufficiently" large to workaround a gstreamer
    // bug: https://gitlab.freedesktop.org/gstreamer/gstreamer/-/issues/3505
    decoder.set("ring-buffer-max-size", 4 * mb);

    updateBufferProgress(0.f);

    padAdded = decoder.onPadAdded<&QGstreamerMediaPlayer::decoderPadAdded>(this);
    padRemoved = decoder.onPadRemoved<&QGstreamerMediaPlayer::decoderPadRemoved>(this);
    selectStream = decoder.connect("select-stream", GCallback(s_decodebin3SelectStream), this);

    QUrl uri = m_stream ? streamURL : content;
    decoder.set("uri", uri.toEncoded().constData());
    if (m_stream) {
        seekableChanged(!m_stream->isSequential());
    } else {
        if (content.toEncoded().startsWith("qrc:"))
            seekableChanged(true); // qrc resources are seekable
    }

    mediaStatusChanged(QMediaPlayer::LoadingMedia);
    playerPipeline.setStateSync(GST_STATE_PAUSED);
    playerPipeline.setPosition(0ns, /*flush=*/false);

    if (!playerPipeline.setStateSync(GST_STATE_PAUSED)) {
        qCWarning(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
        // Note: no further error handling: errors will be delivered via a GstMessage

        // re-set state to ready to ensure that the pipeline is not in a failed state.
        playerPipeline.setStateSync(GST_STATE_READY);

        return;
    }

    resetMetadata.dismiss();
}

void QGstreamerMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    auto *gstOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstOutput)
        gstOutput->setAsync(true);

    std::unique_lock lock{
        trackSelectorsMutex,
    };
    auto &ts = trackSelector(AudioStream);

    if (!ts.inputSelectorInPipeline) {
        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
        return;
    }

    ts.inputSelector.src().modifyPipelineInIdleProbe([&] {
        if (gstAudioOutput)
            disconnectTrackSelectorFromOutput(ts);

        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
        connectTrackSelectorToOutput(ts);
    });

    lock.unlock();
    playerPipeline.recalculateLatency();
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
    if (m_waitingForStreams)
        return {};

    std::unique_lock lock{
        trackSelectorsMutex,
    };
    return m_metaData;
}

void QGstreamerMediaPlayer::setVideoSink(QVideoSink *sink)
{
    auto *gstSink = sink ? static_cast<QGstreamerVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (gstSink)
        gstSink->setAsync(false);

    using namespace std::chrono_literals;
    gstVideoOutput->setVideoSink(sink);

    if (sink && state() != QMediaPlayer::StoppedState) {
        if (!m_pendingSeekPosition) {
            m_pendingSeekPosition = playerPipeline.position();
            applyPendingOperations();
        }
    }
    playerPipeline.dumpGraph("setVideoSink");
}

int QGstreamerMediaPlayer::trackCount(QPlatformMediaPlayer::TrackType type)
{
    if (m_waitingForStreams)
        return 0;

    std::unique_lock lock{
        trackSelectorsMutex,
    };
    return trackSelector(type).streams.size();
}

QMediaMetaData QGstreamerMediaPlayer::trackMetaData(QPlatformMediaPlayer::TrackType type, int index)
{
    if (m_waitingForStreams)
        return {};

    std::unique_lock lock{
        trackSelectorsMutex,
    };

    TrackSelector &ts = trackSelector(type);
    QByteArrayView streamId = ts.streamIdAtIndex(index);

    auto metadataIterator = ts.metaData.find(streamId);
    if (metadataIterator == ts.metaData.end())
        return {};

    QMediaMetaData result = metadataIterator->second;

    return result;
}

int QGstreamerMediaPlayer::activeTrack(TrackType type)
{
    return trackSelector(type).selectedInputIndex;
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
    trackSelector(type).selectedInputIndex = index;

    if (type == TrackType::VideoStream)
        gstVideoOutput->setActive(index >= 0);
}

void QGstreamerMediaPlayer::setActivePad(TrackSelector &ts, const QGstPad &pad, bool flush)
{
    if (pad) {
        ts.setActiveInputPad(pad);
        connectTrackSelectorToOutput(ts);
    } else {
        disconnectTrackSelectorFromOutput(ts);
    }

    if (flush) {
        // seek to force an immediate change of the stream
        if (playerPipeline.state() == GST_STATE_PLAYING)
            playerPipeline.flush();
    }
}

QT_END_NAMESPACE
