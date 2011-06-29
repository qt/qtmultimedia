/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamerplayersession.h"
#include "qgstreamerbushelper.h"

#include "qgstreamervideorendererinterface.h"
#include "gstvideoconnector.h"
#include "qgstutils.h"

#include <gst/gstvalue.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsize.h>
#include <QtCore/qtimer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtGui/qdesktopservices.h>

#if defined(Q_WS_MAEMO_5) || defined(Q_WS_MAEMO_6) || (GST_VERSION_MICRO > 20)
#define USE_PLAYBIN2
#endif

//#define DEBUG_PLAYBIN
//#define DEBUG_VO_BIN_DUMP

typedef enum {
    GST_PLAY_FLAG_VIDEO         = 0x00000001,
    GST_PLAY_FLAG_AUDIO         = 0x00000002,
    GST_PLAY_FLAG_TEXT          = 0x00000004,
    GST_PLAY_FLAG_VIS           = 0x00000008,
    GST_PLAY_FLAG_SOFT_VOLUME   = 0x00000010,
    GST_PLAY_FLAG_NATIVE_AUDIO  = 0x00000020,
    GST_PLAY_FLAG_NATIVE_VIDEO  = 0x00000040,
    GST_PLAY_FLAG_DOWNLOAD      = 0x00000080,
    GST_PLAY_FLAG_BUFFERING     = 0x000000100
} GstPlayFlags;

QGstreamerPlayerSession::QGstreamerPlayerSession(QObject *parent)
    :QObject(parent),
     m_state(QMediaPlayer::StoppedState),
     m_pendingState(QMediaPlayer::StoppedState),
     m_busHelper(0),
     m_playbin(0),
     m_usePlaybin2(false),
     m_usingColorspaceElement(false),
     m_videoSink(0),
     m_pendingVideoSink(0),
     m_nullVideoSink(0),
     m_bus(0),
     m_videoOutput(0),
     m_renderer(0),
     m_haveQueueElement(false),
#if defined(HAVE_GST_APPSRC)
     m_appSrc(0),
#endif
     m_volume(100),
     m_playbackRate(1.0),
     m_muted(false),
     m_audioAvailable(false),
     m_videoAvailable(false),
     m_seekable(false),
     m_lastPosition(0),
     m_duration(-1),
     m_durationQueries(0),
     m_everPlayed(false) ,
     m_sourceType(UnknownSrc)
{
#ifdef USE_PLAYBIN2
    m_playbin = gst_element_factory_make("playbin2", NULL);
#endif

    if (m_playbin) {
        m_usePlaybin2 = true;

        //GST_PLAY_FLAG_NATIVE_VIDEO omits configuration of ffmpegcolorspace and videoscale,
        //since those elements are included in the video output bin when necessary.
#ifdef Q_WS_MAEMO_6
        int flags = GST_PLAY_FLAG_VIDEO | GST_PLAY_FLAG_AUDIO |
                    GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_NATIVE_AUDIO;
#else
        int flags = 0;
        g_object_get(G_OBJECT(m_playbin), "flags", &flags, NULL);
        flags |= GST_PLAY_FLAG_NATIVE_VIDEO;
#endif
        g_object_set(G_OBJECT(m_playbin), "flags", flags, NULL);
    } else {
        m_usePlaybin2 = false;
        m_playbin = gst_element_factory_make("playbin", NULL);
    }

    m_videoOutputBin = gst_bin_new("video-output-bin");
    gst_object_ref(GST_OBJECT(m_videoOutputBin));

    m_videoIdentity = GST_ELEMENT(g_object_new(gst_video_connector_get_type(), 0));
    g_signal_connect(G_OBJECT(m_videoIdentity), "connection-failed", G_CALLBACK(insertColorSpaceElement), (gpointer)this);
    m_colorSpace = gst_element_factory_make("ffmpegcolorspace", "ffmpegcolorspace-vo");
    gst_object_ref(GST_OBJECT(m_colorSpace));

    m_nullVideoSink = gst_element_factory_make("fakesink", NULL);
    g_object_set(G_OBJECT(m_nullVideoSink), "sync", true, NULL);
    gst_object_ref(GST_OBJECT(m_nullVideoSink));
    gst_bin_add_many(GST_BIN(m_videoOutputBin), m_videoIdentity, m_nullVideoSink, NULL);
    gst_element_link(m_videoIdentity, m_nullVideoSink);

    m_videoSink = m_nullVideoSink;

    // add ghostpads
    GstPad *pad = gst_element_get_static_pad(m_videoIdentity,"sink");
    gst_element_add_pad(GST_ELEMENT(m_videoOutputBin), gst_ghost_pad_new("videosink", pad));
    gst_object_unref(GST_OBJECT(pad));

    if (m_playbin != 0) {
        // Sort out messages
        m_bus = gst_element_get_bus(m_playbin);
        m_busHelper = new QGstreamerBusHelper(m_bus, this);
        connect(m_busHelper, SIGNAL(message(QGstreamerMessage)), SLOT(busMessage(QGstreamerMessage)));
        m_busHelper->installSyncEventFilter(this);

        g_object_set(G_OBJECT(m_playbin), "video-sink", m_videoOutputBin, NULL);

        g_signal_connect(G_OBJECT(m_playbin), "notify::source", G_CALLBACK(playbinNotifySource), this);
        g_signal_connect(G_OBJECT(m_playbin), "element-added",  G_CALLBACK(handleElementAdded), this);

        // Initial volume
        double volume = 1.0;
        g_object_get(G_OBJECT(m_playbin), "volume", &volume, NULL);
        m_volume = int(volume*100);

        g_signal_connect(G_OBJECT(m_playbin), "notify::volume", G_CALLBACK(handleVolumeChange), this);
        if (m_usePlaybin2)
            g_signal_connect(G_OBJECT(m_playbin), "notify::mute", G_CALLBACK(handleMutedChange), this);
    }
}

QGstreamerPlayerSession::~QGstreamerPlayerSession()
{
    if (m_playbin) {
        stop();

        delete m_busHelper;
        gst_object_unref(GST_OBJECT(m_bus));
        gst_object_unref(GST_OBJECT(m_playbin));
        gst_object_unref(GST_OBJECT(m_colorSpace));
        gst_object_unref(GST_OBJECT(m_nullVideoSink));
        gst_object_unref(GST_OBJECT(m_videoOutputBin));
    }
}

#if defined(HAVE_GST_APPSRC)
void QGstreamerPlayerSession::configureAppSrcElement(GObject* object, GObject *orig, GParamSpec *pspec, QGstreamerPlayerSession* self)
{
    if (self->appsrc()->isReady())
        return;

    GstElement *appsrc;
    g_object_get(orig, "source", &appsrc, NULL);

    if (!self->appsrc()->setup(appsrc))
        qWarning()<<"Could not setup appsrc element";
}
#endif

void QGstreamerPlayerSession::loadFromStream(const QNetworkRequest &request, QIODevice *appSrcStream)
{
#if defined(HAVE_GST_APPSRC)
    m_request = request;
    m_duration = -1;
    m_lastPosition = 0;
    m_haveQueueElement = false;

    if (m_appSrc)
        m_appSrc->deleteLater();
    m_appSrc = new QGstAppSrc(this);
    m_appSrc->setStream(appSrcStream);

    if (m_playbin) {
        m_tags.clear();
        emit tagsChanged();

        g_signal_connect(G_OBJECT(m_playbin), "deep-notify::source", (GCallback) &QGstreamerPlayerSession::configureAppSrcElement, (gpointer)this);
        g_object_set(G_OBJECT(m_playbin), "uri", "appsrc://", NULL);

        if (!m_streamTypes.isEmpty()) {
            m_streamProperties.clear();
            m_streamTypes.clear();

            emit streamsChanged();
        }
    }
#endif
}

void QGstreamerPlayerSession::loadFromUri(const QNetworkRequest &request)
{
    m_request = request;
    m_duration = -1;
    m_lastPosition = 0;
    m_haveQueueElement = false;

    if (m_playbin) {
        m_tags.clear();
        emit tagsChanged();

        g_object_set(G_OBJECT(m_playbin), "uri", m_request.url().toEncoded().constData(), NULL);

        if (!m_streamTypes.isEmpty()) {
            m_streamProperties.clear();
            m_streamTypes.clear();

            emit streamsChanged();
        }
    }
}

qint64 QGstreamerPlayerSession::duration() const
{
    return m_duration;
}

qint64 QGstreamerPlayerSession::position() const
{
    GstFormat   format = GST_FORMAT_TIME;
    gint64      position = 0;

    if ( m_playbin && gst_element_query_position(m_playbin, &format, &position))
        m_lastPosition = position / 1000000;

    return m_lastPosition;
}

qreal QGstreamerPlayerSession::playbackRate() const
{
    return m_playbackRate;
}

void QGstreamerPlayerSession::setPlaybackRate(qreal rate)
{
    if (!qFuzzyCompare(m_playbackRate, rate)) {
        m_playbackRate = rate;
        if (m_playbin) {
            gst_element_seek(m_playbin, rate, GST_FORMAT_TIME,
                             GstSeekFlags(GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH),
                             GST_SEEK_TYPE_NONE,0,
                             GST_SEEK_TYPE_NONE,0 );
        }
        emit playbackRateChanged(m_playbackRate);
    }
}

QMediaTimeRange QGstreamerPlayerSession::availablePlaybackRanges() const
{
    QMediaTimeRange ranges;
#if (GST_VERSION_MAJOR >= 0) &&  (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 31)
    //GST_FORMAT_TIME would be more appropriate, but unfortunately it's not supported.
    //with GST_FORMAT_PERCENT media is treated as encoded with constant bitrate.
    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (gst_element_query(m_playbin, query)) {
        for (guint index = 0; index < gst_query_get_n_buffering_ranges(query); index++) {
            gint64 rangeStart = 0;
            gint64 rangeStop = 0;

            //This query should return values in GST_FORMAT_PERCENT_MAX range,
            //but queue2 returns values in 0..100 range instead
            if (gst_query_parse_nth_buffering_range(query, index, &rangeStart, &rangeStop))
                ranges.addInterval(rangeStart * duration() / 100,
                                   rangeStop * duration() / 100);
        }
    }

    gst_query_unref(query);
#endif

    //without queue2 element in pipeline all the media is considered available
    if (ranges.isEmpty() && duration() > 0 && !m_haveQueueElement)
        ranges.addInterval(0, duration());

#ifdef DEBUG_PLAYBIN
    qDebug() << ranges;
#endif

    return ranges;
}

int QGstreamerPlayerSession::activeStream(QMediaStreamsControl::StreamType streamType) const
{
    int streamNumber = -1;
    if (m_playbin) {
        switch (streamType) {
        case QMediaStreamsControl::AudioStream:
            g_object_get(G_OBJECT(m_playbin), "current-audio", streamNumber, NULL);
            break;
        case QMediaStreamsControl::VideoStream:
            g_object_get(G_OBJECT(m_playbin), "current-video", streamNumber, NULL);
            break;
        case QMediaStreamsControl::SubPictureStream:
            g_object_get(G_OBJECT(m_playbin), "current-text", streamNumber, NULL);
            break;
        default:
            break;
        }
    }

    if (m_usePlaybin2 && streamNumber >= 0)
        streamNumber += m_playbin2StreamOffset.value(streamType,0);

    return streamNumber;
}

void QGstreamerPlayerSession::setActiveStream(QMediaStreamsControl::StreamType streamType, int streamNumber)
{

    if (m_usePlaybin2 && streamNumber >= 0)
        streamNumber -= m_playbin2StreamOffset.value(streamType,0);

    if (m_playbin) {
        switch (streamType) {
        case QMediaStreamsControl::AudioStream:
            g_object_set(G_OBJECT(m_playbin), "current-audio", &streamNumber, NULL);
            break;
        case QMediaStreamsControl::VideoStream:
            g_object_set(G_OBJECT(m_playbin), "current-video", &streamNumber, NULL);
            break;
        case QMediaStreamsControl::SubPictureStream:
            g_object_set(G_OBJECT(m_playbin), "current-text", &streamNumber, NULL);
            break;
        default:
            break;
        }
    }
}


bool QGstreamerPlayerSession::isBuffering() const
{
    return false;
}

int QGstreamerPlayerSession::bufferingProgress() const
{
    return 0;
}

int QGstreamerPlayerSession::volume() const
{
    return m_volume;
}

bool QGstreamerPlayerSession::isMuted() const
{
    return m_muted;
}

bool QGstreamerPlayerSession::isAudioAvailable() const
{
    return m_audioAvailable;
}

static void block_pad_cb(GstPad *pad, gboolean blocked, gpointer user_data)
{
    Q_UNUSED(pad);
#ifdef DEBUG_PLAYBIN
    qDebug() << "block_pad_cb, blocked:" << blocked;
#endif

    if (blocked && user_data) {
        QGstreamerPlayerSession *session = reinterpret_cast<QGstreamerPlayerSession*>(user_data);
        QMetaObject::invokeMethod(session, "finishVideoOutputChange", Qt::QueuedConnection);
    }
}

void QGstreamerPlayerSession::updateVideoRenderer()
{
#ifdef DEBUG_PLAYBIN
    qDebug() << "Video sink has chaged, reload video output";
#endif

    if (m_videoOutput)
        setVideoRenderer(m_videoOutput);
}

void QGstreamerPlayerSession::setVideoRenderer(QObject *videoOutput)
{
    if (m_videoOutput != videoOutput) {
        if (m_videoOutput) {
            disconnect(m_videoOutput, SIGNAL(sinkChanged()),
                       this, SLOT(updateVideoRenderer()));
            disconnect(m_videoOutput, SIGNAL(readyChanged(bool)),
                   this, SLOT(updateVideoRenderer()));
        }

        if (videoOutput) {
            connect(videoOutput, SIGNAL(sinkChanged()),
                    this, SLOT(updateVideoRenderer()));
            connect(videoOutput, SIGNAL(readyChanged(bool)),
                   this, SLOT(updateVideoRenderer()));
        }

        m_videoOutput = videoOutput;
    }

    QGstreamerVideoRendererInterface* renderer = qobject_cast<QGstreamerVideoRendererInterface*>(videoOutput);   

    m_renderer = renderer;

#ifdef DEBUG_VO_BIN_DUMP
    _gst_debug_bin_to_dot_file_with_ts(GST_BIN(m_playbin),
                                  GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_ALL /* GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES*/),
                                  "playbin_set");
#endif

    GstElement *videoSink = 0;
    if (m_renderer && m_renderer->isReady())
        videoSink = m_renderer->videoSink();

    if (!videoSink)
        videoSink = m_nullVideoSink;

#ifdef DEBUG_PLAYBIN
    qDebug() << "Set video output:" << videoOutput;
    qDebug() << "Current sink:" << (m_videoSink ? GST_ELEMENT_NAME(m_videoSink) : "") <<  m_videoSink
             << "pending:" << (m_pendingVideoSink ? GST_ELEMENT_NAME(m_pendingVideoSink) : "") << m_pendingVideoSink
             << "new sink:" << (videoSink ? GST_ELEMENT_NAME(videoSink) : "") << videoSink;
#endif

    if (m_pendingVideoSink == videoSink ||
        (m_pendingVideoSink == 0 && m_videoSink == videoSink)) {
#ifdef DEBUG_PLAYBIN
        qDebug() << "Video sink has not changed, skip video output reconfiguration";
#endif
        return;
    }

#ifdef DEBUG_PLAYBIN
    qDebug() << "Reconfigure video output";
#endif

    if (m_state == QMediaPlayer::StoppedState) {
#ifdef DEBUG_PLAYBIN
        qDebug() << "The pipeline has not started yet, pending state:" << m_pendingState;
#endif
        //the pipeline has not started yet
        m_pendingVideoSink = 0;        
        gst_element_set_state(m_videoSink, GST_STATE_NULL);
        gst_element_set_state(m_playbin, GST_STATE_NULL);

        if (m_usingColorspaceElement) {
            gst_element_unlink(m_colorSpace, m_videoSink);
            gst_bin_remove(GST_BIN(m_videoOutputBin), m_colorSpace);
        } else {
            gst_element_unlink(m_videoIdentity, m_videoSink);
        }

        gst_bin_remove(GST_BIN(m_videoOutputBin), m_videoSink);

        m_videoSink = videoSink;

        gst_bin_add(GST_BIN(m_videoOutputBin), m_videoSink);

        m_usingColorspaceElement = false;
        bool linked = gst_element_link(m_videoIdentity, m_videoSink);
        if (!linked) {
            m_usingColorspaceElement = true;
#ifdef DEBUG_PLAYBIN
            qDebug() << "Failed to connect video output, inserting the colorspace element.";
#endif
            gst_bin_add(GST_BIN(m_videoOutputBin), m_colorSpace);
            linked = gst_element_link_many(m_videoIdentity, m_colorSpace, m_videoSink, NULL);
        }

        switch (m_pendingState) {
        case QMediaPlayer::PausedState:
            gst_element_set_state(m_playbin, GST_STATE_PAUSED);
            break;
        case QMediaPlayer::PlayingState:
            gst_element_set_state(m_playbin, GST_STATE_PLAYING);
            break;
        default:
            break;
        }
    } else {
        if (m_pendingVideoSink) {
#ifdef DEBUG_PLAYBIN
            qDebug() << "already waiting for pad to be blocked, just change the pending sink";
#endif
            m_pendingVideoSink = videoSink;
            return;
        }

        m_pendingVideoSink = videoSink;

#ifdef DEBUG_PLAYBIN
        qDebug() << "Blocking the video output pad...";
#endif

        //block pads, async to avoid locking in paused state
        GstPad *srcPad = gst_element_get_static_pad(m_videoIdentity, "src");
        gst_pad_set_blocked_async(srcPad, true, &block_pad_cb, this);
        gst_object_unref(GST_OBJECT(srcPad));

        //Unpause the sink to avoid waiting until the buffer is processed
        //while the sink is paused. The pad will be blocked as soon as the current
        //buffer is processed.
        if (m_state == QMediaPlayer::PausedState) {
#ifdef DEBUG_PLAYBIN
            qDebug() << "Starting video output to avoid blocking in paused state...";
#endif
            gst_element_set_state(m_videoSink, GST_STATE_PLAYING);
        }
    }
}

void QGstreamerPlayerSession::finishVideoOutputChange()
{
    if (!m_pendingVideoSink)
        return;

#ifdef DEBUG_PLAYBIN
    qDebug() << "finishVideoOutputChange" << m_pendingVideoSink;
#endif

    GstPad *srcPad = gst_element_get_static_pad(m_videoIdentity, "src");

    if (!gst_pad_is_blocked(srcPad)) {
        //pad is not blocked, it's possible to swap outputs only in the null state
        qWarning() << "Pad is not blocked yet, could not switch video sink";
        GstState identityElementState = GST_STATE_NULL;
        gst_element_get_state(m_videoIdentity, &identityElementState, NULL, GST_CLOCK_TIME_NONE);
        if (identityElementState != GST_STATE_NULL) {
            gst_object_unref(GST_OBJECT(srcPad));
            return; //can't change vo yet, received async call from the previous change
        }
    }

    if (m_pendingVideoSink == m_videoSink) {
        //video output was change back to the current one,
        //no need to torment the pipeline, just unblock the pad
        if (gst_pad_is_blocked(srcPad))
            gst_pad_set_blocked_async(srcPad, false, &block_pad_cb, 0);

        m_pendingVideoSink = 0;
        gst_object_unref(GST_OBJECT(srcPad));
        return;
    }  

    if (m_usingColorspaceElement) {
        gst_element_set_state(m_colorSpace, GST_STATE_NULL);
        gst_element_set_state(m_videoSink, GST_STATE_NULL);

        gst_element_unlink(m_colorSpace, m_videoSink);
        gst_bin_remove(GST_BIN(m_videoOutputBin), m_colorSpace);
    } else {
        gst_element_set_state(m_videoSink, GST_STATE_NULL);
        gst_element_unlink(m_videoIdentity, m_videoSink);
    }

    gst_bin_remove(GST_BIN(m_videoOutputBin), m_videoSink);

    m_videoSink = m_pendingVideoSink;
    m_pendingVideoSink = 0;

    gst_bin_add(GST_BIN(m_videoOutputBin), m_videoSink);

    m_usingColorspaceElement = false;
    bool linked = gst_element_link(m_videoIdentity, m_videoSink);
    if (!linked) {
        m_usingColorspaceElement = true;
#ifdef DEBUG_PLAYBIN
        qDebug() << "Failed to connect video output, inserting the colorspace element.";
#endif
        gst_bin_add(GST_BIN(m_videoOutputBin), m_colorSpace);
        linked = gst_element_link_many(m_videoIdentity, m_colorSpace, m_videoSink, NULL);
    }

    if (!linked)
        qWarning() << "Linking video output element failed";

#ifdef DEBUG_PLAYBIN
    qDebug() << "notify the video connector it has to emit a new segment message...";
#endif
    //it's necessary to send a new segment event just before
    //the first buffer pushed to the new sink
    g_signal_emit_by_name(m_videoIdentity,
                          "resend-new-segment",
                          true //emit connection-failed signal
                               //to have a chance to insert colorspace element
                          );


    GstState state;

    switch (m_pendingState) {
    case QMediaPlayer::StoppedState:
        state = GST_STATE_NULL;
        break;
    case QMediaPlayer::PausedState:
        state = GST_STATE_PAUSED;
        break;
    case QMediaPlayer::PlayingState:
        state = GST_STATE_PLAYING;
        break;
    }

    if (m_usingColorspaceElement)
        gst_element_set_state(m_colorSpace, state);

    gst_element_set_state(m_videoSink, state);    

    // Set state change that was deferred due the video output
    // change being pending
    gst_element_set_state(m_playbin, state);

    //don't have to wait here, it will unblock eventually
    if (gst_pad_is_blocked(srcPad))
        gst_pad_set_blocked_async(srcPad, false, &block_pad_cb, 0);
    gst_object_unref(GST_OBJECT(srcPad));

#ifdef DEBUG_VO_BIN_DUMP
    _gst_debug_bin_to_dot_file_with_ts(GST_BIN(m_playbin),
                                  GstDebugGraphDetails(GST_DEBUG_GRAPH_SHOW_ALL /* GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES*/),
                                  "playbin_finish");
#endif
}

void QGstreamerPlayerSession::insertColorSpaceElement(GstElement *element, gpointer data)
{
    Q_UNUSED(element);
    QGstreamerPlayerSession* session = reinterpret_cast<QGstreamerPlayerSession*>(data);

    if (session->m_usingColorspaceElement)
        return;
    session->m_usingColorspaceElement = true;

#ifdef DEBUG_PLAYBIN
    qDebug() << "Failed to connect video output, inserting the colorspace elemnt.";
    qDebug() << "notify the video connector it has to emit a new segment message...";
#endif
    //it's necessary to send a new segment event just before
    //the first buffer pushed to the new sink
    g_signal_emit_by_name(session->m_videoIdentity,
                          "resend-new-segment",
                          false // don't emit connection-failed signal
                          );

    gst_element_unlink(session->m_videoIdentity, session->m_videoSink);
    gst_bin_add(GST_BIN(session->m_videoOutputBin), session->m_colorSpace);
    gst_element_link_many(session->m_videoIdentity, session->m_colorSpace, session->m_videoSink, NULL);

    GstState state;

    switch (session->m_pendingState) {
    case QMediaPlayer::StoppedState:
        state = GST_STATE_NULL;
        break;
    case QMediaPlayer::PausedState:
        state = GST_STATE_PAUSED;
        break;
    case QMediaPlayer::PlayingState:
        state = GST_STATE_PLAYING;
        break;
    }

    gst_element_set_state(session->m_colorSpace, state);
}


bool QGstreamerPlayerSession::isVideoAvailable() const
{
    return m_videoAvailable;
}

bool QGstreamerPlayerSession::isSeekable() const
{
    return m_seekable;
}

bool QGstreamerPlayerSession::play()
{
    m_everPlayed = false;
    if (m_playbin) {
        m_pendingState = QMediaPlayer::PlayingState;
        if (gst_element_set_state(m_playbin, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
            qWarning() << "GStreamer; Unable to play -" << m_request.url().toString();
            m_pendingState = m_state = QMediaPlayer::StoppedState;

            emit stateChanged(m_state);
        } else
            return true;
    }

    return false;
}

bool QGstreamerPlayerSession::pause()
{
    if (m_playbin) {
        m_pendingState = QMediaPlayer::PausedState;
        if (m_pendingVideoSink != 0)
            return true;

        if (gst_element_set_state(m_playbin, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
            qWarning() << "GStreamer; Unable to pause -" << m_request.url().toString();
            m_pendingState = m_state = QMediaPlayer::StoppedState;

            emit stateChanged(m_state);
        } else {
            return true;
        }
    }

    return false;
}

void QGstreamerPlayerSession::stop()
{
    m_everPlayed = false;
    if (m_playbin) {
        if (m_renderer)
            m_renderer->stopRenderer();

        gst_element_set_state(m_playbin, GST_STATE_NULL);

        m_lastPosition = 0;
        QMediaPlayer::State oldState = m_state;
        m_pendingState = m_state = QMediaPlayer::StoppedState;

        finishVideoOutputChange();

        //we have to do it here, since gstreamer will not emit bus messages any more
        setSeekable(false);
        if (oldState != m_state)
            emit stateChanged(m_state);
    }
}

bool QGstreamerPlayerSession::seek(qint64 ms)
{
    //seek locks when the video output sink is changing and pad is blocked
    if (m_playbin && !m_pendingVideoSink && m_state != QMediaPlayer::StoppedState) {
        ms = qMax(ms,qint64(0));
        gint64  position = ms * 1000000;
        bool isSeeking = gst_element_seek(m_playbin,
                                          m_playbackRate,
                                          GST_FORMAT_TIME,
                                          GstSeekFlags(GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH),
                                          GST_SEEK_TYPE_SET,
                                          position,
                                          GST_SEEK_TYPE_NONE,
                                          0);
        if (isSeeking)
            m_lastPosition = ms;

        return isSeeking;
    }

    return false;
}

void QGstreamerPlayerSession::setVolume(int volume)
{
    if (m_volume != volume) {
        m_volume = volume;

        if (m_playbin) {
            //playbin2 allows to set volume and muted independently,
            //with playbin1 it's necessary to keep volume at 0 while muted
            if (!m_muted || m_usePlaybin2)
                g_object_set(G_OBJECT(m_playbin), "volume", m_volume/100.0, NULL);
        }

        emit volumeChanged(m_volume);
    }
}

void QGstreamerPlayerSession::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;

        if (m_usePlaybin2)
            g_object_set(G_OBJECT(m_playbin), "mute", m_muted, NULL);
        else
            g_object_set(G_OBJECT(m_playbin), "volume", (m_muted ? 0 : m_volume/100.0), NULL);

        emit mutedStateChanged(m_muted);
    }
}


void QGstreamerPlayerSession::setSeekable(bool seekable)
{
    if (seekable != m_seekable) {
        m_seekable = seekable;
        emit seekableChanged(m_seekable);
    }
}

bool QGstreamerPlayerSession::processSyncMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();

    if (gm && GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ELEMENT) {
        if (m_renderer) {
            if (GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_videoSink))
                m_renderer->handleSyncMessage(gm);

            if (gst_structure_has_name(gm->structure, "prepare-xwindow-id")) {
                m_renderer->precessNewStream();
                return true;
            }
        }
    }

    return false;
}

void QGstreamerPlayerSession::busMessage(const QGstreamerMessage &message)
{
    GstMessage* gm = message.rawMessage();

    if (gm) {
        //tag message comes from elements inside playbin, not from playbin itself
        if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_TAG) {
            //qDebug() << "tag message";
            GstTagList *tag_list;
            gst_message_parse_tag(gm, &tag_list);
            m_tags.unite(QGstUtils::gstTagListToMap(tag_list));

            //qDebug() << m_tags;

            emit tagsChanged();
        } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_DURATION) {
            updateDuration();
        }

#ifdef DEBUG_PLAYBIN
        if (m_sourceType == MMSSrc && qstrcmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(gm)), "source") == 0) {
            qDebug() << "Message from MMSSrc: " << GST_MESSAGE_TYPE(gm);
        }
#endif

        if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_BUFFERING) {
            int progress = 0;
            gst_message_parse_buffering(gm, &progress);
            emit bufferingProgressChanged(progress);
        }

        bool handlePlaybin2 = false;
        if (GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_playbin)) {
            switch (GST_MESSAGE_TYPE(gm))  {
            case GST_MESSAGE_STATE_CHANGED:
                {
                    GstState    oldState;
                    GstState    newState;
                    GstState    pending;

                    gst_message_parse_state_changed(gm, &oldState, &newState, &pending);

#ifdef DEBUG_PLAYBIN
                    QStringList states;
                    states << "GST_STATE_VOID_PENDING" <<  "GST_STATE_NULL" << "GST_STATE_READY" << "GST_STATE_PAUSED" << "GST_STATE_PLAYING";

                    qDebug() << QString("state changed: old: %1  new: %2  pending: %3") \
                            .arg(states[oldState]) \
                            .arg(states[newState]) \
                            .arg(states[pending]);
#endif

                    switch (newState) {
                    case GST_STATE_VOID_PENDING:
                    case GST_STATE_NULL:
                        setSeekable(false);
                        finishVideoOutputChange();
                        if (m_state != QMediaPlayer::StoppedState)
                            emit stateChanged(m_state = QMediaPlayer::StoppedState);
                        break;
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
                            if (m_sourceType == SoupHTTPSrc || m_sourceType == MMSSrc) {
                                //since udpsrc is a live source, it is not applicable here
                                m_everPlayed = true;
                            }

                            getStreamsInfo();
                            updateVideoResolutionTag();

                            //gstreamer doesn't give a reliable indication the duration
                            //information is ready, GST_MESSAGE_DURATION is not sent by most elements
                            //the duration is queried up to 5 times with increasing delay
                            m_durationQueries = 5;
                            updateDuration();

                            /*
                                //gst_element_seek_simple doesn't work reliably here, have to find a better solution

                                GstFormat   format = GST_FORMAT_TIME;
                                gint64      position = 0;
                                bool seekable = false;
                                if (gst_element_query_position(m_playbin, &format, &position)) {
                                    seekable = gst_element_seek_simple(m_playbin, format, GST_SEEK_FLAG_NONE, position);
                                }

                                setSeekable(seekable);
                                */

                            setSeekable(true);

                            if (!qFuzzyCompare(m_playbackRate, qreal(1.0))) {
                                qreal rate = m_playbackRate;
                                m_playbackRate = 1.0;
                                setPlaybackRate(rate);
                            }
                        }

                        if (m_state != prevState)
                            emit stateChanged(m_state);

                        break;
                    }
                    case GST_STATE_PLAYING:
                        m_everPlayed = true;
                        if (m_state != QMediaPlayer::PlayingState)
                            emit stateChanged(m_state = QMediaPlayer::PlayingState);

                        break;
                    }
                }
                break;

            case GST_MESSAGE_EOS:
                emit playbackFinished();
                break;

            case GST_MESSAGE_TAG:
            case GST_MESSAGE_STREAM_STATUS:
            case GST_MESSAGE_UNKNOWN:
                break;
            case GST_MESSAGE_ERROR: {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_error(gm, &err, &debug);
                    if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND)
                        processInvalidMedia(QMediaPlayer::FormatError, tr("Cannot play stream of type: <unknown>"));
                    else
                        processInvalidMedia(QMediaPlayer::ResourceError, QString::fromUtf8(err->message));
                    qWarning() << "Error:" << QString::fromUtf8(err->message);
                    g_error_free(err);
                    g_free(debug);
                }
                break;
            case GST_MESSAGE_WARNING:
                {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_warning (gm, &err, &debug);
                    qWarning() << "Warning:" << QString::fromUtf8(err->message);
                    g_error_free (err);
                    g_free (debug);
                }
                break;
            case GST_MESSAGE_INFO:
#ifdef DEBUG_PLAYBIN
                {
                    GError *err;
                    gchar *debug;
                    gst_message_parse_info (gm, &err, &debug);
                    qDebug() << "Info:" << QString::fromUtf8(err->message);
                    g_error_free (err);
                    g_free (debug);
                }
#endif
                break;
            case GST_MESSAGE_BUFFERING:
            case GST_MESSAGE_STATE_DIRTY:
            case GST_MESSAGE_STEP_DONE:
            case GST_MESSAGE_CLOCK_PROVIDE:
            case GST_MESSAGE_CLOCK_LOST:
            case GST_MESSAGE_NEW_CLOCK:
            case GST_MESSAGE_STRUCTURE_CHANGE:
            case GST_MESSAGE_APPLICATION:
            case GST_MESSAGE_ELEMENT:
                break;
            case GST_MESSAGE_SEGMENT_START:
                {
                    const GstStructure *structure = gst_message_get_structure(gm);
                    qint64 position = g_value_get_int64(gst_structure_get_value(structure, "position"));
                    position /= 1000000;
                    m_lastPosition = position;
                    emit positionChanged(position);
                }
                break;
            case GST_MESSAGE_SEGMENT_DONE:
                break;
            case GST_MESSAGE_LATENCY:
#if (GST_VERSION_MAJOR >= 0) &&  (GST_VERSION_MINOR >= 10) && (GST_VERSION_MICRO >= 13)
            case GST_MESSAGE_ASYNC_START:
                break;
            case GST_MESSAGE_ASYNC_DONE:
            {
                GstFormat   format = GST_FORMAT_TIME;
                gint64      position = 0;
                if (gst_element_query_position(m_playbin, &format, &position)) {
                    position /= 1000000;
                    m_lastPosition = position;
                    emit positionChanged(position);
                }
                break;
            }
#if GST_VERSION_MICRO >= 23
            case GST_MESSAGE_REQUEST_STATE:
#endif
#endif
            case GST_MESSAGE_ANY:
                break;
            default:
                break;
            }
        } else if (m_videoSink
                   && m_renderer
                   && GST_MESSAGE_SRC(gm) == GST_OBJECT_CAST(m_videoSink)) {

            m_renderer->handleBusMessage(gm);
            if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_STATE_CHANGED) {
                GstState oldState;
                GstState newState;
                gst_message_parse_state_changed(gm, &oldState, &newState, 0);

                if (oldState == GST_STATE_READY && newState == GST_STATE_PAUSED)
                    m_renderer->precessNewStream();
            }
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
                handlePlaybin2 = m_usePlaybin2;
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
            handlePlaybin2 = m_usePlaybin2;
        }

        if (handlePlaybin2) {
            if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_WARNING) {
                GError *err;
                gchar *debug;
                gst_message_parse_warning(gm, &err, &debug);
                if (err->domain == GST_STREAM_ERROR && err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND)
                    emit error(int(QMediaPlayer::FormatError), tr("Cannot play stream of type: <unknown>"));
                qWarning() << "Warning:" << QString::fromUtf8(err->message);
                g_error_free(err);
                g_free(debug);
            } else if (GST_MESSAGE_TYPE(gm) == GST_MESSAGE_ERROR) {
                GError *err;
                gchar *debug;
                gst_message_parse_error(gm, &err, &debug);
                if (qstrncmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(gm)), "decodebin2", 10) == 0
                    || qstrncmp(GST_OBJECT_NAME(GST_MESSAGE_SRC(gm)), "uridecodebin", 12) == 0) {
                    processInvalidMedia(QMediaPlayer::ResourceError, QString::fromUtf8(err->message));
                } else if (err->domain == GST_STREAM_ERROR
                           && (err->code == GST_STREAM_ERROR_DECRYPT || err->code == GST_STREAM_ERROR_DECRYPT_NOKEY)) {
                    processInvalidMedia(QMediaPlayer::AccessDeniedError, QString::fromUtf8(err->message));
                }
                qWarning() << "Error:" << QString::fromUtf8(err->message);
                g_error_free(err);
                g_free(debug);
            }
        }
    }
}

void QGstreamerPlayerSession::getStreamsInfo()
{
    //check if video is available:
    bool haveAudio = false;
    bool haveVideo = false;
    m_streamProperties.clear();
    m_streamTypes.clear();

    if (m_usePlaybin2) {
        gint audioStreamsCount = 0;
        gint videoStreamsCount = 0;
        gint textStreamsCount = 0;

        g_object_get(G_OBJECT(m_playbin), "n-audio", &audioStreamsCount, NULL);
        g_object_get(G_OBJECT(m_playbin), "n-video", &videoStreamsCount, NULL);
        g_object_get(G_OBJECT(m_playbin), "n-text", &textStreamsCount, NULL);

        haveAudio = audioStreamsCount > 0;
        haveVideo = videoStreamsCount > 0;

        m_playbin2StreamOffset[QMediaStreamsControl::AudioStream] = 0;
        m_playbin2StreamOffset[QMediaStreamsControl::VideoStream] = audioStreamsCount;
        m_playbin2StreamOffset[QMediaStreamsControl::SubPictureStream] = audioStreamsCount+videoStreamsCount;

        for (int i=0; i<audioStreamsCount; i++)
            m_streamTypes.append(QMediaStreamsControl::AudioStream);

        for (int i=0; i<videoStreamsCount; i++)
            m_streamTypes.append(QMediaStreamsControl::VideoStream);

        for (int i=0; i<textStreamsCount; i++)
            m_streamTypes.append(QMediaStreamsControl::SubPictureStream);

        for (int i=0; i<m_streamTypes.count(); i++) {
            QMediaStreamsControl::StreamType streamType = m_streamTypes[i];
            QMap<QtMultimediaKit::MetaData, QVariant> streamProperties;

            int streamIndex = i - m_playbin2StreamOffset[streamType];

            GstTagList *tags = 0;
            switch (streamType) {
            case QMediaStreamsControl::AudioStream:
                g_signal_emit_by_name(G_OBJECT(m_playbin), "get-audio-tags", streamIndex, &tags);
                break;
            case QMediaStreamsControl::VideoStream:
                g_signal_emit_by_name(G_OBJECT(m_playbin), "get-video-tags", streamIndex, &tags);
                break;
            case QMediaStreamsControl::SubPictureStream:
                g_signal_emit_by_name(G_OBJECT(m_playbin), "get-text-tags", streamIndex, &tags);
                break;
            default:
                break;
            }

            if (tags && gst_is_tag_list(tags)) {
                gchar *languageCode = 0;
                if (gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &languageCode))
                    streamProperties[QtMultimediaKit::Language] = QString::fromUtf8(languageCode);

                //qDebug() << "language for setream" << i << QString::fromUtf8(languageCode);
                g_free (languageCode);
            }

            m_streamProperties.append(streamProperties);
        }
    } else { // PlayBin 1
        enum {
            GST_STREAM_TYPE_UNKNOWN,
            GST_STREAM_TYPE_AUDIO,
            GST_STREAM_TYPE_VIDEO,
            GST_STREAM_TYPE_TEXT,
            GST_STREAM_TYPE_SUBPICTURE,
            GST_STREAM_TYPE_ELEMENT
        };

        GList*      streamInfoList;
        g_object_get(G_OBJECT(m_playbin), "stream-info", &streamInfoList, NULL);

        for (; streamInfoList != 0; streamInfoList = g_list_next(streamInfoList)) {
            gint        type;
            gchar *languageCode = 0;

            GObject*    streamInfo = G_OBJECT(streamInfoList->data);

            g_object_get(streamInfo, "type", &type, NULL);
            g_object_get(streamInfo, "language-code", &languageCode, NULL);

            QMediaStreamsControl::StreamType streamType = QMediaStreamsControl::UnknownStream;

            switch (type) {
            case GST_STREAM_TYPE_VIDEO:
                streamType = QMediaStreamsControl::VideoStream;
                haveVideo = true;
                break;
            case GST_STREAM_TYPE_AUDIO:
                streamType = QMediaStreamsControl::AudioStream;
                haveAudio = true;
                break;
            case GST_STREAM_TYPE_SUBPICTURE:
                streamType = QMediaStreamsControl::SubPictureStream;
                break;
            case GST_STREAM_TYPE_UNKNOWN: {
                GstCaps *caps = 0;
                g_object_get(streamInfo, "caps", &caps, NULL);
                const GstStructure *structure = gst_caps_get_structure(caps, 0);
                const gchar *media_type = gst_structure_get_name(structure);
                emit error(int(QMediaPlayer::FormatError), QString::fromLatin1("Cannot play stream of type: %1").arg(QString::fromUtf8(media_type)));
#ifdef DEBUG_PLAYBIN
                qDebug() << "Encountered unknown stream type";
#endif
                gst_caps_unref(caps);
            }
            default:
                streamType = QMediaStreamsControl::UnknownStream;
                break;
            }

            QMap<QtMultimediaKit::MetaData, QVariant> streamProperties;
            streamProperties[QtMultimediaKit::Language] = QString::fromUtf8(languageCode);

            m_streamProperties.append(streamProperties);
            m_streamTypes.append(streamType);
        }
    }


    if (haveAudio != m_audioAvailable) {
        m_audioAvailable = haveAudio;
        emit audioAvailableChanged(m_audioAvailable);
    }
    if (haveVideo != m_videoAvailable) {
        m_videoAvailable = haveVideo;
        emit videoAvailableChanged(m_videoAvailable);
    }

    emit streamsChanged();
}

void QGstreamerPlayerSession::updateVideoResolutionTag()
{
    QSize size;
    QSize aspectRatio;

    GstPad *pad = gst_element_get_static_pad(m_videoIdentity, "src");
    GstCaps *caps = gst_pad_get_negotiated_caps(pad);

    if (caps) {
        const GstStructure *structure = gst_caps_get_structure(caps, 0);
        gst_structure_get_int(structure, "width", &size.rwidth());
        gst_structure_get_int(structure, "height", &size.rheight());

        gint aspectNum = 0;
        gint aspectDenum = 0;
        if (!size.isEmpty() && gst_structure_get_fraction(
                    structure, "pixel-aspect-ratio", &aspectNum, &aspectDenum)) {
            if (aspectDenum > 0)
                aspectRatio = QSize(aspectNum, aspectDenum);
        }
        gst_caps_unref(caps);
    }

    gst_object_unref(GST_OBJECT(pad));

    QSize currentSize = m_tags.value("resolution").toSize();
    QSize currentAspectRatio = m_tags.value("pixel-aspect-ratio").toSize();

    if (currentSize != size || currentAspectRatio != aspectRatio) {
        if (aspectRatio.isEmpty())
            m_tags.remove("pixel-aspect-ratio");

        if (size.isEmpty()) {
            m_tags.remove("resolution");
        } else {
            m_tags.insert("resolution", QVariant(size));
            if (!aspectRatio.isEmpty())
                m_tags.insert("pixel-aspect-ratio", QVariant(aspectRatio));
        }

        emit tagsChanged();
    }
}

void QGstreamerPlayerSession::updateDuration()
{
    GstFormat format = GST_FORMAT_TIME;
    gint64 gstDuration = 0;
    int duration = -1;

    if (m_playbin && gst_element_query_duration(m_playbin, &format, &gstDuration))
        duration = gstDuration / 1000000;

    if (m_duration != duration) {
        m_duration = duration;
        emit durationChanged(m_duration);
    }

    if (m_duration > 0)
        m_durationQueries = 0;

    if (m_durationQueries > 0) {
        //increase delay between duration requests
        int delay = 25 << (5 - m_durationQueries);
        QTimer::singleShot(delay, this, SLOT(updateDuration()));
        m_durationQueries--;
    }
}

void QGstreamerPlayerSession::playbinNotifySource(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(p);

    GstElement *source = 0;
    g_object_get(o, "source", &source, NULL);
    if (source == 0)
        return;

#ifdef DEBUG_PLAYBIN
    qDebug() << "Playbin source added:" << G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(source));
#endif

    // Turn off icecast metadata request, will be re-set if in QNetworkRequest
    // (souphttpsrc docs say is false by default, but header appears in request
    // @version 0.10.21)
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "iradio-mode") != 0)
        g_object_set(G_OBJECT(source), "iradio-mode", FALSE, NULL);


    // Set Headers
    const QByteArray userAgentString("User-Agent");

    QGstreamerPlayerSession *self = reinterpret_cast<QGstreamerPlayerSession *>(d);

    // User-Agent - special case, souphhtpsrc will always set something, even if
    // defined in extra-headers
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "user-agent") != 0) {
        g_object_set(G_OBJECT(source), "user-agent",
                     self->m_request.rawHeader(userAgentString).constData(), NULL);
    }

    // The rest
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "extra-headers") != 0) {
        GstStructure *extras = gst_structure_empty_new("extras");

        foreach (const QByteArray &rawHeader, self->m_request.rawHeaderList()) {
            if (rawHeader == userAgentString) // Filter User-Agent
                continue;
            else {
                GValue headerValue;

                memset(&headerValue, 0, sizeof(GValue));
                g_value_init(&headerValue, G_TYPE_STRING);

                g_value_set_string(&headerValue,
                                   self->m_request.rawHeader(rawHeader).constData());

                gst_structure_set_value(extras, rawHeader.constData(), &headerValue);
            }
        }

        if (gst_structure_n_fields(extras) > 0)
            g_object_set(G_OBJECT(source), "extra-headers", extras, NULL);

        gst_structure_free(extras);
    }

    //set timeout property to 5 seconds
    if (qstrcmp(G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(source)), "GstUDPSrc") == 0) {
        //udpsrc timeout unit = microsecond
        g_object_set(G_OBJECT(source), "timeout", G_GUINT64_CONSTANT(5000000), NULL);
        self->m_sourceType = UDPSrc;
    } else if (qstrcmp(G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(source)), "GstSoupHTTPSrc") == 0) {
        //souphttpsrc timeout unit = second
        g_object_set(G_OBJECT(source), "timeout", guint(5), NULL);
        self->m_sourceType = SoupHTTPSrc;
    } else if (qstrcmp(G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(source)), "GstMMSSrc") == 0) {
        self->m_sourceType = MMSSrc;
        g_object_set(G_OBJECT(source), "tcp-timeout", G_GUINT64_CONSTANT(5000000), NULL);
    } else {
        self->m_sourceType = UnknownSrc;
    }

    gst_object_unref(source);
}

void QGstreamerPlayerSession::handleVolumeChange(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(o);
    Q_UNUSED(p);
    QGstreamerPlayerSession *session = reinterpret_cast<QGstreamerPlayerSession *>(d);
    QMetaObject::invokeMethod(session, "updateVolume", Qt::QueuedConnection);
}

void QGstreamerPlayerSession::updateVolume()
{
    double volume = 1.0;
    g_object_get(m_playbin, "volume", &volume, NULL);

    //special case for playbin1 volume changes in muted state
    //playbin1 has no separate muted state,
    //it's emulated with volume value saved and set to 0
    //this change should not be reported to user
    if (!m_usePlaybin2 && m_muted) {
        if (volume > 0.001) {
            //volume is changed, player in not muted any more
            m_muted = false;
            emit mutedStateChanged(m_muted = false);
        } else {
            //don't emit volume changed to 0 when player is muted
            return;
        }
    }

    if (m_volume != int(volume*100)) {
        m_volume = int(volume*100);
#ifdef DEBUG_PLAYBIN
        qDebug() << Q_FUNC_INFO << m_muted;
#endif
        emit volumeChanged(m_volume);
    }
}

void QGstreamerPlayerSession::handleMutedChange(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(o);
    Q_UNUSED(p);
    QGstreamerPlayerSession *session = reinterpret_cast<QGstreamerPlayerSession *>(d);
    QMetaObject::invokeMethod(session, "updateMuted", Qt::QueuedConnection);
}

void QGstreamerPlayerSession::updateMuted()
{
    gboolean muted = false;
    g_object_get(G_OBJECT(m_playbin), "mute", &muted, NULL);
    if (m_muted != muted) {
        m_muted = muted;
#ifdef DEBUG_PLAYBIN
        qDebug() << Q_FUNC_INFO << m_muted;
#endif
        emit mutedStateChanged(muted);
    }
}


void QGstreamerPlayerSession::handleElementAdded(GstBin *bin, GstElement *element, QGstreamerPlayerSession *session)
{
    Q_UNUSED(bin);
    //we have to configure queue2 element to enable media downloading
    //and reporting available ranges,
    //but it's added dynamically to playbin2

    gchar *elementName = gst_element_get_name(element);

    if (g_str_has_prefix(elementName, "queue2")) {
        session->m_haveQueueElement = true;

        if (session->property("mediaDownloadEnabled").toBool()) {
            QDir cacheDir(QDesktopServices::storageLocation(QDesktopServices::CacheLocation));
            QString cacheLocation = cacheDir.absoluteFilePath("gstmedia__XXXXXX");
#ifdef DEBUG_PLAYBIN
            qDebug() << "set queue2 temp-location" << cacheLocation;
#endif
            g_object_set(G_OBJECT(element), "temp-template", cacheLocation.toUtf8().constData(), NULL);
        } else {
            g_object_set(G_OBJECT(element), "temp-template", NULL, NULL);
        }
    } else if (g_str_has_prefix(elementName, "uridecodebin") ||
               g_str_has_prefix(elementName, "decodebin2")) {
        //listen for queue2 element added to uridecodebin/decodebin2 as well.
        //Don't touch other bins since they may have unrelated queues
        g_signal_connect(element, "element-added",
                         G_CALLBACK(handleElementAdded), session);
    }

    g_free(elementName);
}

//doing proper operations when detecting an invalidMedia: change media status before signal the erorr
void QGstreamerPlayerSession::processInvalidMedia(QMediaPlayer::Error errorCode, const QString& errorString)
{
    emit invalidMedia();
    stop();
    emit error(int(errorCode), errorString);
}
