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

namespace {

QGstElement makeTee(const char *name)
{
    QGstElement tee = QGstElement::createFromFactory("tee", name);
    tee.set("allow-not-linked", true);
    return tee;
}

template <typename Functor>
void executeWhilePadsAreIdle(QSpan<QGstPad> pads, Functor &&f)
{
    if (pads.isEmpty())
        f();

    if (!pads.front())
        return executeWhilePadsAreIdle(pads.subspan(1), f);

    if (pads.size() == 1)
        pads.front().modifyPipelineInIdleProbe(f);
    else {
        auto remain = pads.subspan(1);
        pads.front().modifyPipelineInIdleProbe([&] {
            executeWhilePadsAreIdle(remain, f);
        });
    }
}

void setStateOnElements(QSpan<const QGstElement> elements, GstState state)
{
    for (QGstElement element : elements)
        if (element)
            element.setState(state);
}

void finishStateChangeOnElements(QSpan<const QGstElement> elements)
{
    for (QGstElement element : elements)
        if (element)
            element.finishStateChange();
}

} // namespace

q23::expected<QPlatformMediaCaptureSession *, QString> QGstreamerMediaCaptureSession::create()
{
    auto videoOutput = QGstreamerVideoOutput::create();
    if (!videoOutput)
        return q23::unexpected{ videoOutput.error() };

    static const auto error = qGstErrorMessageIfElementsNotAvailable("tee", "capsfilter");
    if (error)
        return q23::unexpected{ *error };

    return new QGstreamerMediaCaptureSession(videoOutput.value());
}

QGstreamerMediaCaptureSession::QGstreamerMediaCaptureSession(QGstreamerVideoOutput *videoOutput)
    : capturePipeline{
          QGstPipeline::create("mediaCapturePipeline"),
      },
      gstAudioTee{
          makeTee("audioTee"),
      },
      audioSrcPadForEncoder{ gstAudioTee.getRequestPad("src_%u") },
      audioSrcPadForOutput{ gstAudioTee.getRequestPad("src_%u") },
      gstVideoTee{
          makeTee("videoTee"),
      },
      videoSrcPadForEncoder{ gstVideoTee.getRequestPad("src_%u") },
      videoSrcPadForOutput{ gstVideoTee.getRequestPad("src_%u") },
      videoSrcPadForImageCapture{ gstVideoTee.getRequestPad("src_%u") },
      gstVideoOutput(videoOutput)
{
    gstVideoOutput->setParent(this);
    gstVideoOutput->setIsPreview();

    capturePipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    capturePipeline.set("message-forward", true);

    // Use system clock to drive all elements in the pipeline. Otherwise,
    // the clock is sourced from the elements (e.g. from an audio source).
    // Since the elements are added and removed dynamically the clock would
    // also change causing lost of synchronization in the pipeline.

    QGstClockHandle systemClock{
        gst_system_clock_obtain(),
        QGstClockHandle::HasRef,
    };
    gst_pipeline_use_clock(capturePipeline.pipeline(), systemClock.get());

    // This is the recording pipeline with only live sources, thus the pipeline
    // will be always in the playing state.
    capturePipeline.setState(GST_STATE_PLAYING);
    gstVideoOutput->setActive(true);

    capturePipeline.dumpGraph("initial");
}

QGstPad QGstreamerMediaCaptureSession::imageCaptureSink()
{
    return m_imageCapture ? m_imageCapture->gstElement().staticPad("sink") : QGstPad{};
}

QGstPad QGstreamerMediaCaptureSession::videoOutputSink()
{
    return gstVideoOutput ? gstVideoOutput->gstElement().staticPad("sink") : QGstPad{};
}

QGstPad QGstreamerMediaCaptureSession::audioOutputSink()
{
    return gstAudioOutput ? gstAudioOutput->gstElement().staticPad("sink") : QGstPad{};
}

QGstreamerMediaCaptureSession::~QGstreamerMediaCaptureSession()
{
    setMediaRecorder(nullptr);
    setImageCapture(nullptr);
    setCamera(nullptr);
    capturePipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    capturePipeline.setStateSync(GST_STATE_READY);
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
    std::array padsToSync = {
        videoSrcPadForEncoder,
        videoSrcPadForImageCapture,
        videoSrcPadForOutput,
        gstVideoTee.sink(),
    };

    QGstElement cameraElement = gstCamera->gstElement();
    QGstElement videoOutputElement = gstVideoOutput->gstElement();

    if (activate) {
        gstCamera->setCaptureSession(this);
        capturePipeline.add(gstVideoTee);

        executeWhilePadsAreIdle(padsToSync, [&] {
            capturePipeline.add(cameraElement);
            if (videoOutputElement)
                capturePipeline.add(videoOutputElement);

            if (m_currentRecorderState && m_currentRecorderState->videoSink)
                videoSrcPadForEncoder.link(m_currentRecorderState->videoSink);
            if (videoOutputElement)
                videoSrcPadForOutput.link(videoOutputSink());
            if (m_imageCapture)
                videoSrcPadForImageCapture.link(imageCaptureSink());

            qLinkGstElements(cameraElement, gstVideoTee);

            setStateOnElements({ gstVideoTee, cameraElement, videoOutputElement },
                               GST_STATE_PLAYING);
        });

        finishStateChangeOnElements({ gstVideoTee, cameraElement, videoOutputElement });

        for (QGstElement addedElement : { gstVideoTee, cameraElement, videoOutputElement })
            addedElement.finishStateChange();

    } else {
        executeWhilePadsAreIdle(padsToSync, [&] {
            for (QGstPad &pad : padsToSync)
                pad.unlinkPeer();
        });
        capturePipeline.stopAndRemoveElements(cameraElement, gstVideoTee, videoOutputElement);

        gstCamera->setCaptureSession(nullptr);
    }

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

    videoSrcPadForEncoder.modifyPipelineInIdleProbe([&] {
        if (m_imageCapture) {
            qUnlinkGstElements(gstVideoTee, m_imageCapture->gstElement());
            capturePipeline.stopAndRemoveElements(m_imageCapture->gstElement());
            m_imageCapture->setCaptureSession(nullptr);
        }

        m_imageCapture = control;

        if (m_imageCapture) {
            capturePipeline.add(m_imageCapture->gstElement());
            videoSrcPadForImageCapture.link(imageCaptureSink());
            m_imageCapture->setCaptureSession(this);
            m_imageCapture->gstElement().setState(GST_STATE_PLAYING);
        }
    });
    if (m_imageCapture)
        m_imageCapture->gstElement().finishStateChange();

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

void QGstreamerMediaCaptureSession::linkAndStartEncoder(RecorderElements recorder,
                                                        const QMediaMetaData &metadata)
{
    Q_ASSERT(!m_currentRecorderState);

    std::array padsToSync = {
        audioSrcPadForEncoder,
        videoSrcPadForEncoder,
    };

    executeWhilePadsAreIdle(padsToSync, [&] {
        capturePipeline.add(recorder.encodeBin, recorder.fileSink);
        qLinkGstElements(recorder.encodeBin, recorder.fileSink);

        applyMetaDataToTagSetter(metadata, recorder.encodeBin);

        if (recorder.videoSink) {
            QGstCaps capsFromCamera = gstVideoTee.sink().currentCaps();

            encoderVideoCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderVideoCapsFilter");
            encoderVideoCapsFilter.set("caps", capsFromCamera);

            capturePipeline.add(encoderVideoCapsFilter);
            encoderVideoCapsFilter.src().link(recorder.videoSink);
            videoSrcPadForEncoder.link(encoderVideoCapsFilter.sink());
        }

        if (recorder.audioSink) {
            QGstCaps capsFromInput = gstAudioTee.sink().currentCaps();

            encoderAudioCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderAudioCapsFilter");

            encoderAudioCapsFilter.set("caps", capsFromInput);

            capturePipeline.add(encoderAudioCapsFilter);

            encoderAudioCapsFilter.src().link(recorder.audioSink);
            audioSrcPadForEncoder.link(encoderAudioCapsFilter.sink());
        }
        setStateOnElements({ recorder.encodeBin, recorder.fileSink, encoderVideoCapsFilter,
                             encoderAudioCapsFilter },
                           GST_STATE_PLAYING);

        GstEvent *event = gst_event_new_reconfigure();
        gst_element_send_event(recorder.fileSink.element(), event);
    });

    finishStateChangeOnElements({ recorder.encodeBin, recorder.fileSink, encoderVideoCapsFilter,
                                  encoderAudioCapsFilter });

    m_currentRecorderState = std::move(recorder);
}

void QGstreamerMediaCaptureSession::unlinkRecorder()
{
    std::array padsToSync = {
        audioSrcPadForEncoder,
        videoSrcPadForEncoder,
    };

    executeWhilePadsAreIdle(padsToSync, [&] {
        if (encoderVideoCapsFilter)
            qUnlinkGstElements(gstVideoTee, encoderVideoCapsFilter);

        if (encoderAudioCapsFilter)
            qUnlinkGstElements(gstAudioTee, encoderAudioCapsFilter);
    });

    if (encoderVideoCapsFilter) {
        capturePipeline.stopAndRemoveElements(encoderVideoCapsFilter);
        encoderVideoCapsFilter = {};
    }

    if (encoderAudioCapsFilter) {
        capturePipeline.stopAndRemoveElements(encoderAudioCapsFilter);
        encoderAudioCapsFilter = {};
    }

    m_currentRecorderState->encodeBin.sendEos();
}

void QGstreamerMediaCaptureSession::finalizeRecorder()
{
    capturePipeline.stopAndRemoveElements(m_currentRecorderState->encodeBin,
                                          m_currentRecorderState->fileSink);

    m_currentRecorderState = std::nullopt;
}

const QGstPipeline &QGstreamerMediaCaptureSession::pipeline() const
{
    return capturePipeline;
}

void QGstreamerMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (gstAudioInput == input)
        return;

    if (input && !gstAudioInput) {
        // a new input is connected, we need to add/link the audio tee and audio tee

        capturePipeline.add(gstAudioTee);

        std::array padsToSync = {
            audioSrcPadForEncoder,
            audioSrcPadForOutput,
            gstAudioTee.sink(),
        };

        executeWhilePadsAreIdle(padsToSync, [&] {
            if (m_currentRecorderState && m_currentRecorderState->audioSink)
                audioSrcPadForEncoder.link(m_currentRecorderState->audioSink);
            if (gstAudioOutput) {
                capturePipeline.add(gstAudioOutput->gstElement());
                audioSrcPadForOutput.link(audioOutputSink());
            }

            gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
            capturePipeline.add(gstAudioInput->gstElement());

            qLinkGstElements(gstAudioInput->gstElement(), gstAudioTee);

            gstAudioTee.setState(GST_STATE_PLAYING);
            if (gstAudioOutput)
                gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
            gstAudioInput->gstElement().setState(GST_STATE_PLAYING);
        });

    } else if (!input && gstAudioInput) {
        // input has been removed, unlink and remove audio output and audio tee

        std::array padsToSync = {
            audioSrcPadForEncoder,
            audioSrcPadForOutput,
            gstAudioTee.sink(),
        };

        executeWhilePadsAreIdle(padsToSync, [&] {
            for (QGstPad &pad : padsToSync)
                pad.unlinkPeer();
        });

        capturePipeline.stopAndRemoveElements(gstAudioTee);
        if (gstAudioOutput)
            capturePipeline.stopAndRemoveElements(gstAudioOutput->gstElement());
        capturePipeline.stopAndRemoveElements(gstAudioInput->gstElement());

        gstAudioInput = nullptr;
    } else {
        QGstElement oldInputElement = gstAudioInput->gstElement();

        gstAudioTee.sink().modifyPipelineInIdleProbe([&] {
            oldInputElement.sink().unlinkPeer();
            gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
            capturePipeline.add(gstAudioInput->gstElement());

            qLinkGstElements(gstAudioInput->gstElement(), gstAudioTee);

            gstAudioInput->gstElement().setState(GST_STATE_PLAYING);
        });

        gstAudioInput->gstElement().finishStateChange();

        capturePipeline.stopAndRemoveElements(gstAudioInput->gstElement());
    }
}

void QGstreamerMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    auto *gstSink = sink ? static_cast<QGstreamerVideoSink *>(sink->platformVideoSink()) : nullptr;
    if (gstSink)
        gstSink->setAsync(false);

    gstVideoOutput->setVideoSink(sink);
    capturePipeline.dumpGraph("setVideoPreview");
}

void QGstreamerMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (gstAudioOutput == output)
        return;

    auto *gstOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstOutput)
        gstOutput->setAsync(false);

    if (!gstAudioInput) {
        // audio output is not active, since there is no audio input
        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
    } else {
        QGstElement oldOutputElement =
                gstAudioOutput ? gstAudioOutput->gstElement() : QGstElement{};
        gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);

        audioSrcPadForOutput.modifyPipelineInIdleProbe([&] {
            if (oldOutputElement)
                oldOutputElement.sink().unlinkPeer();

            if (gstAudioOutput) {
                capturePipeline.add(gstAudioOutput->gstElement());
                audioSrcPadForOutput.link(gstAudioOutput->gstElement().staticPad("sink"));
                gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
            }
        });

        if (gstAudioOutput)
            gstAudioOutput->gstElement().finishStateChange();

        if (oldOutputElement)
            capturePipeline.stopAndRemoveElements(oldOutputElement);
    }
}

QGstreamerVideoSink *QGstreamerMediaCaptureSession::gstreamerVideoSink() const
{
    return gstVideoOutput ? gstVideoOutput->gstreamerVideoSink() : nullptr;
}

bool QGstreamerMediaCaptureSession::processBusMessage(const QGstreamerMessage &msg)
{
    if (m_mediaRecorder)
        m_mediaRecorder->processBusMessage(msg);

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
