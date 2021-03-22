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

#include <private/qgstreamermediaplayer_p.h>
#include <private/qgstreamervideorenderer_p.h>
#include <private/qgstreamerbushelper_p.h>
#include <private/qgstreamermetadata_p.h>
#include <private/qgstreamerformatinfo_p.h>
#include <private/qgstreameraudiooutput_p.h>
#include <private/qgstreamervideooutput_p.h>
#include "private/qgstreamermessage_p.h"
#include <private/qaudiodeviceinfo_gstreamer_p.h>
#include <private/qgstappsrc_p.h>
#include <qaudiodeviceinfo.h>

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

Q_LOGGING_CATEGORY(qLcMediaPlayer, "qt.multimedia.player")

QT_BEGIN_NAMESPACE

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QMediaPlayer *parent)
    : QObject(parent),
      QPlatformMediaPlayer(parent)
{
    gstAudioOutput = new QGstreamerAudioOutput(this);
    gstAudioOutput->setPipeline(playerPipeline);
    connect(gstAudioOutput, &QGstreamerAudioOutput::mutedChanged, this, &QGstreamerMediaPlayer::mutedChangedHandler);
    connect(gstAudioOutput, &QGstreamerAudioOutput::volumeChanged, this, &QGstreamerMediaPlayer::volumeChangedHandler);

    gstVideoOutput = new QGstreamerVideoOutput(this);
    gstVideoOutput->setPipeline(playerPipeline);

    inputSelector[AudioStream] = QGstElement("input-selector", "audioInputSelector");
    inputSelector[VideoStream] = QGstElement("input-selector", "videoInputSelector");
    inputSelector[SubtitleStream] = QGstElement("input-selector", "subTitleInputSelector");

    playerPipeline.add(inputSelector[AudioStream], inputSelector[VideoStream], inputSelector[SubtitleStream]);

    playerPipeline.setState(GST_STATE_NULL);
    playerPipeline.installMessageFilter(this);

    /* Taken from gstdicoverer.c:
     * This is ugly. We get the GType of decodebin so we can quickly detect
     * when a decodebin is added to uridecodebin so we can set the
     * post-stream-topology setting to TRUE */
    auto decodebin = QGstElement("decodebin", nullptr);
    decodebinType = G_OBJECT_TYPE(decodebin.element());
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    playerPipeline.setStateSync(GST_STATE_NULL);
    if (ownStream)
        delete m_stream;
    if (networkManager)
        delete networkManager;
    topology.free();
}

qint64 QGstreamerMediaPlayer::position() const
{
    if (playerPipeline.isNull())
        return 0;

    return playerPipeline.position()/1e6;
}

qint64 QGstreamerMediaPlayer::duration() const
{
    return m_duration;
}

QMediaPlayer::State QGstreamerMediaPlayer::state() const
{
    return m_state;
}

QMediaPlayer::MediaStatus QGstreamerMediaPlayer::mediaStatus() const
{
    return m_mediaStatus;
}

int QGstreamerMediaPlayer::bufferStatus() const
{
    return m_bufferProgress;
}

int QGstreamerMediaPlayer::volume() const
{
    return gstAudioOutput->volume();
}

bool QGstreamerMediaPlayer::isMuted() const
{
    return gstAudioOutput->isMuted();
}

bool QGstreamerMediaPlayer::isSeekable() const
{
    return true;
}

QMediaTimeRange QGstreamerMediaPlayer::availablePlaybackRanges() const
{
    return QMediaTimeRange();
}

qreal QGstreamerMediaPlayer::playbackRate() const
{
    return m_playbackRate;
}

void QGstreamerMediaPlayer::setPlaybackRate(qreal rate)
{
    if (rate == m_playbackRate)
        return;
    m_playbackRate = rate;
    playerPipeline.seek(playerPipeline.position(), m_playbackRate);
    emit playbackRateChanged(rate);
}

void QGstreamerMediaPlayer::setPosition(qint64 pos)
{
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << pos/1000.0;
    playerPipeline.seek(pos*1e6, m_playbackRate);
}

void QGstreamerMediaPlayer::play()
{
    int ret = playerPipeline.setState(GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the playing state.";
    m_state = QMediaPlayer::PlayingState;
    emit stateChanged(m_state);
}

void QGstreamerMediaPlayer::pause()
{
    int ret = playerPipeline.setState(GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    m_state = QMediaPlayer::PausedState;
    emit stateChanged(m_state);
}

void QGstreamerMediaPlayer::stop()
{
    int ret = playerPipeline.setStateSync(GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";
    playerPipeline.seek(0, m_playbackRate);
    m_state = QMediaPlayer::StoppedState;
    emit stateChanged(m_state);
}

void QGstreamerMediaPlayer::setVolume(int vol)
{
    gstAudioOutput->setVolume(vol);
}

void QGstreamerMediaPlayer::setMuted(bool muted)
{
    gstAudioOutput->setMuted(muted);
}

bool QGstreamerMediaPlayer::processBusMessage(const QGstreamerMessage &message)
{
    if (message.isNull())
        return false;

    qCDebug(qLcMediaPlayer) << "received bus message from" << message.source().name() << message.type() << (message.type() == GST_MESSAGE_TAG);

    GstMessage* gm = message.rawMessage();
    switch (message.type()) {
    case GST_MESSAGE_TAG: {
        // #### This isn't ideal. We shouldn't catch stream specific tags here, rather the global ones
        GstTagList *tag_list;
        gst_message_parse_tag(gm, &tag_list);
        qCDebug(qLcMediaPlayer) << "Got tags: " << message.source().name() << gst_tag_list_to_string(tag_list);
        auto metaData = QGstreamerMetaData::fromGstTagList(tag_list);
        for (auto k : metaData.keys())
            m_metaData.insert(k, metaData.value(k));
        break;
    }
    case GST_MESSAGE_ASYNC_DONE:
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
        stop();
        // anything to do here?
        break;
    case GST_MESSAGE_BUFFERING: {
        qCDebug(qLcMediaPlayer) << "    buffering message";
        int progress = 0;
        gst_message_parse_buffering(gm, &progress);
        m_bufferProgress = progress;
        emit bufferStatusChanged(progress);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
        if (message.source() != playerPipeline)
            return false;

        GstState    oldState;
        GstState    newState;
        GstState    pending;
        qCDebug(qLcMediaPlayer) << "    state changed message";

        gst_message_parse_state_changed(gm, &oldState, &newState, &pending);

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
            setSeekable(false);
            if (m_state != QMediaPlayer::StoppedState)
                emit stateChanged(m_state = QMediaPlayer::StoppedState);
            break;
        case GST_STATE_PAUSED:
        {
            QMediaPlayer::State prevState = m_state;
            m_state = QMediaPlayer::PausedState;

            if (prerolling) {
                prerolling = false;
                GST_DEBUG_BIN_TO_DOT_FILE(playerPipeline.bin(), GST_DEBUG_GRAPH_SHOW_ALL, "playerPipeline");

                parseStreamsAndMetadata();

                if (!qFuzzyCompare(m_playbackRate, qreal(1.0)))
                    playerPipeline.seek(playerPipeline.position(), m_playbackRate);

                emit tracksChanged();
            }

            if (m_state != prevState)
                emit stateChanged(m_state);

            break;
        }
        case GST_STATE_PLAYING:
            if (m_state != QMediaPlayer::PlayingState)
                emit stateChanged(m_state = QMediaPlayer::PlayingState);
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
        g_error_free(err);
        g_free(debug);
        break;
    }
    case GST_MESSAGE_WARNING: {
        GError *err;
        gchar *debug;
        gst_message_parse_warning (gm, &err, &debug);
        qCWarning(qLcMediaPlayer) << "Warning:" << QString::fromUtf8(err->message);
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
        const GstStructure *structure = gst_message_get_structure(gm);
        qint64 position = g_value_get_int64(gst_structure_get_value(structure, "position"));
        position /= 1000000;
        emit positionChanged(position);
    }
    case GST_MESSAGE_ELEMENT: {
        QGstStructure structure(gst_message_get_structure(gm));
        auto type = structure.name();
        if (type == "stream-topology") {
            topology.free();
            topology = structure.copy();
        }
    }

    default:
        qCDebug(qLcMediaPlayer) << "    default message handler, doing nothing";

        break;
    }

#if 0
    } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
        GError *err;
        gchar *debug;
        gst_message_parse_error(gm, &err, &debug);
        // If the source has given up, so do we.
        if (qstrcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(gm)), "source") == 0) {
            bool everPlayed = m_everPlayed;
            // Try and differentiate network related resource errors from the others
            if (!m_request.url().isRelative() && m_request.url().scheme().compare(QLatin1String("file"), Qt::CaseInsensitive) != 0 ) {
                if (everPlayed ||
                    (err->domain == GST_RESOURCE_ERROR && (
                     err->code == GST_RESOURCE_ERROR_BUSY ||
                     err->code == GST_RESOURCE_ERROR_OPEN_READ ||
                     err->code == GST_RESOURCE_ERROR_READ ||
                     err->code == GST_RESOURCE_ERROR_SEEK ||
                     err->code == GST_RESOURCE_ERROR_SYNC))) {
                    processInvalidMedia(QMediaPlayer::NetworkError, QString::fromUtf8(err->message));
                } else {
                    processInvalidMedia(QMediaPlayer::ResourceError, QString::fromUtf8(err->message));
                }
            }
            else
                processInvalidMedia(QMediaPlayer::ResourceError, QString::fromUtf8(err->message));
        } else if (err->domain == GST_STREAM_ERROR
                   && (err->code == GST_STREAM_ERROR_DECRYPT || err->code == GST_STREAM_ERROR_DECRYPT_NOKEY)) {
            processInvalidMedia(QMediaPlayer::AccessDeniedError, QString::fromUtf8(err->message));
        } else {
            handlePlaybin2 = true;
        }
        if (!handlePlaybin2)
            qWarning() << "Error:" << QString::fromUtf8(err->message);
        g_error_free(err);
        g_free(debug);
    } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ELEMENT
               && qstrcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(gm)), "source") == 0
               && m_sourceType == UDPSrc
               && gst_structure_has_name(gst_message_get_structure(gm), "GstUDPSrcTimeout")) {
        //since udpsrc will not generate an error for the timeout event,
        //we need to process its element message here and treat it as an error.
        processInvalidMedia(m_everPlayed ? QMediaPlayer::NetworkError : QMediaPlayer::ResourceError,
                            tr("UDP source timeout"));
    } else {
        handlePlaybin2 = true;
    }
    if (handlePlaybin2) {
        if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_WARNING) {
            GError *err;
            gchar *debug;
            gst_message_parse_warning(gm, &err, &debug);
            if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND)
                emit error(int(QMediaPlayer::FormatError), tr("Cannot play stream of type: <unknown>"));
            // GStreamer shows warning for HTTP playlists
            if (err && err->message)
                qWarning() << "Warning:" << QString::fromUtf8(err->message);
            g_error_free(err);
            g_free(debug);
        } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
            GError *err;
            gchar *debug;
            gst_message_parse_error(gm, &err, &debug);

            // Nearly all errors map to ResourceError
            QMediaPlayer::Error qerror = QMediaPlayer::ResourceError;
            if (err->domain == GST_STREAM_ERROR
                       && (err->code == GST_STREAM_ERROR_DECRYPT
                           || err->code == GST_STREAM_ERROR_DECRYPT_NOKEY)) {
                qerror = QMediaPlayer::AccessDeniedError;
            }
            processInvalidMedia(qerror, QString::fromUtf8(err->message));
            if (err && err->message)
                qWarning() << "Error:" << QString::fromUtf8(err->message);

            g_error_free(err);
            g_free(debug);
        }
    }
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
    qCDebug(qLcMediaPlayer) << "    " << caps.toString();

    TrackType streamType = NTrackTypes;
    QGstElement output;
    if (type.startsWith("video/x-raw")) {
        streamType = VideoStream;
        output = gstVideoOutput->gstElement();
    } else if (type.startsWith("audio/x-raw")) {
        streamType = AudioStream;
        output = gstAudioOutput->gstElement();
    } else if (type.startsWith("text/")) {
        streamType = SubtitleStream;
    } else {
        qCWarning(qLcMediaPlayer) << "Ignoring unknown media stream:" << pad.name() << type;
        return;
    }
    if (!selectorIsConnected[streamType] && !output.isNull()) {
        playerPipeline.add(output);
        inputSelector[streamType].link(output);
        output.setState(GST_STATE_PAUSED);
        selectorIsConnected[streamType] = true;
    }

    QGstPad sinkPad = inputSelector[streamType].getRequestPad("sink_%u");
    if (!pad.link(sinkPad))
          qCWarning(qLcMediaPlayer) << "Failed to link video pads.";
    m_streams[streamType].append(sinkPad);

    if (m_streams[streamType].size() == 1) {
        if (streamType == VideoStream)
            emit videoAvailableChanged(true);
        else if (streamType == AudioStream)
            emit audioAvailableChanged(true);
    }

    if (!prerolling)
        emit tracksChanged();

    decoderOutputMap.insert(pad.name(), sinkPad);
}

void QGstreamerMediaPlayer::decoderPadRemoved(const QGstElement &src, const QGstPad &pad)
{
    int streamType;
    for (streamType = 0; streamType < NTrackTypes; ++streamType) {
        if (src == inputSelector[streamType])
            break;
    }
    if (streamType == NTrackTypes)
        return;

    qCDebug(qLcMediaPlayer) << "Removed pad" << pad.name() << "from" << src.name();
    QGstPad peer = decoderOutputMap.value(pad.name());
    if (peer.isNull())
        return;
    QGstElement peerParent = peer.parent();
    qCDebug(qLcMediaPlayer) << "   was linked to pad" << peer.name() << "from" << peerParent.name();
    peerParent.releaseRequestPad(peer);

    Q_ASSERT(m_streams[streamType].indexOf(peer) != -1);
    m_streams[streamType].removeAll(peer);

    if (m_streams[streamType].size() == 0) {
        QGstElement outputToRemove;
        if (streamType == VideoStream) {
            emit videoAvailableChanged(false);
            outputToRemove = gstVideoOutput->gstElement();
        } else if (streamType == AudioStream) {
            emit audioAvailableChanged(false);
            outputToRemove = gstAudioOutput->gstElement();
        }
        if (!outputToRemove.isNull()) {
            outputToRemove.setState(GST_STATE_NULL);
            playerPipeline.remove(outputToRemove);
            selectorIsConnected[streamType] = false;
        }
    }


    if (!prerolling)
        emit tracksChanged();
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

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << "setting location to" << content;

    prerolling = true;

    int ret = playerPipeline.setStateSync(GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";

    m_url = content;
    m_stream = stream;
    m_metaData.clear();

    if (!src.isNull())
        playerPipeline.remove(src);
    if (!decoder.isNull())
        playerPipeline.remove(decoder);
    src = QGstElement();
    decoder = QGstElement();

    if (m_stream) {
        if (!m_appSrc)
            m_appSrc = new QGstAppSrc(this);
        src = QGstElement("appsrc", "appsrc");
        decoder = QGstElement("decodebin", "decoder");
        decoder.set("post-stream-topology", true);
        playerPipeline.add(src, decoder);
        src.link(decoder);

        m_appSrc->setStream(m_stream);
        m_appSrc->setup(src.element());
    } else {
        // use uridecodebin
        decoder = QGstElement("uridecodebin", "uridecoder");
        playerPipeline.add(decoder);
        // can't set post-stream-topology to true, as uridecodebin doesn't have the property. Use a hack
        decoder.connect("element-added", GCallback(QGstreamerMediaPlayer::uridecodebinElementAddedCallback), this);

        decoder.set("uri", content.toEncoded().constData());
        if (m_bufferProgress != -1) {
            m_bufferProgress = -1;
            emit bufferStatusChanged(0);
        }
    }
    decoder.onPadAdded<&QGstreamerMediaPlayer::decoderPadAdded>(this);
    decoder.onPadRemoved<&QGstreamerMediaPlayer::decoderPadRemoved>(this);

    if (m_state == QMediaPlayer::PlayingState) {
            int ret = playerPipeline.setState(GST_STATE_PLAYING);
            if (ret == GST_STATE_CHANGE_FAILURE)
                qCWarning(qLcMediaPlayer) << "Unable to set the pipeline to the playing state.";
    } else {
        int ret = playerPipeline.setState(GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE)
            qCWarning(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    }

    emit positionChanged(position());
}

bool QGstreamerMediaPlayer::setAudioOutput(const QAudioDeviceInfo &info)
{
    return gstAudioOutput->setAudioOutput(info);
}

QAudioDeviceInfo QGstreamerMediaPlayer::audioOutput() const
{
    return gstAudioOutput->audioOutput();
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
    return m_metaData;
}

void QGstreamerMediaPlayer::setVideoSink(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

void QGstreamerMediaPlayer::setSeekable(bool seekable)
{
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << seekable;
    if (seekable == m_seekable)
        return;
    m_seekable = seekable;
    emit seekableChanged(m_seekable);
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
    if (!next.isList())
        return;

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

    QGstPad sinkPad = inputSelector[VideoStream].getObject("active-pad");
    Q_ASSERT(!sinkPad.isNull());
    bool hasTags = g_object_class_find_property (G_OBJECT_GET_CLASS (sinkPad.object()), "tags") != NULL;

    GstTagList *tl = nullptr;
    g_object_get(sinkPad.object(), "tags", &tl, nullptr);
    qCDebug(qLcMediaPlayer) << "    tags=" << hasTags << (tl ? gst_tag_list_to_string(tl) : "(null)");


    qCDebug(qLcMediaPlayer) << "============== end parse topology ============";
    emit metaDataChanged();
}

int QGstreamerMediaPlayer::trackCount(QPlatformMediaPlayer::TrackType type)
{
    return m_streams[type].count();
}

QMediaMetaData QGstreamerMediaPlayer::trackMetaData(QPlatformMediaPlayer::TrackType type, int index)
{
    auto &s = m_streams[type];
    if (index < 0 || index >= s.count())
        return QMediaMetaData();

    GstTagList *tagList;
    g_object_get(s.at(index).object(), "tags", &tagList, nullptr);

    QMediaMetaData md = QGstreamerMetaData::fromGstTagList(tagList);
    return md;
}

int QGstreamerMediaPlayer::activeTrack(QPlatformMediaPlayer::TrackType type)
{
    auto &selector = inputSelector[type];
    if (selector.isNull())
        return -1;
    QGstPad activePad = selector.getObject("active-pad");
    if (activePad.isNull())
        return -1;
    auto &streams = m_streams[type];
    return streams.indexOf(activePad);
}

void QGstreamerMediaPlayer::setActiveTrack(QPlatformMediaPlayer::TrackType type, int index)
{
    auto &streams = m_streams[type];
    if (index < 0 || index >= streams.count())
        return;
    auto &selector = inputSelector[type];
    if (selector.isNull())
        return;
    selector.set("active-pad", streams.at(index));
}

bool QGstreamerMediaPlayer::isAudioAvailable() const
{
    return !m_streams[AudioStream].isEmpty();
}

bool QGstreamerMediaPlayer::isVideoAvailable() const
{
    return !m_streams[VideoStream].isEmpty();
}

QT_END_NAMESPACE
