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
#include <private/qgstpipeline_p.h>
#include <private/qgstreamermetadata_p.h>
#include <private/qgstreamerformatinfo_p.h>
#include <private/qgstreameraudiooutput_p.h>
#include <private/qgstreamervideooutput_p.h>
#include "private/qgstreamermessage_p.h"
#include <private/qgstreameraudiodevice_p.h>
#include <private/qgstappsrc_p.h>
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

Q_LOGGING_CATEGORY(qLcMediaPlayer, "qt.multimedia.player")

QT_BEGIN_NAMESPACE

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QMediaPlayer *parent)
    : QObject(parent),
      QPlatformMediaPlayer(parent),
      playerPipeline("playerPipeline")
{
    gstAudioOutput = new QGstreamerAudioOutput(nullptr);
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
    connect(&positionUpdateTimer, &QTimer::timeout, this, &QGstreamerMediaPlayer::updatePosition);
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    playerPipeline.removeMessageFilter(this);
    playerPipeline.setStateSync(GST_STATE_NULL);
    topology.free();
    delete gstAudioOutput;
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

int QGstreamerMediaPlayer::volume() const
{
    return qRound(gstAudioOutput->volume*100.);
}

bool QGstreamerMediaPlayer::isMuted() const
{
    return gstAudioOutput->muted;
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
    qint64 currentPos = playerPipeline.position()/1e6;
    if (pos == currentPos)
        return;
    playerPipeline.finishStateChange();
    playerPipeline.seek(pos*1e6, m_playbackRate);
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << pos << playerPipeline.position()/1e6;
    if (mediaStatus() == QMediaPlayer::EndOfMedia)
        mediaStatusChanged(QMediaPlayer::LoadedMedia);
    positionChanged(pos);
}

void QGstreamerMediaPlayer::play()
{
    if (state() == QMediaPlayer::PlayingState || m_url.isEmpty())
        return;

    *playerPipeline.inStoppedState() = false;
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        playerPipeline.seek(0, m_playbackRate);
        updatePosition();
    }

    qCDebug(qLcMediaPlayer) << "play().";
    int ret = playerPipeline.setState(GST_STATE_PLAYING);
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
    if (*playerPipeline.inStoppedState()) {
        *playerPipeline.inStoppedState() = false;
        playerPipeline.seek(playerPipeline.position(), m_playbackRate);
    }
    int ret = playerPipeline.setState(GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    if (mediaStatus() == QMediaPlayer::EndOfMedia) {
        playerPipeline.seek(0, m_playbackRate);
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

void QGstreamerMediaPlayer::stopOrEOS(bool eos)
{
    positionUpdateTimer.stop();
    *playerPipeline.inStoppedState() = true;
    bool ret = playerPipeline.setStateSync(GST_STATE_PAUSED);
    if (!ret)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";
    if (!eos)
        playerPipeline.seek(0, m_playbackRate);
    updatePosition();
    emit stateChanged(QMediaPlayer::StoppedState);
    mediaStatusChanged(eos ? QMediaPlayer::EndOfMedia : QMediaPlayer::LoadedMedia);
}

void QGstreamerMediaPlayer::setVolume(int vol)
{
    float v = vol/100.;
    if (v == gstAudioOutput->volume)
        return;
    gstAudioOutput->volume = v;
    gstAudioOutput->setVolume(vol/100.);
    volumeChanged(vol);
}

void QGstreamerMediaPlayer::setMuted(bool muted)
{
    if (muted == gstAudioOutput->muted)
        return;
    gstAudioOutput->muted = muted;
    gstAudioOutput->setMuted(muted);
    mutedChanged(muted);
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
            setSeekable(false);
            break;
        case GST_STATE_PAUSED:
        {
            if (prerolling) {
                qCDebug(qLcMediaPlayer) << "Preroll done, setting status to Loaded";
                prerolling = false;
                GST_DEBUG_BIN_TO_DOT_FILE(playerPipeline.bin(), GST_DEBUG_GRAPH_SHOW_ALL, "playerPipeline");

                parseStreamsAndMetadata();

                if (!qFuzzyCompare(m_playbackRate, qreal(1.0)))
                    playerPipeline.seek(playerPipeline.position(), m_playbackRate);

                emit tracksChanged();
                mediaStatusChanged(QMediaPlayer::LoadedMedia);
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

    if (m_streams[streamType].size() == 0)
        removeOutput(TrackType(streamType));

    if (!prerolling)
        emit tracksChanged();
}

void QGstreamerMediaPlayer::removeAllOutputs()
{
    for (int i = 0; i < NTrackTypes; ++i) {
        removeOutput(TrackType(i));
        for (QGstPad pad : qAsConst(m_streams[i])) {
            inputSelector[i].releaseRequestPad(pad);
        }
        m_streams[i].clear();
    }
}

void QGstreamerMediaPlayer::removeOutput(TrackType t)
{
    if (selectorIsConnected[t]) {
        QGstElement e;
        if (t == AudioStream)
            e = gstAudioOutput->gstElement();
        else if (t == VideoStream)
            e = gstVideoOutput->gstElement();
        if (!e.isNull()) {
            qCDebug(qLcMediaPlayer) << "removing output for track type" << t;
            e.setState(GST_STATE_NULL);
            playerPipeline.remove(e);
            selectorIsConnected[t] = false;
        }
    }
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
        if (!m_appSrc)
            m_appSrc = new QGstAppSrc(this);
        src = m_appSrc->element();
        decoder = QGstElement("decodebin", "decoder");
        decoder.set("post-stream-topology", true);
        playerPipeline.add(src, decoder);
        src.link(decoder);

        m_appSrc->setup(m_stream);
    } else {
        // use uridecodebin
        decoder = QGstElement("uridecodebin", "uridecoder");
        playerPipeline.add(decoder);
        // can't set post-stream-topology to true, as uridecodebin doesn't have the property. Use a hack
        decoder.connect("element-added", GCallback(QGstreamerMediaPlayer::uridecodebinElementAddedCallback), this);

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

    positionChanged(0);
}

bool QGstreamerMediaPlayer::setAudioOutput(const QAudioDevice &info)
{
    return gstAudioOutput->setAudioOutput(info);
}

QAudioDevice QGstreamerMediaPlayer::audioOutput() const
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

    QGstPad sinkPad = inputSelector[VideoStream].getObject("active-pad");
    if (!sinkPad.isNull()) {
        bool hasTags = g_object_class_find_property (G_OBJECT_GET_CLASS (sinkPad.object()), "tags") != NULL;

        GstTagList *tl = nullptr;
        g_object_get(sinkPad.object(), "tags", &tl, nullptr);
        qCDebug(qLcMediaPlayer) << "    tags=" << hasTags << (tl ? gst_tag_list_to_string(tl) : "(null)");
    }


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
    if (index >= streams.count())
        return;
    if (type == QPlatformMediaPlayer::SubtitleStream) {
        QGstElement src;
        if (index >= 0)
            src = inputSelector[type];
        gstVideoOutput->linkSubtitleStream(src);
    }
    if (index < 0)
        // ### This should disable the stream for audio/video as well
        return;

    auto &selector = inputSelector[type];
    if (selector.isNull())
        return;
    selector.set("active-pad", streams.at(index));
    // seek to force an immediate change of the stream
    playerPipeline.seek(playerPipeline.position(), m_playbackRate);
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
