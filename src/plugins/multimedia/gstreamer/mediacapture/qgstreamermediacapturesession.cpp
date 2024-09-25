// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <mediacapture/qgstreamermediacapturesession_p.h>
#include <mediacapture/qgstreamermediarecorder_p.h>
#include <mediacapture/qgstreamerimagecapture_p.h>
#include <mediacapture/qgstreamercamera_p.h>
#include <common/qgstpipeline_p.h>
#include <common/qgstreameraudioinput_p.h>
#include <common/qgstreameraudiooutput_p.h>
#include <common/qgstreamervideooutput_p.h>
#include <common/qgst_debug_p.h>

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

QMaybe<QPlatformMediaCaptureSession *> QGstreamerMediaCaptureSession::create()
{
    auto videoOutput = QGstreamerVideoOutput::create();
    if (!videoOutput)
        return videoOutput.error();

    static const auto error = qGstErrorMessageIfElementsNotAvailable("tee", "capsfilter");
    if (error)
        return *error;

    return new QGstreamerMediaCaptureSession(videoOutput.value());
}

QGstreamerMediaCaptureSession::QGstreamerMediaCaptureSession(QGstreamerVideoOutput *videoOutput)
    : capturePipeline(QGstPipeline::create("mediaCapturePipeline")), gstVideoOutput(videoOutput)
{
    gstVideoOutput->setParent(this);
    gstVideoOutput->setIsPreview();

    capturePipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));

    // Use system clock to drive all elements in the pipeline. Otherwise,
    // the clock is sourced from the elements (e.g. from an audio source).
    // Since the elements are added and removed dynamically the clock would
    // also change causing lost of synchronization in the pipeline.

    QGstClockHandle systemClock{
        gst_system_clock_obtain(),
    };
    gst_pipeline_use_clock(capturePipeline.pipeline(), systemClock.get());

    // This is the recording pipeline with only live sources, thus the pipeline
    // will be always in the playing state.
    capturePipeline.setState(GST_STATE_PLAYING);
    gstVideoOutput->setActive(true);

    capturePipeline.dumpGraph("initial");
}

QGstreamerMediaCaptureSession::~QGstreamerMediaCaptureSession()
{
    setMediaRecorder(nullptr);
    setImageCapture(nullptr);
    setCamera(nullptr);
    capturePipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    capturePipeline.setStateSync(GST_STATE_NULL);
}

QPlatformCamera *QGstreamerMediaCaptureSession::camera()
{
    return gstCamera;
}

void QGstreamerMediaCaptureSession::setCamera(QPlatformCamera *platformCamera)
{
    auto *camera = static_cast<QGstreamerCameraBase *>(platformCamera);
    if (gstCamera == camera)
        return;

    if (gstCamera) {
        QObject::disconnect(gstCameraActiveConnection);
        if (gstVideoTee)
            setCameraActive(false);
    }

    gstCamera = camera;

    if (gstCamera) {
        gstCameraActiveConnection =
                QObject::connect(camera, &QPlatformCamera::activeChanged, this,
                                 &QGstreamerMediaCaptureSession::setCameraActive);
        if (gstCamera->isActive())
            setCameraActive(true);
    }

    emit cameraChanged();
}

void QGstreamerMediaCaptureSession::setCameraActive(bool activate)
{
    capturePipeline.modifyPipelineWhileNotRunning([&] {
        if (activate) {
            QGstElement cameraElement = gstCamera->gstElement();
            gstVideoTee = QGstElement::createFromFactory("tee", "videotee");
            gstVideoTee.set("allow-not-linked", true);

            capturePipeline.add(gstVideoOutput->gstElement(), cameraElement, gstVideoTee);

            linkTeeToPad(gstVideoTee, encoderVideoSink);
            linkTeeToPad(gstVideoTee, gstVideoOutput->gstElement().staticPad("sink"));
            linkTeeToPad(gstVideoTee, imageCaptureSink);

            qLinkGstElements(cameraElement, gstVideoTee);

            capturePipeline.syncChildrenState();
        } else {
            if (encoderVideoCapsFilter)
                qUnlinkGstElements(gstVideoTee, encoderVideoCapsFilter);
            if (m_imageCapture)
                qUnlinkGstElements(gstVideoTee, m_imageCapture->gstElement());

            auto camera = gstCamera->gstElement();

            capturePipeline.stopAndRemoveElements(camera, gstVideoTee,
                                                  gstVideoOutput->gstElement());

            gstVideoTee = {};
            gstCamera->setCaptureSession(nullptr);
        }
    });

    capturePipeline.dumpGraph("camera");
}

QPlatformImageCapture *QGstreamerMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QGstreamerMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    QGstreamerImageCapture *control = static_cast<QGstreamerImageCapture *>(imageCapture);
    if (m_imageCapture == control)
        return;

    capturePipeline.modifyPipelineWhileNotRunning([&] {
        if (m_imageCapture) {
            if (gstVideoTee)
                qUnlinkGstElements(gstVideoTee, m_imageCapture->gstElement());
            capturePipeline.stopAndRemoveElements(m_imageCapture->gstElement());
            imageCaptureSink = {};
            m_imageCapture->setCaptureSession(nullptr);
        }

        m_imageCapture = control;
        if (m_imageCapture) {
            imageCaptureSink = m_imageCapture->gstElement().staticPad("sink");
            capturePipeline.add(m_imageCapture->gstElement());
            m_imageCapture->gstElement().syncStateWithParent();
            linkTeeToPad(gstVideoTee, imageCaptureSink);
            m_imageCapture->setCaptureSession(this);
        }
    });

    capturePipeline.dumpGraph("imageCapture");

    emit imageCaptureChanged();
}

void QGstreamerMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    QGstreamerMediaRecorder *control = static_cast<QGstreamerMediaRecorder *>(recorder);
    if (m_mediaRecorder == control)
        return;

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(nullptr);
    m_mediaRecorder = control;
    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(this);

    emit encoderChanged();
    capturePipeline.dumpGraph("encoder");
}

QPlatformMediaRecorder *QGstreamerMediaCaptureSession::mediaRecorder()
{
    return m_mediaRecorder;
}

void QGstreamerMediaCaptureSession::linkEncoder(QGstPad audioSink, QGstPad videoSink)
{
    capturePipeline.modifyPipelineWhileNotRunning([&] {
        if (!gstVideoTee.isNull() && !videoSink.isNull()) {
            QGstCaps caps = gstVideoTee.sink().currentCaps();

            encoderVideoCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderVideoCapsFilter");
            Q_ASSERT(encoderVideoCapsFilter);
            encoderVideoCapsFilter.set("caps", caps);

            capturePipeline.add(encoderVideoCapsFilter);

            encoderVideoCapsFilter.src().link(videoSink);
            linkTeeToPad(gstVideoTee, encoderVideoCapsFilter.sink());
            encoderVideoSink = encoderVideoCapsFilter.sink();
        }

        if (!gstAudioTee.isNull() && !audioSink.isNull()) {
            QGstCaps caps = gstAudioTee.sink().currentCaps();

            encoderAudioCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderAudioCapsFilter");
            Q_ASSERT(encoderAudioCapsFilter);
            encoderAudioCapsFilter.set("caps", caps);

            capturePipeline.add(encoderAudioCapsFilter);

            encoderAudioCapsFilter.src().link(audioSink);
            linkTeeToPad(gstAudioTee, encoderAudioCapsFilter.sink());
            encoderAudioSink = encoderAudioCapsFilter.sink();
        }
    });
}

void QGstreamerMediaCaptureSession::unlinkEncoder()
{
    capturePipeline.modifyPipelineWhileNotRunning([&] {
        if (encoderVideoCapsFilter) {
            if (gstVideoTee)
                qUnlinkGstElements(gstVideoTee, encoderVideoCapsFilter);
            capturePipeline.stopAndRemoveElements(encoderVideoCapsFilter);
            encoderVideoCapsFilter = {};
        }

        if (encoderAudioCapsFilter) {
            if (gstAudioTee)
                qUnlinkGstElements(gstAudioTee, encoderAudioCapsFilter);
            capturePipeline.stopAndRemoveElements(encoderAudioCapsFilter);
            encoderAudioCapsFilter = {};
        }

        encoderAudioSink = {};
        encoderVideoSink = {};
    });
}

const QGstPipeline &QGstreamerMediaCaptureSession::pipeline() const
{
    return capturePipeline;
}

void QGstreamerMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (gstAudioInput == input)
        return;

    capturePipeline.modifyPipelineWhileNotRunning([&] {
        if (gstAudioInput) {
            if (encoderAudioCapsFilter)
                qUnlinkGstElements(gstAudioTee, encoderAudioCapsFilter);

            if (gstAudioOutput) {
                qUnlinkGstElements(gstAudioTee, gstAudioOutput->gstElement());
                capturePipeline.stopAndRemoveElements(gstAudioOutput->gstElement());
            }

            capturePipeline.stopAndRemoveElements(gstAudioInput->gstElement(), gstAudioTee);
            gstAudioTee = {};
        }

        gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
        if (gstAudioInput) {
            Q_ASSERT(gstAudioTee.isNull());
            gstAudioTee = QGstElement::createFromFactory("tee", "audiotee");
            gstAudioTee.set("allow-not-linked", true);
            capturePipeline.add(gstAudioInput->gstElement(), gstAudioTee);
            qLinkGstElements(gstAudioInput->gstElement(), gstAudioTee);

            if (gstAudioOutput) {
                capturePipeline.add(gstAudioOutput->gstElement());
                gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
                linkTeeToPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
            }

            capturePipeline.syncChildrenState();

            linkTeeToPad(gstAudioTee, encoderAudioSink);
        }
    });
}

void QGstreamerMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    gstVideoOutput->setVideoSink(sink);
}

void QGstreamerMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    capturePipeline.modifyPipelineWhileNotRunning([&] {
        if (gstAudioOutput && gstAudioInput) {
            // If audio input is set, the output is in the pipeline
            qUnlinkGstElements(gstAudioTee, gstAudioOutput->gstElement());
            capturePipeline.stopAndRemoveElements(gstAudioOutput->gstElement());
        }

        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
        if (gstAudioOutput && gstAudioInput) {
            capturePipeline.add(gstAudioOutput->gstElement());
            capturePipeline.syncChildrenState();
            linkTeeToPad(gstAudioTee, gstAudioOutput->gstElement().staticPad("sink"));
        }
    });
}

QGstreamerVideoSink *QGstreamerMediaCaptureSession::gstreamerVideoSink() const
{
    return gstVideoOutput ? gstVideoOutput->gstreamerVideoSink() : nullptr;
}

bool QGstreamerMediaCaptureSession::processBusMessage(const QGstreamerMessage &msg)
{
    switch (msg.type()) {
    case GST_MESSAGE_ERROR:
        return processBusMessageError(msg);

    case GST_MESSAGE_LATENCY:
        return processBusMessageLatency(msg);

    default:
        break;
    }

    return false;
}

bool QGstreamerMediaCaptureSession::processBusMessageError(const QGstreamerMessage &msg)
{
    QUniqueGErrorHandle error;
    QUniqueGStringHandle message;
    gst_message_parse_error(msg.message(), &error, &message);

    qWarning() << "QGstreamerMediaCapture: received error from gstreamer" << error << message;
    capturePipeline.dumpGraph("captureError");

    return false;
}

bool QGstreamerMediaCaptureSession::processBusMessageLatency(const QGstreamerMessage &)
{
    capturePipeline.recalculateLatency();
    return false;
}

QT_END_NAMESPACE
