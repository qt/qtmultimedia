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

#include <QtMultimedia/private/qthreadlocalrhi_p.h>

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
    : m_capturePipeline{
          QGstPipeline::create("mediaCapturePipeline"),
      },
      m_gstAudioTee{
          makeTee("audioTee"),
      },
      m_audioSrcPadForEncoder{ m_gstAudioTee.getRequestPad("src_%u") },
      m_audioSrcPadForOutput{ m_gstAudioTee.getRequestPad("src_%u") },
      m_gstVideoTee{
          makeTee("videoTee"),
      },
      m_videoSrcPadForEncoder{ m_gstVideoTee.getRequestPad("src_%u") },
      m_videoSrcPadForOutput{ m_gstVideoTee.getRequestPad("src_%u") },
      m_videoSrcPadForImageCapture{ m_gstVideoTee.getRequestPad("src_%u") },
      m_gstVideoOutput(videoOutput)
{
    m_gstVideoOutput->setParent(this);

    // NOTE: Creating a GStreamer video sink to be owned by the capture session, any sink created by
    // user would be a pluggable sink connected to this
    m_gstVideoSink = new QGstreamerRelayVideoSink(this);
    m_gstVideoOutput->setVideoSink(m_gstVideoSink);

    m_gstVideoOutput->setIsPreview();

    m_capturePipeline.installMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    m_capturePipeline.set("message-forward", true);

    // Use system clock to drive all elements in the pipeline. Otherwise,
    // the clock is sourced from the elements (e.g. from an audio source).
    // Since the elements are added and removed dynamically the clock would
    // also change causing lost of synchronization in the pipeline.

    QGstClockHandle systemClock{
        gst_system_clock_obtain(),
        QGstClockHandle::HasRef,
    };
    gst_pipeline_use_clock(m_capturePipeline.pipeline(), systemClock.get());

    // This is the recording pipeline with only live sources, thus the pipeline
    // will be always in the playing state.
    m_capturePipeline.setState(GST_STATE_PLAYING);
    m_gstVideoOutput->setActive(true);

    m_capturePipeline.dumpGraph("initial");
}

QGstPad QGstreamerMediaCaptureSession::imageCaptureSink()
{
    return m_imageCapture ? m_imageCapture->gstElement().staticPad("sink") : QGstPad{};
}

QGstPad QGstreamerMediaCaptureSession::videoOutputSink()
{
    return m_gstVideoOutput ? m_gstVideoOutput->gstElement().staticPad("sink") : QGstPad{};
}

QGstPad QGstreamerMediaCaptureSession::audioOutputSink()
{
    return m_gstAudioOutput ? m_gstAudioOutput->gstElement().staticPad("sink") : QGstPad{};
}

QGstreamerMediaCaptureSession::~QGstreamerMediaCaptureSession()
{
    setMediaRecorder(nullptr);
    setImageCapture(nullptr);
    setCamera(nullptr);
    m_capturePipeline.removeMessageFilter(static_cast<QGstreamerBusMessageFilter *>(this));
    m_capturePipeline.setStateSync(GST_STATE_READY);
    m_capturePipeline.setStateSync(GST_STATE_NULL);
}

QPlatformCamera *QGstreamerMediaCaptureSession::camera()
{
    return m_gstCamera;
}

void QGstreamerMediaCaptureSession::setCamera(QPlatformCamera *platformCamera)
{
    auto *camera = static_cast<QGstreamerCameraBase *>(platformCamera);
    if (m_gstCamera == camera)
        return;

    if (m_gstCamera) {
        QObject::disconnect(m_gstCameraActiveConnection);
        if (m_gstVideoTee)
            setCameraActive(false);
    }

    m_gstCamera = camera;

    if (m_gstCamera) {
        m_gstCameraActiveConnection =
                QObject::connect(camera, &QPlatformCamera::activeChanged, this,
                                 &QGstreamerMediaCaptureSession::setCameraActive);
        if (m_gstCamera->isActive())
            setCameraActive(true);
    }

    emit cameraChanged();
}

void QGstreamerMediaCaptureSession::setCameraActive(bool activate)
{
    std::array padsToSync = {
        m_videoSrcPadForEncoder,
        m_videoSrcPadForImageCapture,
        m_videoSrcPadForOutput,
        m_gstVideoTee.sink(),
    };

    QGstElement cameraElement = m_gstCamera->gstElement();
    QGstElement videoOutputElement = m_gstVideoOutput->gstElement();

    if (activate) {
        m_gstCamera->setCaptureSession(this);
        m_capturePipeline.add(m_gstVideoTee);

        executeWhilePadsAreIdle(padsToSync, [&] {
            m_capturePipeline.add(cameraElement);
            if (videoOutputElement)
                m_capturePipeline.add(videoOutputElement);

            if (m_currentRecorderState && m_currentRecorderState->videoSink)
                m_videoSrcPadForEncoder.link(m_currentRecorderState->videoSink);
            if (videoOutputElement)
                m_videoSrcPadForOutput.link(videoOutputSink());
            if (m_imageCapture)
                m_videoSrcPadForImageCapture.link(imageCaptureSink());

            qLinkGstElements(cameraElement, m_gstVideoTee);

            setStateOnElements({ m_gstVideoTee, cameraElement, videoOutputElement },
                               GST_STATE_PLAYING);
        });

        finishStateChangeOnElements({ m_gstVideoTee, cameraElement, videoOutputElement });

        for (QGstElement addedElement : { m_gstVideoTee, cameraElement, videoOutputElement })
            addedElement.finishStateChange();

    } else {
        executeWhilePadsAreIdle(padsToSync, [&] {
            for (QGstPad &pad : padsToSync)
                pad.unlinkPeer();
        });
        m_capturePipeline.stopAndRemoveElements(cameraElement, m_gstVideoTee, videoOutputElement);

        m_gstCamera->setCaptureSession(nullptr);
    }

    m_capturePipeline.dumpGraph("camera");
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

    m_videoSrcPadForEncoder.modifyPipelineInIdleProbe([&] {
        if (m_imageCapture) {
            qUnlinkGstElements(m_gstVideoTee, m_imageCapture->gstElement());
            m_capturePipeline.stopAndRemoveElements(m_imageCapture->gstElement());
            m_imageCapture->setCaptureSession(nullptr);
        }

        m_imageCapture = control;

        if (m_imageCapture) {
            m_capturePipeline.add(m_imageCapture->gstElement());
            m_videoSrcPadForImageCapture.link(imageCaptureSink());
            m_imageCapture->setCaptureSession(this);
            m_imageCapture->gstElement().setState(GST_STATE_PLAYING);
        }
    });
    if (m_imageCapture)
        m_imageCapture->gstElement().finishStateChange();

    m_capturePipeline.dumpGraph("imageCapture");

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
    m_capturePipeline.dumpGraph("encoder");
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
        m_audioSrcPadForEncoder,
        m_videoSrcPadForEncoder,
    };

    executeWhilePadsAreIdle(padsToSync, [&] {
        m_capturePipeline.add(recorder.encodeBin, recorder.fileSink);
        qLinkGstElements(recorder.encodeBin, recorder.fileSink);

        applyMetaDataToTagSetter(metadata, recorder.encodeBin);

        if (recorder.videoSink) {
            QGstCaps capsFromCamera = m_gstVideoTee.sink().currentCaps();

            m_encoderVideoCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderVideoCapsFilter");
            m_encoderVideoCapsFilter.set("caps", capsFromCamera);

            m_capturePipeline.add(m_encoderVideoCapsFilter);
            m_encoderVideoCapsFilter.src().link(recorder.videoSink);
            m_videoSrcPadForEncoder.link(m_encoderVideoCapsFilter.sink());
        }

        if (recorder.audioSink) {
            QGstCaps capsFromInput = m_gstAudioTee.sink().currentCaps();

            m_encoderAudioCapsFilter =
                    QGstElement::createFromFactory("capsfilter", "encoderAudioCapsFilter");

            m_encoderAudioCapsFilter.set("caps", capsFromInput);

            m_capturePipeline.add(m_encoderAudioCapsFilter);

            m_encoderAudioCapsFilter.src().link(recorder.audioSink);
            m_audioSrcPadForEncoder.link(m_encoderAudioCapsFilter.sink());
        }
        setStateOnElements({ recorder.encodeBin, recorder.fileSink, m_encoderVideoCapsFilter,
                             m_encoderAudioCapsFilter },
                           GST_STATE_PLAYING);

        GstEvent *event = gst_event_new_reconfigure();
        gst_element_send_event(recorder.fileSink.element(), event);
    });

    finishStateChangeOnElements({ recorder.encodeBin, recorder.fileSink, m_encoderVideoCapsFilter,
                                  m_encoderAudioCapsFilter });

    m_currentRecorderState = std::move(recorder);
}

void QGstreamerMediaCaptureSession::unlinkRecorder()
{
    std::array padsToSync = {
        m_audioSrcPadForEncoder,
        m_videoSrcPadForEncoder,
    };

    executeWhilePadsAreIdle(padsToSync, [&] {
        if (m_encoderVideoCapsFilter)
            qUnlinkGstElements(m_gstVideoTee, m_encoderVideoCapsFilter);

        if (m_encoderAudioCapsFilter)
            qUnlinkGstElements(m_gstAudioTee, m_encoderAudioCapsFilter);
    });

    if (m_encoderVideoCapsFilter) {
        m_capturePipeline.stopAndRemoveElements(m_encoderVideoCapsFilter);
        m_encoderVideoCapsFilter = {};
    }

    if (m_encoderAudioCapsFilter) {
        m_capturePipeline.stopAndRemoveElements(m_encoderAudioCapsFilter);
        m_encoderAudioCapsFilter = {};
    }

    m_currentRecorderState->encodeBin.sendEos();
}

void QGstreamerMediaCaptureSession::finalizeRecorder()
{
    m_capturePipeline.stopAndRemoveElements(m_currentRecorderState->encodeBin,
                                            m_currentRecorderState->fileSink);

    m_currentRecorderState = std::nullopt;
}

const QGstPipeline &QGstreamerMediaCaptureSession::pipeline() const
{
    return m_capturePipeline;
}

void QGstreamerMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (m_gstAudioInput == input)
        return;

    if (input && !m_gstAudioInput) {
        // a new input is connected, we need to add/link the audio tee and audio tee

        m_capturePipeline.add(m_gstAudioTee);

        std::array padsToSync = {
            m_audioSrcPadForEncoder,
            m_audioSrcPadForOutput,
            m_gstAudioTee.sink(),
        };

        executeWhilePadsAreIdle(padsToSync, [&] {
            if (m_currentRecorderState && m_currentRecorderState->audioSink)
                m_audioSrcPadForEncoder.link(m_currentRecorderState->audioSink);
            if (m_gstAudioOutput) {
                m_capturePipeline.add(m_gstAudioOutput->gstElement());
                m_audioSrcPadForOutput.link(audioOutputSink());
            }

            m_gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
            m_capturePipeline.add(m_gstAudioInput->gstElement());

            qLinkGstElements(m_gstAudioInput->gstElement(), m_gstAudioTee);

            m_gstAudioTee.setState(GST_STATE_PLAYING);
            if (m_gstAudioOutput)
                m_gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
            m_gstAudioInput->gstElement().setState(GST_STATE_PLAYING);
        });

    } else if (!input && m_gstAudioInput) {
        // input has been removed, unlink and remove audio output and audio tee

        std::array padsToSync = {
            m_audioSrcPadForEncoder,
            m_audioSrcPadForOutput,
            m_gstAudioTee.sink(),
        };

        executeWhilePadsAreIdle(padsToSync, [&] {
            for (QGstPad &pad : padsToSync)
                pad.unlinkPeer();
        });

        m_capturePipeline.stopAndRemoveElements(m_gstAudioTee);
        if (m_gstAudioOutput)
            m_capturePipeline.stopAndRemoveElements(m_gstAudioOutput->gstElement());
        m_capturePipeline.stopAndRemoveElements(m_gstAudioInput->gstElement());

        m_gstAudioInput = nullptr;
    } else {
        QGstElement oldInputElement = m_gstAudioInput->gstElement();

        m_gstAudioTee.sink().modifyPipelineInIdleProbe([&] {
            oldInputElement.sink().unlinkPeer();
            m_gstAudioInput = static_cast<QGstreamerAudioInput *>(input);
            m_capturePipeline.add(m_gstAudioInput->gstElement());

            qLinkGstElements(m_gstAudioInput->gstElement(), m_gstAudioTee);

            m_gstAudioInput->gstElement().setState(GST_STATE_PLAYING);
        });

        m_gstAudioInput->gstElement().finishStateChange();

        m_capturePipeline.stopAndRemoveElements(m_gstAudioInput->gstElement());
    }
}

void QGstreamerMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    // Disconnect previous sink
    m_gstVideoSink->disconnectPluggableVideoSink();

    if (!sink)
        return;

    // Connect pluggable sink to native sink
    auto pluggableSink = dynamic_cast<QGstreamerPluggableVideoSink *>(sink->platformVideoSink());
    Q_ASSERT(pluggableSink);
    m_gstVideoSink->connectPluggableVideoSink(pluggableSink);
}

void QGstreamerMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_gstAudioOutput == output)
        return;

    auto *gstOutput = static_cast<QGstreamerAudioOutput *>(output);
    if (gstOutput)
        gstOutput->setAsync(false);

    if (!m_gstAudioInput) {
        // audio output is not active, since there is no audio input
        m_gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);
    } else {
        QGstElement oldOutputElement =
                m_gstAudioOutput ? m_gstAudioOutput->gstElement() : QGstElement{};
        m_gstAudioOutput = static_cast<QGstreamerAudioOutput *>(output);

        m_audioSrcPadForOutput.modifyPipelineInIdleProbe([&] {
            if (oldOutputElement)
                oldOutputElement.sink().unlinkPeer();

            if (m_gstAudioOutput) {
                m_capturePipeline.add(m_gstAudioOutput->gstElement());
                m_audioSrcPadForOutput.link(m_gstAudioOutput->gstElement().staticPad("sink"));
                m_gstAudioOutput->gstElement().setState(GST_STATE_PLAYING);
            }
        });

        if (m_gstAudioOutput)
            m_gstAudioOutput->gstElement().finishStateChange();

        if (oldOutputElement)
            m_capturePipeline.stopAndRemoveElements(oldOutputElement);
    }
}

QGstreamerRelayVideoSink *QGstreamerMediaCaptureSession::gstreamerVideoSink() const
{
    return m_gstVideoOutput ? m_gstVideoOutput->gstreamerVideoSink() : nullptr;
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
    m_capturePipeline.dumpGraph("captureError");

    return false;
}

bool QGstreamerMediaCaptureSession::processBusMessageLatency(const QGstreamerMessage &)
{
    m_capturePipeline.recalculateLatency();
    return false;
}

QT_END_NAMESPACE
