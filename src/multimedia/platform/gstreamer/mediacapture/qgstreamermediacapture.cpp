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

#include "qgstreamermediacapture_p.h"
#include "qgstreamermediaencoder_p.h"
#include "qgstreamerimagecapture_p.h"
#include "qgstreamercamera_p.h"
#include <private/qgstpipeline_p.h>

#include "private/qgstreameraudioinput_p.h"
#include "private/qgstreameraudiooutput_p.h"
#include "private/qgstreamervideooutput_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMediaCapture, "qt.multimedia.capture")


static void linkTeeToPad(QGstElement tee, QGstPad sink)
{
    if (tee.isNull() || sink.isNull())
        return;

    auto source = tee.getRequestPad("src_%u");
    source.link(sink);
}

static void unlinkTeeFromPad(QGstElement tee, QGstPad sink)
{
    if (tee.isNull() || sink.isNull())
        return;

    auto source = sink.peer();
    source.unlink(sink);

    tee.releaseRequestPad(source);
}


QGstreamerMediaCapture::QGstreamerMediaCapture()
    : gstPipeline("pipeline")
{
    gstVideoOutput = new QGstreamerVideoOutput(this);
    gstVideoOutput->setIsPreview();
    gstVideoOutput->setPipeline(gstPipeline);

    // Use system clock to drive all elements in the pipeline. Otherwise,
    // the clock is sourced from the elements (e.g. from an audio source).
    // Since the elements are added and removed dynamically the clock would
    // also change causing lost of synchronization in the pipeline.
    gst_pipeline_use_clock(gstPipeline.pipeline(), gst_system_clock_obtain());

    // This is the recording pipeline with only live sources, thus the pipeline
    // will be always in the playing state.
    gstPipeline.setState(GST_STATE_PLAYING);
    gstPipeline.setInStoppedState(false);

    gstPipeline.dumpGraph("initial");
}

QGstreamerMediaCapture::~QGstreamerMediaCapture()
{
    setMediaRecorder(nullptr);
    setImageCapture(nullptr);
    setCamera(nullptr);
    gstPipeline.setStateSync(GST_STATE_NULL);
}

QPlatformCamera *QGstreamerMediaCapture::camera()
{
    return gstCamera;
}

void QGstreamerMediaCapture::setCamera(QPlatformCamera *camera)
{
    QGstreamerCamera *control = static_cast<QGstreamerCamera *>(camera);
    if (gstCamera == control)
        return;

    if (gstCamera) {
        unlinkTeeFromPad(gstVideoTee, encoderVideoSink);
        unlinkTeeFromPad(gstVideoTee, imageCaptureSink);

        auto camera = gstCamera->gstElement();

        gstPipeline.remove(camera);
        gstPipeline.remove(gstVideoTee);
        gstPipeline.remove(gstVideoOutput->gstElement());

        camera.setStateSync(GST_STATE_NULL);
        gstVideoTee.setStateSync(GST_STATE_NULL);
        gstVideoOutput->gstElement().setStateSync(GST_STATE_NULL);

        gstVideoTee = {};
        gstCamera->setCaptureSession(nullptr);
    }

    gstCamera = control;
    if (gstCamera) {
        QGstElement camera = gstCamera->gstElement();
        gstVideoTee = QGstElement("tee", "videotee");
        gstVideoTee.set("allow-not-linked", true);

        gstPipeline.add(gstVideoOutput->gstElement(), camera, gstVideoTee);

        linkTeeToPad(gstVideoTee, encoderVideoSink);
        linkTeeToPad(gstVideoTee, gstVideoOutput->gstElement().staticPad("sink"));
        linkTeeToPad(gstVideoTee, imageCaptureSink);

        camera.link(gstVideoTee);

        gstVideoOutput->gstElement().setState(GST_STATE_PLAYING);
        gstVideoTee.setState(GST_STATE_PLAYING);
        camera.setState(GST_STATE_PLAYING);
    }

    gstPipeline.dumpGraph("camera");

    emit cameraChanged();
}

QPlatformImageCapture *QGstreamerMediaCapture::imageCapture()
{
    return m_imageCapture;
}

void QGstreamerMediaCapture::setImageCapture(QPlatformImageCapture *imageCapture)
{
    QGstreamerImageCapture *control = static_cast<QGstreamerImageCapture *>(imageCapture);
    if (m_imageCapture == control)
        return;

    if (m_imageCapture) {
        unlinkTeeFromPad(gstVideoTee, imageCaptureSink);
        gstPipeline.remove(m_imageCapture->gstElement());
        m_imageCapture->gstElement().setStateSync(GST_STATE_NULL);
        imageCaptureSink = {};
        m_imageCapture->setCaptureSession(nullptr);
    }

    m_imageCapture = control;
    if (m_imageCapture) {
        imageCaptureSink = m_imageCapture->gstElement().staticPad("sink");
        m_imageCapture->gstElement().setState(GST_STATE_PLAYING);
        gstPipeline.add(m_imageCapture->gstElement());
        linkTeeToPad(gstVideoTee, imageCaptureSink);
        m_imageCapture->setCaptureSession(this);
    }

    gstPipeline.dumpGraph("imageCapture");

    emit imageCaptureChanged();
}

void QGstreamerMediaCapture::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    QGstreamerMediaEncoder *control = static_cast<QGstreamerMediaEncoder *>(recorder);
    if (m_mediaEncoder == control)
        return;

    if (m_mediaEncoder)
        m_mediaEncoder->setCaptureSession(nullptr);
    m_mediaEncoder = control;
    if (m_mediaEncoder)
        m_mediaEncoder->setCaptureSession(this);

    emit encoderChanged();
    gstPipeline.dumpGraph("encoder");
}

QPlatformMediaRecorder *QGstreamerMediaCapture::mediaRecorder()
{
    return m_mediaEncoder;
}

void QGstreamerMediaCapture::linkEncoder(QGstPad audioSink, QGstPad videoSink)
{
    if (!gstVideoTee.isNull() && !videoSink.isNull()) {
        auto caps = gst_pad_get_current_caps(gstVideoTee.sink().pad());

        encoderVideoCapsFilter = QGstElement("capsfilter", "encoderVideoCapsFilter");
        encoderVideoCapsFilter.set("caps", QGstMutableCaps(caps));

        gstPipeline.add(encoderVideoCapsFilter);

        encoderVideoCapsFilter.src().link(videoSink);
        linkTeeToPad(gstVideoTee, encoderVideoCapsFilter.sink());
        encoderVideoCapsFilter.setState(GST_STATE_PLAYING);
        encoderVideoSink = encoderVideoCapsFilter.sink();
    }

    if (!gstAudioTee.isNull() && !audioSink.isNull()) {
        auto caps = gst_pad_get_current_caps(gstAudioTee.sink().pad());

        encoderAudioCapsFilter = QGstElement("capsfilter", "encoderAudioCapsFilter");
        encoderAudioCapsFilter.set("caps", QGstMutableCaps(caps));

        gstPipeline.add(encoderAudioCapsFilter);

        encoderAudioCapsFilter.src().link(audioSink);
        linkTeeToPad(gstAudioTee, encoderAudioCapsFilter.sink());
        encoderAudioCapsFilter.setState(GST_STATE_PLAYING);
        encoderAudioSink = encoderAudioCapsFilter.sink();
    }
}

void QGstreamerMediaCapture::unlinkEncoder()
{
    if (!encoderVideoCapsFilter.isNull()) {
        encoderVideoCapsFilter.src().unlinkPeer();
        unlinkTeeFromPad(gstVideoTee, encoderVideoCapsFilter.sink());
        gstPipeline.remove(encoderVideoCapsFilter);
        encoderVideoCapsFilter.setStateSync(GST_STATE_NULL);
        encoderVideoCapsFilter = {};
    }

    if (!encoderAudioCapsFilter.isNull()) {
        encoderAudioCapsFilter.src().unlinkPeer();
        unlinkTeeFromPad(gstAudioTee, encoderAudioCapsFilter.sink());
        gstPipeline.remove(encoderAudioCapsFilter);
        encoderAudioCapsFilter.setStateSync(GST_STATE_NULL);
        encoderAudioCapsFilter = {};
    }

    encoderAudioSink = {};
    encoderVideoSink = {};
}

void QGstreamerMediaCapture::setAudioInput(QPlatformAudioInput *input)
{
    if (gstAudioInput == input)
        return;

    if (gstAudioInput) {
        unlinkTeeFromPad(gstAudioTee, encoderAudioSink);

        if (gstAudioOutput) {
            unlinkTeeFromPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
            gstPipeline.remove(gstAudioOutput->gstElement());
            gstAudioOutput->gstElement().setStateSync(GST_STATE_NULL);
        }

        gstPipeline.remove(gstAudioInput->gstElement());
        gstPipeline.remove(gstAudioTee);
        gstAudioInput->gstElement().setStateSync(GST_STATE_NULL);
        gstAudioTee.setStateSync(GST_STATE_NULL);
        gstAudioTee = {};
    }

    gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
    if (gstAudioInput) {
        Q_ASSERT(gstAudioTee.isNull());
        gstAudioTee = QGstElement("tee", "audiotee");
        gstAudioTee.set("allow-not-linked", true);
        gstPipeline.add(gstAudioInput->gstElement(), gstAudioTee);
        gstAudioInput->gstElement().link(gstAudioTee);

        if (gstAudioOutput) {
            gstPipeline.add(gstAudioOutput->gstElement());
            gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
            linkTeeToPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
        }

        gstAudioTee.setState(GST_STATE_PLAYING);
        gstAudioInput->gstElement().setStateSync(GST_STATE_PLAYING);

        linkTeeToPad(gstAudioTee, encoderAudioSink);
    }
}

void QGstreamerMediaCapture::setVideoPreview(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

void QGstreamerMediaCapture::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    if (gstAudioOutput && gstAudioInput) {
        // If audio input is set, the output is in the pipeline
        unlinkTeeFromPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
        gstPipeline.remove(gstAudioOutput->gstElement());
        gstAudioOutput->gstElement().setStateSync(GST_STATE_NULL);
    }

    gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstAudioOutput && gstAudioInput) {
        gstPipeline.add(gstAudioOutput->gstElement());
        gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
        linkTeeToPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
    }
}

QGstreamerVideoSink *QGstreamerMediaCapture::gstreamerVideoSink() const
{
    return gstVideoOutput ? gstVideoOutput->gstreamerVideoSink() : nullptr;
}


QT_END_NAMESPACE
