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
    videoScale = QGstElement("videoscale", "videoScale");
    videoSink = QGstElement("fakesink", "fakeVideoSink");
    gstVideoOutput.add(videoQueue, videoConvert, videoScale, videoSink);
    videoQueue.link(videoConvert, videoScale, videoSink);

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
        connect(m_videoOutput, SIGNAL(sinkChanged()),
                this, SLOT(updateVideoRenderer()));
    }

    m_videoOutput->setSurface(surface);

    newVideoSink = getSink(m_videoOutput);
    if (newVideoSink == videoSink) {
        newVideoSink = {};
        return;
    }
    gstVideoOutput.add(newVideoSink);

    qCDebug(qLcMediaVideoOutput) << "setVideoSurface: Reconfiguring video output" << QThread::currentThreadId();

    auto state = gstPipeline.state();

    if (state != GST_STATE_PLAYING) {
        changeVideoOutput();
        return;
    }

    // This doesn't quite work, as we're be getting the callback in another thread where state changes aren't allowed.
    auto pad = videoScale.staticPad("src");
    pad.addProbe<&QGstreamerVideoOutput::prepareVideoOutputChange>(this, GST_PAD_PROBE_TYPE_IDLE);
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

void QGstreamerVideoOutput::changeVideoOutput()
{
    qCDebug(qLcMediaVideoOutput) << "Changing video output" << QThread::currentThreadId();

    auto state = videoSink.state();
    videoSink.setState(GST_STATE_NULL);
    gstVideoOutput.remove(videoSink);
    videoSink = newVideoSink;
    videoScale.link(videoSink);
    videoSink.setState(state);
    newVideoSink = {};
}

void QGstreamerVideoOutput::prepareVideoOutputChange(const QGstPad &/*pad*/)
{
    qCDebug(qLcMediaVideoOutput) << "Reconfiguring video output" << QThread::currentThreadId();

    gstPipeline.setState(GST_STATE_PAUSED);
    changeVideoOutput();
    gstPipeline.setState(GST_STATE_PLAYING);
}


QT_END_NAMESPACE
