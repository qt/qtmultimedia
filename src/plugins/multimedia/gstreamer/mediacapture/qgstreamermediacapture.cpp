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

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/quniquehandle_p.h>

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
    : gstPipeline(QGstPipeline::create("mediaCapturePipeline")), gstVideoOutput(videoOutput)
{
    gstVideoOutput->setParent(this);
    gstVideoOutput->setIsPreview();
    gstVideoOutput->setPipeline(gstPipeline);

    // Use system clock to drive all elements in the pipeline. Otherwise,
    // the clock is sourced from the elements (e.g. from an audio source).
    // Since the elements are added and removed dynamically the clock would
    // also change causing lost of synchronization in the pipeline.

    QGstClockHandle systemClock{
        gst_system_clock_obtain(),
    };
    gst_pipeline_use_clock(gstPipeline.pipeline(), systemClock.get());

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

void QGstreamerMediaCapture::setCamera(QPlatformCamera *platformCamera)
{
    QGstreamerCamera *camera = static_cast<QGstreamerCamera *>(platformCamera);
    if (gstCamera == camera)
        return;

    if (gstCamera) {
        QObject::disconnect(gstCameraActiveConnection);
        if (gstVideoTee)
            setCameraActive(false);
    }

    gstCamera = camera;

    if (gstCamera) {
        gstCameraActiveConnection = QObject::connect(camera, &QGstreamerCamera::activeChanged, this,
                                                     &QGstreamerMediaCapture::setCameraActive);
        if (gstCamera->isActive())
            setCameraActive(true);
    }

    emit cameraChanged();
}

void QGstreamerMediaCapture::setCameraActive(bool activate)
{
    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (activate) {
            QGstElement cameraElement = gstCamera->gstElement();
            gstVideoTee = QGstElement::createFromFactory("tee", "videotee");
            gstVideoTee.set("allow-not-linked", true);

            gstPipeline.add(gstVideoOutput->gstElement(), cameraElement, gstVideoTee);

            linkTeeToPad(gstVideoTee, encoderVideoSink);
            linkTeeToPad(gstVideoTee, gstVideoOutput->gstElement().staticPad("sink"));
            linkTeeToPad(gstVideoTee, imageCaptureSink);

            qLinkGstElements(cameraElement, gstVideoTee);

            gstPipeline.syncChildrenState();
        } else {
            unlinkTeeFromPad(gstVideoTee, encoderVideoSink);
            unlinkTeeFromPad(gstVideoTee, imageCaptureSink);

            auto camera = gstCamera->gstElement();

            gstPipeline.stopAndRemoveElements(camera, gstVideoTee, gstVideoOutput->gstElement());

            gstVideoTee = {};
            gstCamera->setCaptureSession(nullptr);
        }
    });

    gstPipeline.dumpGraph("camera");
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

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (m_imageCapture) {
            unlinkTeeFromPad(gstVideoTee, imageCaptureSink);
            gstPipeline.stopAndRemoveElements(m_imageCapture->gstElement());
            imageCaptureSink = {};
            m_imageCapture->setCaptureSession(nullptr);
        }

        m_imageCapture = control;
        if (m_imageCapture) {
            imageCaptureSink = m_imageCapture->gstElement().staticPad("sink");
            gstPipeline.add(m_imageCapture->gstElement());
            m_imageCapture->gstElement().syncStateWithParent();
            linkTeeToPad(gstVideoTee, imageCaptureSink);
            m_imageCapture->setCaptureSession(this);
        }
    });

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
    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (!gstVideoTee.isNull() && !videoSink.isNull()) {
            auto caps = gst_pad_get_current_caps(gstVideoTee.sink().pad());

            encoderVideoCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderVideoCapsFilter");
            Q_ASSERT(encoderVideoCapsFilter);
            encoderVideoCapsFilter.set("caps", QGstCaps(caps, QGstCaps::HasRef));

            gstPipeline.add(encoderVideoCapsFilter);

            encoderVideoCapsFilter.src().link(videoSink);
            linkTeeToPad(gstVideoTee, encoderVideoCapsFilter.sink());
            encoderVideoCapsFilter.syncStateWithParent();
            encoderVideoSink = encoderVideoCapsFilter.sink();
        }

        if (!gstAudioTee.isNull() && !audioSink.isNull()) {
            auto caps = gst_pad_get_current_caps(gstAudioTee.sink().pad());

            encoderAudioCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderAudioCapsFilter");
            Q_ASSERT(encoderAudioCapsFilter);
            encoderAudioCapsFilter.set("caps", QGstCaps(caps, QGstCaps::HasRef));

            gstPipeline.add(encoderAudioCapsFilter);

            encoderAudioCapsFilter.src().link(audioSink);
            linkTeeToPad(gstAudioTee, encoderAudioCapsFilter.sink());
            encoderVideoCapsFilter.syncStateWithParent();
            encoderAudioSink = encoderAudioCapsFilter.sink();
        }
    });
}

void QGstreamerMediaCapture::unlinkEncoder()
{
    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (!encoderVideoCapsFilter.isNull()) {
            encoderVideoCapsFilter.src().unlinkPeer();
            unlinkTeeFromPad(gstVideoTee, encoderVideoCapsFilter.sink());
            gstPipeline.stopAndRemoveElements(encoderVideoCapsFilter);
            encoderVideoCapsFilter = {};
        }

        if (!encoderAudioCapsFilter.isNull()) {
            encoderAudioCapsFilter.src().unlinkPeer();
            unlinkTeeFromPad(gstAudioTee, encoderAudioCapsFilter.sink());
            gstPipeline.stopAndRemoveElements(encoderAudioCapsFilter);
            encoderAudioCapsFilter = {};
        }

        encoderAudioSink = {};
        encoderVideoSink = {};
    });
}

void QGstreamerMediaCapture::setAudioInput(QPlatformAudioInput *input)
{
    if (gstAudioInput == input)
        return;

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (gstAudioInput) {
            unlinkTeeFromPad(gstAudioTee, encoderAudioSink);

            if (gstAudioOutput) {
                unlinkTeeFromPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
                gstPipeline.remove(gstAudioOutput->gstElement());
                gstAudioOutput->gstElement().setStateSync(GST_STATE_NULL);
            }

            gstPipeline.stopAndRemoveElements(gstAudioInput->gstElement(), gstAudioTee);
            gstAudioTee = {};
        }

        gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
        if (gstAudioInput) {
            Q_ASSERT(gstAudioTee.isNull());
            gstAudioTee = QGstElement::createFromFactory("tee", "audiotee");
            gstAudioTee.set("allow-not-linked", true);
            gstPipeline.add(gstAudioInput->gstElement(), gstAudioTee);
            qLinkGstElements(gstAudioInput->gstElement(), gstAudioTee);

            if (gstAudioOutput) {
                gstPipeline.add(gstAudioOutput->gstElement());
                gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
                linkTeeToPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
            }

            gstPipeline.syncChildrenState();

            linkTeeToPad(gstAudioTee, encoderAudioSink);
        }
    });
}

void QGstreamerMediaCapture::setVideoPreview(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

void QGstreamerMediaCapture::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    gstPipeline.modifyPipelineWhileNotRunning([&] {
        if (gstAudioOutput && gstAudioInput) {
            // If audio input is set, the output is in the pipeline
            unlinkTeeFromPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
            gstPipeline.stopAndRemoveElements(gstAudioOutput->gstElement());
        }

        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
        if (gstAudioOutput && gstAudioInput) {
            gstPipeline.add(gstAudioOutput->gstElement());
            gstPipeline.syncChildrenState();
            linkTeeToPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
        }
    });
}

QGstreamerVideoSink *QGstreamerMediaCapture::gstreamerVideoSink() const
{
    return gstVideoOutput ? gstVideoOutput->gstreamerVideoSink() : nullptr;
}


QT_END_NAMESPACE

#include "moc_qgstreamermediacapture_p.cpp"
