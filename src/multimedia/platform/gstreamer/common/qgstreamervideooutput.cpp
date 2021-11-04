/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <private/qgstreamervideooutput_p.h>
#include <private/qgstreamervideosink_p.h>
#include <private/qgstsubtitlesink_p.h>
#include <qvideosink.h>

#include <QtCore/qloggingcategory.h>
#include <qthread.h>

Q_LOGGING_CATEGORY(qLcMediaVideoOutput, "qt.multimedia.videooutput")

QT_BEGIN_NAMESPACE

QGstreamerVideoOutput::QGstreamerVideoOutput(QObject *parent)
    : QObject(parent),
      gstVideoOutput("videoOutput")
{
    videoQueue = QGstElement("queue", "videoQueue");
    videoConvert = QGstElement("videoconvert", "videoConvert");
    videoSink = QGstElement("fakesink", "fakeVideoSink");
    videoSink.set("sync", true);
    gstVideoOutput.add(videoQueue, videoConvert, videoSink);
    if (!videoQueue.link(videoConvert, videoSink))
        qCDebug(qLcMediaVideoOutput) << ">>>>>> linking failed";

    gstVideoOutput.addGhostPad(videoQueue, "sink");
}

QGstreamerVideoOutput::~QGstreamerVideoOutput()
{
    gstVideoOutput.setStateSync(GST_STATE_NULL);
}

void QGstreamerVideoOutput::setVideoSink(QVideoSink *sink)
{
    auto *gstVideoSink = sink ? static_cast<QGstreamerVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (gstVideoSink == m_videoSink)
        return;

    if (m_videoSink)
        m_videoSink->setPipeline({});

    m_videoSink = gstVideoSink;
    if (m_videoSink)
        m_videoSink->setPipeline(gstPipeline);

    QGstElement gstSink;
    if (m_videoSink) {
        gstSink = m_videoSink->gstSink();
        isFakeSink = false;
    } else {
        gstSink = QGstElement("fakesink", "fakevideosink");
        gstSink.set("sync", true);
        isFakeSink = true;
    }

    if (videoSink == gstSink)
        return;

    gstPipeline.beginConfig();
    if (!videoSink.isNull()) {
        gstVideoOutput.remove(videoSink);
        videoSink.setStateSync(GST_STATE_NULL);
    }
    videoSink = gstSink;
    gstVideoOutput.add(videoSink);

    videoConvert.link(videoSink);
    GstEvent *event = gst_event_new_reconfigure();
    gst_element_send_event(videoSink.element(), event);
    videoSink.syncStateWithParent();

    doLinkSubtitleStream();

    gstPipeline.endConfig();

    qCDebug(qLcMediaVideoOutput) << "sinkChanged" << gstSink.name();

    GST_DEBUG_BIN_TO_DOT_FILE(gstPipeline.bin(),
                              GstDebugGraphDetails(/*GST_DEBUG_GRAPH_SHOW_ALL |*/ GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE |
                                                   GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES),
                              videoSink.name());

}

void QGstreamerVideoOutput::setPipeline(const QGstPipeline &pipeline)
{
    gstPipeline = pipeline;
    if (m_videoSink)
        m_videoSink->setPipeline(gstPipeline);
}

void QGstreamerVideoOutput::linkSubtitleStream(QGstElement src)
{
    qCDebug(qLcMediaVideoOutput) << "link subtitle stream" << src.isNull();
    if (src == subtitleSrc)
        return;

    gstPipeline.beginConfig();
    subtitleSrc = src;
    doLinkSubtitleStream();
    gstPipeline.endConfig();
}

void QGstreamerVideoOutput::unlinkSubtitleStream()
{
    if (subtitleSrc.isNull())
        return;
    qCDebug(qLcMediaVideoOutput) << "unlink subtitle stream";
    subtitleSrc = {};
    if (!subtitleSink.isNull()) {
        gstPipeline.beginConfig();
        gstPipeline.remove(subtitleSink);
        gstPipeline.endConfig();
        subtitleSink.setStateSync(GST_STATE_NULL);
        subtitleSink = {};
    }
    if (m_videoSink)
        m_videoSink->setSubtitleText({});
}

void QGstreamerVideoOutput::doLinkSubtitleStream()
{
    if (!subtitleSink.isNull()) {
        gstPipeline.remove(subtitleSink);
        subtitleSink.setStateSync(GST_STATE_NULL);
        subtitleSink = {};
    }
    if (!m_videoSink || subtitleSrc.isNull())
        return;
    if (subtitleSink.isNull()) {
        subtitleSink = m_videoSink->subtitleSink();
        gstPipeline.add(subtitleSink);
    }
    if (!subtitleSrc.link(subtitleSink))
        qCDebug(qLcMediaVideoOutput) << "link subtitle stream failed";
}

void QGstreamerVideoOutput::setIsPreview()
{
    // configures the queue to be fast and lightweight for camera preview
    // also avoids blocking the queue in case we have an encodebin attached to the tee as well
    videoQueue.set("leaky", 2 /*downstream*/);
    videoQueue.set("silent", true);
    videoQueue.set("max-size-buffers", uint(1));
    videoQueue.set("max-size-bytes", uint(0));
    videoQueue.set("max-size-time", quint64(0));
}

void QGstreamerVideoOutput::flushSubtitles()
{
    if (!subtitleSink.isNull()) {
        auto pad = subtitleSink.staticPad("sink");
        auto *event = gst_event_new_flush_start();
        pad.sendEvent(event);
        event = gst_event_new_flush_stop(false);
        pad.sendEvent(event);
    }
}

QT_END_NAMESPACE
