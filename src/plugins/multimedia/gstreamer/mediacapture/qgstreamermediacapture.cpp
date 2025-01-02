// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamermediacapture_p.h"
#include "qgstreamermediaencoder_p.h"
#include "qgstreamerimagecapture_p.h"
#include "qgstreamercamera_p.h"
#include <qgstpipeline_p.h>

#include "qgstreameraudioinput_p.h"
#include "qgstreameraudiooutput_p.h"
#include "qgstreamervideooutput_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

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

QMaybe<QPlatformMediaCaptureSession *> QGstreamerMediaCapture::create()
{
    auto videoOutput = QGstreamerVideoOutput::create();
    if (!videoOutput)
        return videoOutput.error();

    return new QGstreamerMediaCapture(videoOutput.value());
}

QGstreamerMediaCapture::QGstreamerMediaCapture(QGstreamerVideoOutput *videoOutput)
    : gstPipeline("pipeline"), gstVideoOutput(videoOutput)
{
    gstVideoOutput->setParent(this);
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
        Q_ASSERT(encoderVideoCapsFilter);
        encoderVideoCapsFilter.set("caps", QGstCaps(caps, QGstCaps::HasRef));

        gstPipeline.add(encoderVideoCapsFilter);

        encoderVideoCapsFilter.src().link(videoSink);
        linkTeeToPad(gstVideoTee, encoderVideoCapsFilter.sink());
        encoderVideoCapsFilter.setState(GST_STATE_PLAYING);
        encoderVideoSink = encoderVideoCapsFilter.sink();
    }

    if (!gstAudioTee.isNull() && !audioSink.isNull()) {
        auto caps = gst_pad_get_current_caps(gstAudioTee.sink().pad());

        encoderAudioCapsFilter = QGstElement("capsfilter", "encoderAudioCapsFilter");
        Q_ASSERT(encoderAudioCapsFilter);
        encoderAudioCapsFilter.set("caps", QGstCaps(caps, QGstCaps::HasRef));

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

#include "moc_qgstreamermediacapture_p.cpp"
