// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qgstreamermediaplayer_p.h>
#include <qgstpipeline_p.h>
#include <qgstreamermetadata_p.h>
#include <qgstreamerformatinfo_p.h>
#include <qgstreameraudiooutput_p.h>
#include <qgstreamervideooutput_p.h>
#include <qgstreamervideosink_p.h>
#include "qgstreamermessage_p.h"
#include <qgstreameraudiodevice_p.h>
#include <qgstappsrc_p.h>
#include <qaudiodevice.h>

#include <QtCore/qdir.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtNetwork/qnetworkreply.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static Q_LOGGING_CATEGORY(qLcMediaPlayer, "qt.multimedia.player")

QT_BEGIN_NAMESPACE

QGstreamerMediaPlayer::TrackSelector::TrackSelector(TrackType type, QGstElement selector)
    : selector(selector), type(type)
{
    selector.set("sync-streams", true);
    selector.set("sync-mode", 1 /*clock*/);

    if (type == SubtitleStream)
        selector.set("cache-buffers", true);
}

QGstPad QGstreamerMediaPlayer::TrackSelector::createInputPad()
{
    auto pad = selector.getRequestPad("sink_%u");
    tracks.append(pad);
    return pad;
}

void QGstreamerMediaPlayer::TrackSelector::removeAllInputPads()
{
    for (auto &pad : tracks)
        selector.releaseRequestPad(pad);
    tracks.clear();
}

void QGstreamerMediaPlayer::TrackSelector::removeInputPad(QGstPad pad)
{
    selector.releaseRequestPad(pad);
    tracks.removeOne(pad);
}

QGstPad QGstreamerMediaPlayer::TrackSelector::inputPad(int index)
{
    if (index >= 0 && index < tracks.count())
        return tracks[index];
    return {};
}

QGstreamerMediaPlayer::TrackSelector &QGstreamerMediaPlayer::trackSelector(TrackType type)
{
    auto &ts = trackSelectors[type];
    Q_ASSERT(ts.type == type);
    return ts;
}

QMaybe<QPlatformMediaPlayer *> QGstreamerMediaPlayer::create(QMediaPlayer *parent)
{
    auto videoOutput = QGstreamerVideoOutput::create();
    if (!videoOutput)
        return videoOutput.error();

    QGstElement decodebin("decodebin", nullptr);
    if (!decodebin)
        return errorMessageCannotFindElement("decodebin");

    QGstElement videoInputSelector("input-selector", "videoInputSelector");
    if (!videoInputSelector)
        return errorMessageCannotFindElement("input-selector");

    QGstElement audioInputSelector("input-selector", "audioInputSelector");
    if (!audioInputSelector)
        return errorMessageCannotFindElement("input-selector");

    QGstElement subTitleInputSelector("input-selector", "subTitleInputSelector");
    if (!subTitleInputSelector)
        return errorMessageCannotFindElement("input-selector");

    return new QGstreamerMediaPlayer(videoOutput.value(), decodebin, videoInputSelector,
                                     audioInputSelector, subTitleInputSelector, parent);
}

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QGstreamerVideoOutput *videoOutput,
                                             QGstElement decodebin,
                                             QGstElement videoInputSelector,
                                             QGstElement audioInputSelector,
                                             QGstElement subTitleInputSelector,
                                             QMediaPlayer *parent)
    : QObject(parent),
      QPlatformMediaPlayer(parent),
      trackSelectors{ { { VideoStream, videoInputSelector },
                        { AudioStream, audioInputSelector },
                        { SubtitleStream, subTitleInputSelector } } },
      playerPipeline("playerPipeline"),
      gstVideoOutput(videoOutput)
{
    playerPipeline.setFlushOnConfigChanges(true);

    gstVideoOutput->setParent(this);
    gstVideoOutput->setPipeline(playerPipeline);

    for (auto &ts : trackSelectors)
        playerPipeline.add(ts.selector);

    playerPipeline.setState(GST_STATE_NULL);
    playerPipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    playerPipeline.installMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(this));

    gst_pipeline_use_clock(playerPipeline.pipeline(), gst_system_clock_obtain());

    /* Taken from gstdicoverer.c:
     * This is ugly. We get the GType of decodebin so we can quickly detect
     * when a decodebin is added to uridecodebin so we can set the
     * post-stream-topology setting to TRUE */
    decodebinType = G_OBJECT_TYPE(decodebin.element());
    connect(&positionUpdateTimer, &QTimer::timeout, this, &QGstreamerMediaPlayer::updatePosition);
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    playerPipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    playerPipeline.removeMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(this));
    playerPipeline.setStateSync(GST_STATE_NULL);
    topology.free();
}

qint64 QGstreamerMediaPlayer::position() const
{
    if (playerPipeline.isNull() || m_url.isEmpty())
        return 0;

    return playerPipeline.position()/1e6;
}

qint64 QGstreamerMediaPlayer::duration() const
{
    return m_duration;
}

float QGstreamerMediaPlayer::bufferProgress() const
{
    return m_bufferProgress/100.;
}

QMediaTimeRange QGstreamerMediaPlayer::availablePlaybackRanges() const
{
    return QMediaTimeRange();
}

qreal QGstreamerMediaPlayer::playbackRate() const
{
    return playerPipeline.playbackRate();
}

void QGstreamerMediaPlayer::setPlaybackRate(qreal rate)
{
    if (playerPipeline.setPlaybackRate(rate))
        playbackRateChanged(rate);
}

void QGstreamerMediaPlayer::setPosition(qint64 pos)
{
    qint64 currentPos = playerPipeline.position()/1e6;
    if (pos == currentPos)
        return;
    playerPipeline.finishStateChange();
    playerPipeline.setPosition(pos*1e6);
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << pos << playerPipeline.position()/1e6;
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    positionChanged(pos);
}

void QGstreamerMediaPlayer::play()
{
    if (state() == QMediaPlayer::PlayingState || m_url.isEmpty())
        return;
    resetCurrentLoop();

    playerPipeline.setInStoppedState(false);
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        playerPipeline.setPosition(0);
        updatePosition();
    }

    qCDebug(qLcMediaPlayer) << "play().";
    int ret = playerPipeline.setState(GST_STATE_PLAYING);
    if (m_requiresSeekOnPlay) {
        // Flushing the pipeline is required to get track changes
        // immediately, when they happen while paused.
        playerPipeline.flush();
        m_requiresSeekOnPlay = false;
    }
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the playing state.";
    if (mediaStatus() == QMediaPlayer::LoadedMedia)
        mediaStatusChanged(QMediaPlayer::BufferedMedia);
    emit stateChanged(QMediaPlayer::PlayingState);
    positionUpdateTimer.start(100);
}

void QGstreamerMediaPlayer::pause()
{
    if (state() == QMediaPlayer::PausedState || m_url.isEmpty())
        return;

    positionUpdateTimer.stop();
    if (playerPipeline.inStoppedState()) {
        playerPipeline.setInStoppedState(false);
        playerPipeline.flush();
    }
    int ret = playerPipeline.setState(GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        playerPipeline.setPosition(0);
        mediaStatusChanged(QMediaPlayer::BufferedMedia);
    }
    updatePosition();
    emit stateChanged(QMediaPlayer::PausedState);
}

void QGstreamerMediaPlayer::stop()
{
    if (state() == QMediaPlayer::StoppedState)
        return;
    stopOrEOS(false);
}

void *QGstreamerMediaPlayer::nativePipeline()
{
    return playerPipeline.pipeline();
}

void QGstreamerMediaPlayer::stopOrEOS(bool eos)
{
    positionUpdateTimer.stop();
    playerPipeline.setInStoppedState(true);
    bool ret = playerPipeline.setStateSync(GST_STATE_PAUSED);
    if (!ret)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";
    if (!eos)
        playerPipeline.setPosition(0);
    updatePosition();
    emit stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(eos ? QMediaPlayer::EndOfMedia : QMediaPlayer::LoadedMedia);
}

bool QGstreamerMediaPlayer::processBusMessage(const QGstreamerMessage &message)
{
    if (message.isNull())
        return false;

//    qCDebug(qLcMediaPlayer) << "received bus message from" << message.source().name() << message.type() << (message.type() == GST_MESSAGE_TAG);

    GstMessage* gm = message.rawMessage();
    switch (message.type()) {
    case GST_MESSAGE_TAG: {
        // #### This isn't ideal. We shouldn't catch stream specific tags here, rather the global ones
        GstTagList *tag_list;
        gst_message_parse_tag(gm, &tag_list);
        //qCDebug(qLcMediaPlayer) << "Got tags: " << message.source().name() << gst_tag_list_to_string(tag_list);
        auto metaData = QGstreamerMetaData::fromGstTagList(tag_list);
        for (auto k : metaData.keys())
            m_metaData.insert(k, metaData.value(k));
        break;
    }
    case GST_MESSAGE_DURATION_CHANGED: {
        qint64 d = playerPipeline.duration()/1e6;
        qCDebug(qLcMediaPlayer) << "    duration changed message" << d;
        if (d != m_duration) {
            m_duration = d;
            emit durationChanged(duration());
        }
        return false;
    }
    case GST_MESSAGE_EOS:
        if (doLoop()) {
            setPosition(0);
            break;
        }
        stopOrEOS(true);
        break;
    case GST_MESSAGE_BUFFERING: {
        qCDebug(qLcMediaPlayer) << "    buffering message";
        int progress = 0;
        gst_message_parse_buffering(gm, &progress);
        m_bufferProgress = progress;
        mediaStatusChanged(m_bufferProgress == 100 ? QMediaPlayer::BufferedMedia : QMediaPlayer::BufferingMedia);
        emit bufferProgressChanged(m_bufferProgress/100.);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
        if (message.source() != playerPipeline)
            return false;

        GstState    oldState;
        GstState    newState;
        GstState    pending;

        gst_message_parse_state_changed(gm, &oldState, &newState, &pending);
        qCDebug(qLcMediaPlayer) << "    state changed message" << oldState << newState << pending;

#ifdef DEBUG_PLAYBIN
        static QStringList states = {
                  QStringLiteral("GST_STATE_VOID_PENDING"),  QStringLiteral("GST_STATE_NULL"),
                  QStringLiteral("GST_STATE_READY"), QStringLiteral("GST_STATE_PAUSED"),
                  QStringLiteral("GST_STATE_PLAYING") };

        qCDebug(qLcMediaPlayer) << QStringLiteral("state changed: old: %1  new: %2  pending: %3") \
                .arg(states[oldState]) \
                .arg(states[newState]) \
                .arg(states[pending]);
#endif

        switch (newState) {
        case GST_STATE_VOID_PENDING:
        case GST_STATE_NULL:
        case GST_STATE_READY:
            break;
        case GST_STATE_PAUSED:
        {
            if (prerolling) {
                qCDebug(qLcMediaPlayer) << "Preroll done, setting status to Loaded";
                prerolling = false;
                GST_DEBUG_BIN_TO_DOT_FILE(playerPipeline.bin(), GST_DEBUG_GRAPH_SHOW_ALL, "playerPipeline");

                qint64 d = playerPipeline.duration()/1e6;
                if (d != m_duration) {
                    m_duration = d;
                    qCDebug(qLcMediaPlayer) << "    duration changed" << d;
                    emit durationChanged(duration());
                }

                parseStreamsAndMetadata();

                emit tracksChanged();
                mediaStatusChanged(QMediaPlayer::LoadedMedia);

                GstQuery *query = gst_query_new_seeking(GST_FORMAT_TIME);
                gboolean canSeek = false;
                if (gst_element_query(playerPipeline.element(), query)) {
                    gst_query_parse_seeking(query, NULL, &canSeek, nullptr, nullptr);
                    qCDebug(qLcMediaPlayer) << "    pipeline is seekable:" << canSeek;
                } else {
                    qCDebug(qLcMediaPlayer) << "    query for seekable failed.";
                }
                gst_query_unref(query);
                seekableChanged(canSeek);
            }

            break;
        }
        case GST_STATE_PLAYING:
            mediaStatusChanged(QMediaPlayer::BufferedMedia);
            break;
        }
        break;
    }
    case GST_MESSAGE_ERROR: {
        GError *err;
        gchar *debug;
        gst_message_parse_error(gm, &err, &debug);
        if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND)
            emit error(QMediaPlayer::FormatError, tr("Cannot play stream of type: <unknown>"));
        else
            emit error(QMediaPlayer::ResourceError, QString::fromUtf8(err->message));
        playerPipeline.dumpGraph("error");
        mediaStatusChanged(QMediaPlayer::InvalidMedia);
        g_error_free(err);
        g_free(debug);
        break;
    }
    case GST_MESSAGE_WARNING: {
        GError *err;
        gchar *debug;
        gst_message_parse_warning (gm, &err, &debug);
        qCWarning(qLcMediaPlayer) << "Warning:" << QString::fromUtf8(err->message);
        playerPipeline.dumpGraph("warning");
        g_error_free (err);
        g_free (debug);
        break;
    }
    case GST_MESSAGE_INFO: {
        if (qLcMediaPlayer().isDebugEnabled()) {
            GError *err;
            gchar *debug;
            gst_message_parse_info (gm, &err, &debug);
            qCDebug(qLcMediaPlayer) << "Info:" << QString::fromUtf8(err->message);
            g_error_free (err);
            g_free (debug);
        }
        break;
    }
    case GST_MESSAGE_SEGMENT_START: {
        qCDebug(qLcMediaPlayer) << "    segment start message, updating position";
        QGstStructure structure(gst_message_get_structure(gm));
        auto p = structure["position"].toInt64();
        if (p) {
            qint64 position = (*p)/1000000;
            emit positionChanged(position);
        }
        break;
    }
    case GST_MESSAGE_ELEMENT: {
        QGstStructure structure(gst_message_get_structure(gm));
        auto type = structure.name();
        if (type == "stream-topology") {
            topology.free();
            topology = structure.copy();
        }
        break;
    }

    default:
//        qCDebug(qLcMediaPlayer) << "    default message handler, doing nothing";

        break;
    }

    return false;
}

bool QGstreamerMediaPlayer::processSyncMessage(const QGstreamerMessage &message)
{
#if QT_CONFIG(gstreamer_gl)
    if (message.type() != GST_MESSAGE_NEED_CONTEXT)
        return false;
    const gchar *type = nullptr;
    gst_message_parse_context_type (message.rawMessage(), &type);
    if (strcmp(type, GST_GL_DISPLAY_CONTEXT_TYPE))
        return false;
    if (!gstVideoOutput || !gstVideoOutput->gstreamerVideoSink())
        return false;
    auto *context = gstVideoOutput->gstreamerVideoSink()->gstGlDisplayContext();
    if (!context)
        return false;
    gst_element_set_context(GST_ELEMENT(GST_MESSAGE_SRC(message.rawMessage())), context);
    playerPipeline.dumpGraph("need_context");
    return true;
#else
    Q_UNUSED(message);
    return false;
#endif
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
    qCDebug(qLcMediaPlayer) << "    " << caps.toString();

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
            connectOutput(ts);
            ts.setActiveInputPad(sinkPad);
            emit videoAvailableChanged(true);
        }
        else if (streamType == AudioStream) {
            connectOutput(ts);
            ts.setActiveInputPad(sinkPad);
            emit audioAvailableChanged(true);
        }
    }

    if (!prerolling)
        emit tracksChanged();

    decoderOutputMap.insert(pad.name(), sinkPad);
}

void QGstreamerMediaPlayer::decoderPadRemoved(const QGstElement &src, const QGstPad &pad)
{
    if (src != decoder)
        return;

    qCDebug(qLcMediaPlayer) << "Removed pad" << pad.name() << "from" << src.name();
    auto track = decoderOutputMap.value(pad.name());
    if (track.isNull())
        return;

    auto ts = std::find_if(std::begin(trackSelectors), std::end(trackSelectors),
                           [&](TrackSelector &ts){ return ts.selector == track.parent(); });
    if (ts == std::end(trackSelectors))
        return;

    qCDebug(qLcMediaPlayer) << "   was linked to pad" << track.name() << "from" << ts->selector.name();
    ts->removeInputPad(track);

    if (ts->trackCount() == 0) {
        removeOutput(*ts);
        if (ts->type == AudioStream)
            audioAvailableChanged(false);
        else if (ts->type == VideoStream)
            videoAvailableChanged(false);
    }

    if (!prerolling)
        tracksChanged();
}

void QGstreamerMediaPlayer::removeAllOutputs()
{
    for (auto &ts : trackSelectors) {
        removeOutput(ts);
        ts.removeAllInputPads();
    }
    audioAvailableChanged(false);
    videoAvailableChanged(false);
}

void QGstreamerMediaPlayer::connectOutput(TrackSelector &ts)
{
    if (ts.isConnected)
        return;

    QGstElement e;
    switch (ts.type) {
    case AudioStream:
        e = gstAudioOutput ? gstAudioOutput->gstElement() : QGstElement{};
        break;
    case VideoStream:
        e = gstVideoOutput ? gstVideoOutput->gstElement() : QGstElement{};
        break;
    case SubtitleStream:
        if (gstVideoOutput)
            gstVideoOutput->linkSubtitleStream(ts.selector);
        break;
    default:
        return;
    }

    if (!e.isNull()) {
        qCDebug(qLcMediaPlayer) << "connecting output for track type" << ts.type;
        playerPipeline.add(e);
        ts.selector.link(e);
        e.setState(GST_STATE_PAUSED);
    }

    ts.isConnected = true;
}

void QGstreamerMediaPlayer::removeOutput(TrackSelector &ts)
{
    if (!ts.isConnected)
        return;

    QGstElement e;
    switch (ts.type) {
    case AudioStream:
        e = gstAudioOutput ? gstAudioOutput->gstElement() : QGstElement{};
        break;
    case VideoStream:
        e = gstVideoOutput ? gstVideoOutput->gstElement() : QGstElement{};
        break;
    case SubtitleStream:
        if (gstVideoOutput)
            gstVideoOutput->unlinkSubtitleStream();
        break;
    default:
        break;
    }

    if (!e.isNull()) {
        qCDebug(qLcMediaPlayer) << "removing output for track type" << ts.type;
        playerPipeline.remove(e);
        e.setStateSync(GST_STATE_NULL);
    }

    ts.isConnected = false;
}

void QGstreamerMediaPlayer::uridecodebinElementAddedCallback(GstElement */*uridecodebin*/, GstElement *child, QGstreamerMediaPlayer *that)
{
    QGstElement c(child);
    qCDebug(qLcMediaPlayer) << "New element added to uridecodebin:" << c.name();

    if (G_OBJECT_TYPE(child) == that->decodebinType) {
        qCDebug(qLcMediaPlayer) << "     -> setting post-stream-topology property";
        c.set("post-stream-topology", true);
    }
}

void QGstreamerMediaPlayer::sourceSetupCallback(GstElement *uridecodebin, GstElement *source, QGstreamerMediaPlayer *that)
{
    Q_UNUSED(uridecodebin)
    Q_UNUSED(that)

    qCDebug(qLcMediaPlayer) << "Setting up source:" << g_type_name_from_instance((GTypeInstance*)source);

    if (QLatin1String("GstRTSPSrc") == QString::fromUtf8(g_type_name_from_instance((GTypeInstance*)source))) {
        QGstElement s(source);
        int latency{40};
        bool ok{false};
        int v = QString::fromLocal8Bit(qgetenv("QT_MEDIA_RTSP_LATENCY")).toUInt(&ok);
        if (ok)
            latency = v;
        qCDebug(qLcMediaPlayer) << "    -> setting source latency to:" << latency << "ms";
        s.set("latency", latency);

        bool drop{true};
        v = QString::fromLocal8Bit(qgetenv("QT_MEDIA_RTSP_DROP_ON_LATENCY")).toUInt(&ok);
        if (ok && v == 0)
            drop = false;
        qCDebug(qLcMediaPlayer) << "    -> setting drop-on-latency to:" << drop;
        s.set("drop-on-latency", drop);

        bool retrans{false};
        v = QString::fromLocal8Bit(qgetenv("QT_MEDIA_RTSP_DO_RETRANSMISSION")).toUInt(&ok);
        if (ok && v not_eq 0)
            retrans = true;
        qCDebug(qLcMediaPlayer) << "    -> setting do-retransmission to:" << retrans;
        s.set("do-retransmission", retrans);
    }
}

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << "setting location to" << content;

    prerolling = true;

    bool ret = playerPipeline.setStateSync(GST_STATE_NULL);
    if (!ret)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";

    m_url = content;
    m_stream = stream;

    if (!src.isNull())
        playerPipeline.remove(src);
    if (!decoder.isNull())
        playerPipeline.remove(decoder);
    src = QGstElement();
    decoder = QGstElement();
    removeAllOutputs();
    seekableChanged(false);
    playerPipeline.setInStoppedState(true);

    if (m_duration != 0) {
        m_duration = 0;
        durationChanged(0);
    }
    stateChanged(QMediaPlayer::StoppedState);
    if (position() != 0)
        positionChanged(0);
    mediaStatusChanged(QMediaPlayer::NoMedia);
    if (!m_metaData.isEmpty()) {
        m_metaData.clear();
        metaDataChanged();
    }

    if (content.isEmpty())
        return;

    if (m_stream) {
        if (!m_appSrc) {
            auto maybeAppSrc = QGstAppSrc::create(this);
            if (maybeAppSrc) {
                m_appSrc = maybeAppSrc.value();
            } else {
                emit error(QMediaPlayer::ResourceError, maybeAppSrc.error());
                return;
            }
        }
        src = m_appSrc->element();
        decoder = QGstElement("decodebin", "decoder");
        if (!decoder) {
            emit error(QMediaPlayer::ResourceError, errorMessageCannotFindElement("decodebin"));
            return;
        }
        decoder.set("post-stream-topology", true);
        playerPipeline.add(src, decoder);
        src.link(decoder);

        m_appSrc->setup(m_stream);
        seekableChanged(!stream->isSequential());
    } else {
        // use uridecodebin
        decoder = QGstElement("uridecodebin", "uridecoder");
        if (!decoder) {
            emit error(QMediaPlayer::ResourceError, errorMessageCannotFindElement("uridecodebin"));
            return;
        }
        playerPipeline.add(decoder);
        // can't set post-stream-topology to true, as uridecodebin doesn't have the property. Use a hack
        decoder.connect("element-added", GCallback(QGstreamerMediaPlayer::uridecodebinElementAddedCallback), this);
        decoder.connect("source-setup", GCallback(QGstreamerMediaPlayer::sourceSetupCallback), this);

        decoder.set("uri", content.toEncoded().constData());
        if (m_bufferProgress != 0) {
            m_bufferProgress = 0;
            emit bufferProgressChanged(0.);
        }
    }
    decoder.onPadAdded<&QGstreamerMediaPlayer::decoderPadAdded>(this);
    decoder.onPadRemoved<&QGstreamerMediaPlayer::decoderPadRemoved>(this);

    mediaStatusChanged(QMediaPlayer::LoadingMedia);

    if (state() == QMediaPlayer::PlayingState) {
            int ret = playerPipeline.setState(GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE)
                qCWarning(qLcMediaPlayer) << "Unable to set the pipeline to the playing state.";
    } else {
        int ret = playerPipeline.setState(GST_STATE_PAUSED);
        if (!ret)
            qCWarning(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    }

    playerPipeline.setPosition(0);
    positionChanged(0);
}

void QGstreamerMediaPlayer::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    auto &ts = trackSelector(AudioStream);

    playerPipeline.beginConfig();
    if (gstAudioOutput) {
        removeOutput(ts);
        gstAudioOutput->setPipeline({});
    }
    gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstAudioOutput) {
        gstAudioOutput->setPipeline(playerPipeline);
        connectOutput(ts);
    }
    playerPipeline.endConfig();
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
    return m_metaData;
}

void QGstreamerMediaPlayer::setVideoSink(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

static QGstStructure endOfChain(const QGstStructure &s)
{
    QGstStructure e = s;
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
    if (topology.isNull()) {
        qCDebug(qLcMediaPlayer) << "    null topology";
        return;
    }
    auto caps = topology["caps"].toCaps();
    auto structure = caps.at(0);
    auto fileFormat = QGstreamerFormatInfo::fileFormatForCaps(structure);
    qCDebug(qLcMediaPlayer) << caps.toString() << fileFormat;
    m_metaData.insert(QMediaMetaData::FileFormat, QVariant::fromValue(fileFormat));
    m_metaData.insert(QMediaMetaData::Duration, duration());
    m_metaData.insert(QMediaMetaData::Url, m_url);
    QGValue tags = topology["tags"];
    if (!tags.isNull()) {
        GstTagList *tagList = nullptr;
        gst_structure_get(topology.structure, "tags", GST_TYPE_TAG_LIST, &tagList, nullptr);
        const auto metaData = QGstreamerMetaData::fromGstTagList(tagList);
        for (auto k : metaData.keys())
            m_metaData.insert(k, metaData.value(k));
    }

    auto demux = endOfChain(topology);
    auto next = demux["next"];
    if (!next.isList()) {
        qCDebug(qLcMediaPlayer) << "    no additional streams";
        emit metaDataChanged();
        return;
    }

    // collect stream info
    int size = next.listSize();
    for (int i = 0; i < size; ++i) {
        auto val = next.at(i);
        caps = val.toStructure()["caps"].toCaps();
        structure = caps.at(0);
        if (structure.name().startsWith("audio/")) {
            auto codec = QGstreamerFormatInfo::audioCodecForCaps(structure);
            m_metaData.insert(QMediaMetaData::AudioCodec, QVariant::fromValue(codec));
            qCDebug(qLcMediaPlayer) << "    audio" << caps.toString() << (int)codec;
        } else if (structure.name().startsWith("video/")) {
            auto codec = QGstreamerFormatInfo::videoCodecForCaps(structure);
            m_metaData.insert(QMediaMetaData::VideoCodec, QVariant::fromValue(codec));
            qCDebug(qLcMediaPlayer) << "    video" << caps.toString() << (int)codec;
            auto framerate = structure["framerate"].getFraction();
            if (framerate)
                m_metaData.insert(QMediaMetaData::VideoFrameRate, *framerate);
            auto width = structure["width"].toInt();
            auto height = structure["height"].toInt();
            if (width && height)
                m_metaData.insert(QMediaMetaData::Resolution, QSize(*width, *height));
        }
    }

    auto sinkPad = trackSelector(VideoStream).activeInputPad();
    if (!sinkPad.isNull()) {
        bool hasTags = g_object_class_find_property (G_OBJECT_GET_CLASS (sinkPad.object()), "tags") != NULL;

        GstTagList *tl = nullptr;
        g_object_get(sinkPad.object(), "tags", &tl, nullptr);
        qCDebug(qLcMediaPlayer) << "    tags=" << hasTags << (tl ? gst_tag_list_to_string(tl) : "(null)");
    }


    qCDebug(qLcMediaPlayer) << "============== end parse topology ============";
    emit metaDataChanged();
    playerPipeline.dumpGraph("playback");
}

int QGstreamerMediaPlayer::trackCount(QPlatformMediaPlayer::TrackType type)
{
    return trackSelector(type).trackCount();
}

QMediaMetaData QGstreamerMediaPlayer::trackMetaData(QPlatformMediaPlayer::TrackType type, int index)
{
    auto track = trackSelector(type).inputPad(index);
    if (track.isNull())
        return {};

    GstTagList *tagList = nullptr;
    g_object_get(track.object(), "tags", &tagList, nullptr);

    return tagList ? QGstreamerMetaData::fromGstTagList(tagList) : QMediaMetaData{};
}

int QGstreamerMediaPlayer::activeTrack(TrackType type)
{
    return trackSelector(type).activeInputIndex();
}

void QGstreamerMediaPlayer::setActiveTrack(TrackType type, int index)
{
    auto &ts = trackSelector(type);
    auto track = ts.inputPad(index);
    if (track.isNull() && index != -1) {
        qCWarning(qLcMediaPlayer) << "Attempt to set an incorrect index" << index
                                  << "for the track type" << type;
        return;
    }

    qCDebug(qLcMediaPlayer) << "Setting the index" << index << "for the track type" << type;
    if (type == QPlatformMediaPlayer::SubtitleStream)
        gstVideoOutput->flushSubtitles();

    playerPipeline.beginConfig();
    if (track.isNull()) {
        removeOutput(ts);
    } else {
        ts.setActiveInputPad(track);
        connectOutput(ts);
    }
    playerPipeline.endConfig();

    // seek to force an immediate change of the stream
    if (playerPipeline.state() == GST_STATE_PLAYING)
        playerPipeline.flush();
    else
        m_requiresSeekOnPlay = true;
}

QT_END_NAMESPACE

#include "moc_qgstreamermediaplayer_p.cpp"
