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
#include <private/qgstreamerplayersession_p.h>
#include <private/qgstreamerstreamscontrol_p.h>
#include <private/qgstreamervideorenderer_p.h>
#include <private/qgstreamerbushelper_p.h>
#include <private/qgstreamermetadata_p.h>
#include <private/qaudiodeviceinfo_gstreamer_p.h>
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

QGstreamerMediaPlayer::QGstreamerMediaPlayer(QObject *parent)
    : QPlatformMediaPlayer(parent)
{
    audioInputSelector = QGstElement("input-selector", "audioInputSelector");
    audioQueue = QGstElement("queue", "audioQueue");
    audioConvert = QGstElement("audioconvert", "audioConvert");
    audioResample = QGstElement("audioresample", "audioResample");
    audioVolume = QGstElement("volume", "volume");
    audioSink = QGstElement("autoaudiosink", "autoAudioSink");
    playerPipeline.add(audioInputSelector, audioQueue, audioConvert, audioResample, audioVolume, audioSink);
    audioInputSelector.link(audioQueue, audioConvert, audioResample, audioVolume, audioSink);

    videoInputSelector = QGstElement("input-selector", "videoInputSelector");
    videoQueue = QGstElement("queue", "videoQueue");
    videoConvert = QGstElement("videoconvert", "videoConvert");
    videoScale = QGstElement("videoscale", "videoScale");
    playerPipeline.add(videoInputSelector, videoQueue, videoConvert, videoScale);
    videoInputSelector.link(videoQueue, videoConvert, videoScale);

    subTitleInputSelector = QGstElement("input-selector", "subTitleInputSelector");
    playerPipeline.add(subTitleInputSelector);

    playerPipeline.setState(GST_STATE_NULL);

    busHelper = new QGstreamerBusHelper(playerPipeline.bus().bus(), this);
    qRegisterMetaType<QGstreamerMessage>();
    connect(busHelper, &QGstreamerBusHelper::message, this, &QGstreamerMediaPlayer::busMessage);
}

QGstreamerMediaPlayer::~QGstreamerMediaPlayer()
{
    playerPipeline.setStateSync(GST_STATE_NULL);
    if (ownStream)
        delete m_stream;
    if (networkManager)
        delete networkManager;
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
    return m_volume;
}

bool QGstreamerMediaPlayer::isMuted() const
{
    return m_muted;
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
    int ret = playerPipeline.setState(GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";
    m_state = QMediaPlayer::StoppedState;
    emit stateChanged(m_state);
}

void QGstreamerMediaPlayer::setVolume(int vol)
{
    if (vol == m_volume)
        return;
    m_volume = vol;
    audioVolume.set("volume", vol/100.);
    emit volumeChanged(m_volume);
}

void QGstreamerMediaPlayer::setMuted(bool muted)
{
    if (muted == m_muted)
        return;
    m_muted = muted;
    audioVolume.set("mute", muted);
    emit mutedChanged(muted);
}

void QGstreamerMediaPlayer::busMessage(const QGstreamerMessage &message)
{
    if (message.isNull())
        return;

    qCDebug(qLcMediaPlayer) << "received bus message from" << message.source().name() << message.type();
    if (message.source() != playerPipeline && message.source() != decoder)
        return;

    GstMessage* gm = message.rawMessage();
    //tag message comes from elements inside playbin, not from playbin itself
    switch (message.type()) {
    case GST_MESSAGE_TAG: {
        GstTagList *tag_list;
        gst_message_parse_tag(gm, &tag_list);

        m_metaData = QGstreamerMetaData::fromGstTagList(tag_list);

        gst_tag_list_free(tag_list);

        emit metaDataChanged();
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
        return;
    }
    case GST_MESSAGE_EOS:
        stop();
        // anything to do here?
        break;
    case GST_MESSAGE_BUFFERING: {
        int progress = 0;
        gst_message_parse_buffering(gm, &progress);
        m_bufferProgress = progress;
        emit bufferStatusChanged(progress);
        break;
    }
    case GST_MESSAGE_STATE_CHANGED: {
        GstState    oldState;
        GstState    newState;
        GstState    pending;

        gst_message_parse_state_changed(gm, &oldState, &newState, &pending);

#ifdef DEBUG_PLAYBIN
        static QStringList states = {
                  QStringLiteral("GST_STATE_VOID_PENDING"),  QStringLiteral("GST_STATE_NULL"),
                  QStringLiteral("GST_STATE_READY"), QStringLiteral("GST_STATE_PAUSED"),
                  QStringLiteral("GST_STATE_PLAYING") };

        qDebug() << QStringLiteral("state changed: old: %1  new: %2  pending: %3") \
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

            //check for seekable
            if (oldState == GST_STATE_READY) {
//                getStreamsInfo();
//                updateVideoResolutionTag();

                if (!qFuzzyCompare(m_playbackRate, qreal(1.0)))
                    playerPipeline.seek(playerPipeline.position(), m_playbackRate);
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
    default:
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

    QGstElement selector;
    if (type.startsWith("video/x-raw")) {
        selector = videoInputSelector;
    } else if (type.startsWith("audio/x-raw")) {
        selector = audioInputSelector;
    } else if (type.startsWith("text/")) {
        selector = subTitleInputSelector;
    } else {
        qCWarning(qLcMediaPlayer) << "Failed to add fake sink to unknown pad." << pad.name() << type;
        return;
    }
    QGstPad sinkPad = selector.getRequestPad("sink_%u");
    if (!pad.link(sinkPad))
          qCWarning(qLcMediaPlayer) << "Failed to link video pads.";
    decoderOutputMap.insert(pad.name(), sinkPad);
}

void QGstreamerMediaPlayer::decoderPadRemoved(const QGstElement &src, const QGstPad &pad)
{
    qCDebug(qLcMediaPlayer) << "Removed pad" << pad.name() << "from" << src.name();
    QGstPad peer = decoderOutputMap.value(pad.name());
    if (peer.isNull())
        return;
    QGstElement peerParent = peer.parent();
    qCDebug(qLcMediaPlayer) << "   was linked to pad" << peer.name() << "from" << peerParent.name();
    peerParent.releaseRequestPad(peer);
}

void QGstreamerMediaPlayer::setMedia(const QUrl &content, QIODevice *stream)
{
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << "setting location to" << content;

    int ret = playerPipeline.setStateSync(GST_STATE_NULL);
    if (ret == GST_STATE_CHANGE_FAILURE)
        qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the stopped state.";

    m_url = content;
    m_stream = stream;

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
        playerPipeline.add(src, decoder);
        src.link(decoder);

        m_appSrc->setStream(m_stream);
        m_appSrc->setup(src.element());
    } else {
        // use uridecodebin
        decoder = QGstElement("uridecodebin", "uridecoder");
        playerPipeline.add(decoder);

        decoder.set("uri", content.toEncoded().constData());
        if (m_bufferProgress != -1) {
            m_bufferProgress = -1;
            emit bufferStatusChanged(0);
        }
    }
    decoder.onPadAdded<&QGstreamerMediaPlayer::decoderPadAdded>(this);
    decoder.onPadRemoved<&QGstreamerMediaPlayer::decoderPadRemoved>(this);

    if (m_state == QMediaPlayer::PausedState) {
        int ret = playerPipeline.setState(GST_STATE_PAUSED);
        if (ret == GST_STATE_CHANGE_FAILURE)
            qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the paused state.";
    } else if (m_state == QMediaPlayer::PlayingState) {
        int ret = playerPipeline.setState(GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE)
            qCDebug(qLcMediaPlayer) << "Unable to set the pipeline to the playing state.";
    }

    emit positionChanged(position());
}

bool QGstreamerMediaPlayer::setAudioOutput(const QAudioDeviceInfo &info)
{
    if (info == m_audioOutput)
        return true;
    qCDebug(qLcMediaPlayer) << "setAudioOutput" << info.description() << info.isNull();
    m_audioOutput = info;

    if (m_state == QMediaPlayer::StoppedState)
        return changeAudioOutput();

    auto pad = audioVolume.staticPad("src");
    pad.addProbe<&QGstreamerMediaPlayer::prepareAudioOutputChange>(this, GST_PAD_PROBE_TYPE_BLOCK_DOWNSTREAM);

    return true;
}

bool QGstreamerMediaPlayer::changeAudioOutput()
{
    qCDebug(qLcMediaPlayer) << "Changing audio output";
    QGstElement newSink;
    auto *deviceInfo = static_cast<const QGStreamerAudioDeviceInfo *>(m_audioOutput.handle());
    if (deviceInfo && deviceInfo->gstDevice)
        newSink = QGstElement(gst_device_create_element(deviceInfo->gstDevice , "audiosink"), QGstElement::NeedsRef);

    if (newSink.isNull())
        newSink = QGstElement("autoaudiosink", "audiosink");

    playerPipeline.remove(audioSink);
    audioSink = newSink;
    playerPipeline.add(audioSink);
    audioVolume.link(audioSink);

    return true;
}

void QGstreamerMediaPlayer::prepareAudioOutputChange(const QGstPad &/*pad*/)
{
    qCDebug(qLcMediaPlayer) << "Reconfiguring audio output";

    Q_ASSERT(m_state != QMediaPlayer::StoppedState);

    auto state = playerPipeline.state();
    if (state == GST_STATE_PLAYING)
        playerPipeline.setStateSync(GST_STATE_PAUSED);
    audioSink.setStateSync(GST_STATE_NULL);
    changeAudioOutput();
    audioSink.setStateSync(GST_STATE_PAUSED);
    if (state == GST_STATE_PLAYING)
        playerPipeline.setStateSync(state);

    GST_DEBUG_BIN_TO_DOT_FILE(playerPipeline.bin(), GST_DEBUG_GRAPH_SHOW_ALL, "newAudio.dot");
}

QAudioDeviceInfo QGstreamerMediaPlayer::audioOutput() const
{
    return m_audioOutput;
}

QMediaMetaData QGstreamerMediaPlayer::metaData() const
{
//    return m_session->metaData();
    return QMediaMetaData();
}

void QGstreamerMediaPlayer::updateVideoSink()
{
    qCDebug(qLcMediaPlayer) << "Video sink has changed, reload video output";

    QGstElement newSink;
    if (m_videoOutput && m_videoOutput->isReady())
        newSink = QGstElement(m_videoOutput->videoSink(), QGstObject::NeedsRef);

    if (newSink.isNull())
        newSink = QGstElement("fakesink", "fakevideosink");

    if (newSink == videoSink)
        return;

    qCDebug(qLcMediaPlayer) << "Reconfiguring video output";

    if (m_state == QMediaPlayer::StoppedState) {
        qCDebug(qLcMediaPlayer) << "The pipeline has not started yet";

        //the pipeline has not started yet
        playerPipeline.setState(GST_STATE_NULL);
        if (!videoSink.isNull()) {
            videoSink.setState(GST_STATE_NULL);
            playerPipeline.remove(videoSink);
        }
        videoSink = newSink;
        playerPipeline.add(videoSink);
        if (!videoScale.link(videoSink))
            qCWarning(qLcMediaPlayer) << "Linking new video output failed";

        if (g_object_class_find_property(G_OBJECT_GET_CLASS(videoSink.object()), "show-preroll-frame") != nullptr)
            videoSink.set("show-preroll-frame", true);

//        switch (m_pendingState) {
//        case QMediaPlayer::PausedState:
//            gst_element_set_state(m_playbin, GST_STATE_PAUSED);
//            break;
//        case QMediaPlayer::PlayingState:
//            gst_element_set_state(m_playbin, GST_STATE_PLAYING);
//            break;
//        default:
//            break;
//        }

    } else {
//        if (m_pendingVideoSink) {
//#ifdef DEBUG_PLAYBIN
//            qDebug() << "already waiting for pad to be blocked, just change the pending sink";
//#endif
//            m_pendingVideoSink = videoSink;
//            return;
//        }

//        m_pendingVideoSink = videoSink;

//#ifdef DEBUG_PLAYBIN
//        qDebug() << "Blocking the video output pad...";
//#endif

//        //block pads, async to avoid locking in paused state
//        GstPad *srcPad = gst_element_get_static_pad(m_videoIdentity, "src");
//        this->pad_probe_id = gst_pad_add_probe(srcPad, (GstPadProbeType)(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BLOCKING), block_pad_cb, this, nullptr);
//        gst_object_unref(GST_OBJECT(srcPad));

//        //Unpause the sink to avoid waiting until the buffer is processed
//        //while the sink is paused. The pad will be blocked as soon as the current
//        //buffer is processed.
//        if (m_state == QMediaPlayer::PausedState) {
//#ifdef DEBUG_PLAYBIN
//            qDebug() << "Starting video output to avoid blocking in paused state...";
//#endif
//            gst_element_set_state(m_videoSink, GST_STATE_PLAYING);
//        }
    }
}

void QGstreamerMediaPlayer::setSeekable(bool seekable)
{
    qCDebug(qLcMediaPlayer) << Q_FUNC_INFO << seekable;
    if (seekable == m_seekable)
        return;
    m_seekable = seekable;
    emit seekableChanged(m_seekable);
}

void QGstreamerMediaPlayer::setVideoSurface(QAbstractVideoSurface *surface)
{
    if (!m_videoOutput) {
        m_videoOutput = new QGstreamerVideoRenderer;
#ifdef DEBUG_PLAYBIN
    qDebug() << Q_FUNC_INFO;
#endif
        connect(m_videoOutput, SIGNAL(sinkChanged()),
                this, SLOT(updateVideoRenderer()));
    }

    m_videoOutput->setSurface(surface);
    updateVideoSink();
}

QMediaStreamsControl *QGstreamerMediaPlayer::mediaStreams()
{
//    if (!m_streamsControl)
//        m_streamsControl = new QGstreamerStreamsControl(m_session, this);
    return m_streamsControl;
}

bool QGstreamerMediaPlayer::isAudioAvailable() const
{
//    return m_session->isAudioAvailable();
    return true;
}

bool QGstreamerMediaPlayer::isVideoAvailable() const
{
//    return m_session->isVideoAvailable();
    return true;
}

QT_END_NAMESPACE
