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
#include <private/qgstreamervideorenderer_p.h>
#include <private/qgstreamervideosink_p.h>
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
    gstVideoOutput.add(videoQueue, videoConvert, videoSink);
    videoQueue.link(videoConvert, videoSink);

    gstVideoOutput.addGhostPad(videoQueue, "sink");
}

QGstreamerVideoOutput::~QGstreamerVideoOutput()
{
}

static QGstElement getSink(QGstreamerVideoRenderer *output)
{
    QGstElement newSink;
    if (output && output->isReady())
        newSink = output->videoSink();

    if (newSink.isNull())
        newSink = QGstElement("fakesink", "fakevideosink");

    return newSink;
}

void QGstreamerVideoOutput::setVideoSurface(QAbstractVideoSurface *surface)
{
    if (!m_videoOutput) {
        m_videoOutput = new QGstreamerVideoRenderer;
        qCDebug(qLcMediaVideoOutput) << Q_FUNC_INFO;
        connect(m_videoOutput, SIGNAL(sinkChanged()), this, SLOT(sinkChanged()));
    }

    m_videoOutput->setSurface(surface);

    QGstElement gstSink = getSink(m_videoOutput);
    updateVideoSink(gstSink);
}

void QGstreamerVideoOutput::setVideoSink(QVideoSink *sink)
{
    auto *videoSink = static_cast<QGstreamerVideoSink *>(sink->platformVideoSink());
    if (videoSink == m_videoWindow)
        return;

    if (m_videoWindow) {
        gstPipeline.removeMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(m_videoWindow));
        gstPipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(m_videoWindow));
        disconnect(m_videoWindow, SIGNAL(sinkChanged()), this, SLOT(sinkChanged()));
    }

    m_videoWindow = videoSink;
    if (m_videoWindow) {
        gstPipeline.installMessageFilter(static_cast<QGstreamerSyncMessageFilter *>(m_videoWindow));
        gstPipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(m_videoWindow));
        connect(m_videoWindow, SIGNAL(sinkChanged()), this, SLOT(sinkChanged()));
    }
    sinkChanged();
}

void QGstreamerVideoOutput::setIsPreview()
{
    // configures the queue to be fast and lightweight for camera preview
    // also avoids blocking the queue in case we have an encodebin attached to the tee as well
    videoQueue.set("leaky", 2 /*downstream*/);
    videoQueue.set("silent", true);
    videoQueue.set("max-size-buffers", 1);
    videoQueue.set("max-size-bytes", 0);
    videoQueue.set("max-size-time", 0);
}

void QGstreamerVideoOutput::updateVideoSink(const QGstElement &sink)
{
    if (videoSink == sink)
        return;

    newVideoSink = sink;
    gstVideoOutput.add(newVideoSink);

    qCDebug(qLcMediaVideoOutput) << "setVideoSurface: Reconfiguring video output" << QThread::currentThreadId();

    auto state = gstPipeline.state();

    if (state != GST_STATE_PLAYING) {
        changeVideoOutput();
        return;
    }

    // This doesn't quite work, as we're be getting the callback in another thread where state changes aren't allowed.
    auto pad = videoConvert.staticPad("src");
    pad.addProbe<&QGstreamerVideoOutput::prepareVideoOutputChange>(this, GstPadProbeType(GST_PAD_PROBE_TYPE_BUFFER | GST_PAD_PROBE_TYPE_BLOCKING));
}

void QGstreamerVideoOutput::sinkChanged()
{
    QGstElement gstSink;
    if (m_videoWindow) {
        gstSink = m_videoWindow->gstSink();
    } else {
        gstSink = QGstElement("fakesink", "fakevideosink");
    }
    qDebug() << "sinkChanged" << gstSink.name();
    updateVideoSink(gstSink);
}

void QGstreamerVideoOutput::changeVideoOutput()
{
    qCDebug(qLcMediaVideoOutput) << "Changing video output" << QThread::currentThreadId();

    auto pstate = gstPipeline.state();
    if (pstate == GST_STATE_PLAYING)
        gstPipeline.setState(GST_STATE_PAUSED);

    auto state = videoSink.state();
    videoSink.setState(GST_STATE_NULL);
    gstVideoOutput.remove(videoSink);
    videoSink = newVideoSink;
    videoConvert.link(videoSink);
    GstEvent *event = gst_event_new_reconfigure();
    gst_element_send_event(videoSink.element(), event);
    videoSink.setState(state);
    newVideoSink = {};

    gstPipeline.setState(pstate);

    GST_DEBUG_BIN_TO_DOT_FILE(gstPipeline.bin(),
                              GstDebugGraphDetails(/*GST_DEBUG_GRAPH_SHOW_ALL |*/ GST_DEBUG_GRAPH_SHOW_MEDIA_TYPE | GST_DEBUG_GRAPH_SHOW_NON_DEFAULT_PARAMS | GST_DEBUG_GRAPH_SHOW_STATES),
                              videoSink.name());

}

void QGstreamerVideoOutput::prepareVideoOutputChange(const QGstPad &/*pad*/)
{
    qCDebug(qLcMediaVideoOutput) << "Reconfiguring video output" << QThread::currentThreadId();

    if (QThread::currentThread() == this->thread())
        changeVideoOutput();
    else
        QMetaObject::invokeMethod(this, "changeVideoOutput", Qt::BlockingQueuedConnection);
}


QT_END_NAMESPACE
