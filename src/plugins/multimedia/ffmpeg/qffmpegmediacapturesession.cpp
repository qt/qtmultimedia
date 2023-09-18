// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediacapturesession_p.h"

#include "private/qplatformaudioinput_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "private/qplatformsurfacecapture_p.h"
#include "qffmpegimagecapture_p.h"
#include "qffmpegmediarecorder_p.h"
#include "private/qplatformcamera_p.h"
#include "qvideosink.h"
#include "qffmpegaudioinput_p.h"
#include "qaudiosink.h"
#include "qaudiobuffer.h"
#include "qaudiooutput.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcFFmpegMediaCaptureSession, "qt.multimedia.ffmpeg.mediacapturesession")

static int preferredAudioSinkBufferSize(const QFFmpegAudioInput &input)
{
    // Heuristic params to avoid jittering
    // TODO: investigate the reason of jittering and probably reduce the factor
    constexpr int BufferSizeFactor = 2;
    constexpr int BufferSizeExceeding = 4096;

    return input.bufferSize() * BufferSizeFactor + BufferSizeExceeding;
}

QFFmpegMediaCaptureSession::QFFmpegMediaCaptureSession()
{
    connect(this, &QFFmpegMediaCaptureSession::primaryActiveVideoSourceChanged, this,
            &QFFmpegMediaCaptureSession::updateVideoFrameConnection);
}

QFFmpegMediaCaptureSession::~QFFmpegMediaCaptureSession() = default;

QPlatformCamera *QFFmpegMediaCaptureSession::camera()
{
    return m_camera;
}

void QFFmpegMediaCaptureSession::setCamera(QPlatformCamera *camera)
{
    if (setVideoSource(m_camera, camera))
        emit cameraChanged();
}

QPlatformSurfaceCapture *QFFmpegMediaCaptureSession::screenCapture()
{
    return m_screenCapture;
}

void QFFmpegMediaCaptureSession::setScreenCapture(QPlatformSurfaceCapture *screenCapture)
{
    if (setVideoSource(m_screenCapture, screenCapture))
        emit screenCaptureChanged();
}

QPlatformSurfaceCapture *QFFmpegMediaCaptureSession::windowCapture()
{
    return m_windowCapture;
}

void QFFmpegMediaCaptureSession::setWindowCapture(QPlatformSurfaceCapture *windowCapture)
{
    if (setVideoSource(m_windowCapture, windowCapture))
        emit windowCaptureChanged();
}

QPlatformImageCapture *QFFmpegMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QFFmpegMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    if (m_imageCapture == imageCapture)
        return;

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = static_cast<QFFmpegImageCapture *>(imageCapture);

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(this);

    emit imageCaptureChanged();
}

void QFFmpegMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *recorder)
{
    auto *r = static_cast<QFFmpegMediaRecorder *>(recorder);
    if (m_mediaRecorder == r)
        return;

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(nullptr);
    m_mediaRecorder = r;
    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(this);

    emit encoderChanged();
}

QPlatformMediaRecorder *QFFmpegMediaCaptureSession::mediaRecorder()
{
    return m_mediaRecorder;
}

void QFFmpegMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    qCDebug(qLcFFmpegMediaCaptureSession)
            << "set audio input:" << (input ? input->device.description() : "null");

    auto ffmpegAudioInput = dynamic_cast<QFFmpegAudioInput *>(input);
    Q_ASSERT(!!input == !!ffmpegAudioInput);

    if (m_audioInput == ffmpegAudioInput)
        return;

    if (m_audioInput)
        m_audioInput->q->disconnect(this);

    m_audioInput = ffmpegAudioInput;
    if (m_audioInput)
        // TODO: implement the signal in QPlatformAudioInput and connect to it, QTBUG-112294
        connect(m_audioInput->q, &QAudioInput::deviceChanged, this,
                &QFFmpegMediaCaptureSession::updateAudioSink);

    updateAudioSink();
}

void QFFmpegMediaCaptureSession::updateAudioSink()
{
    if (m_audioSink) {
        m_audioSink->reset();
        m_audioSink.reset();
    }

    if (!m_audioInput || !m_audioOutput)
        return;

    auto format = m_audioInput->device.preferredFormat();

    if (!m_audioOutput->device.isFormatSupported(format))
        qWarning() << "Audio source format" << format << "is not compatible with the audio output";

    m_audioSink = std::make_unique<QAudioSink>(m_audioOutput->device, format);

    m_audioBufferSize = preferredAudioSinkBufferSize(*m_audioInput);
    m_audioSink->setBufferSize(m_audioBufferSize);

    qCDebug(qLcFFmpegMediaCaptureSession)
            << "Create audiosink, format:" << format << "bufferSize:" << m_audioSink->bufferSize()
            << "output device:" << m_audioOutput->device.description();

    m_audioIODevice = m_audioSink->start();
    if (m_audioIODevice) {
        connect(m_audioInput, &QFFmpegAudioInput::newAudioBuffer, m_audioSink.get(),
                [this](const QAudioBuffer &buffer) {
                    if (m_audioBufferSize < preferredAudioSinkBufferSize(*m_audioInput)) {
                        qCDebug(qLcFFmpegMediaCaptureSession)
                                << "Recreate audiosink due to small buffer size:"
                                << m_audioBufferSize;

                        updateAudioSink();
                    }

                    const auto written =
                            m_audioIODevice->write(buffer.data<const char>(), buffer.byteCount());

                    if (written < buffer.byteCount())
                        qCWarning(qLcFFmpegMediaCaptureSession)
                                << "Not all bytes written:" << written << "vs"
                                << buffer.byteCount();
                });
    } else {
        qWarning() << "Failed to start audiosink push mode";
    }

    updateVolume();
}

void QFFmpegMediaCaptureSession::updateVolume()
{
    if (m_audioSink)
        m_audioSink->setVolume(m_audioOutput->muted ? 0.f : m_audioOutput->volume);
}

QPlatformAudioInput *QFFmpegMediaCaptureSession::audioInput()
{
    return m_audioInput;
}

void QFFmpegMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    if (std::exchange(m_videoSink, sink) == sink)
        return;

    updateVideoFrameConnection();
}

void QFFmpegMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    qCDebug(qLcFFmpegMediaCaptureSession)
            << "set audio output:" << (output ? output->device.description() : "null");

    if (m_audioOutput == output)
        return;

    if (m_audioOutput)
        m_audioOutput->q->disconnect(this);

    m_audioOutput = output;
    if (m_audioOutput) {
        // TODO: implement the signals in QPlatformAudioOutput and connect to them, QTBUG-112294
        connect(m_audioOutput->q, &QAudioOutput::deviceChanged, this,
                &QFFmpegMediaCaptureSession::updateAudioSink);
        connect(m_audioOutput->q, &QAudioOutput::volumeChanged, this,
                &QFFmpegMediaCaptureSession::updateVolume);
        connect(m_audioOutput->q, &QAudioOutput::mutedChanged, this,
                &QFFmpegMediaCaptureSession::updateVolume);
    }

    updateAudioSink();
}

void QFFmpegMediaCaptureSession::updateVideoFrameConnection()
{
    disconnect(m_videoFrameConnection);

    if (m_primaryActiveVideoSource && m_videoSink) {
        // deliver frames directly to video sink;
        // AutoConnection type might be a pessimization due to an extra queuing
        // TODO: investigate and integrate direct connection
        m_videoFrameConnection =
                connect(m_primaryActiveVideoSource, &QPlatformVideoSource::newVideoFrame,
                        m_videoSink, &QVideoSink::setVideoFrame);
    }
}

void QFFmpegMediaCaptureSession::updatePrimaryActiveVideoSource()
{
    auto sources = activeVideoSources();
    auto source = sources.empty() ? nullptr : sources.front();
    if (std::exchange(m_primaryActiveVideoSource, source) != source)
        emit primaryActiveVideoSourceChanged();
}

template<typename VideoSource>
bool QFFmpegMediaCaptureSession::setVideoSource(QPointer<VideoSource> &source,
                                                VideoSource *newSource)
{
    if (source == newSource)
        return false;

    if (auto prevSource = std::exchange(source, newSource)) {
        prevSource->setCaptureSession(nullptr);
        prevSource->disconnect(this);
    }

    if (source) {
        source->setCaptureSession(this);
        connect(source, &QPlatformVideoSource::activeChanged, this,
                &QFFmpegMediaCaptureSession::updatePrimaryActiveVideoSource);
        connect(source, &QObject::destroyed, this,
                &QFFmpegMediaCaptureSession::updatePrimaryActiveVideoSource, Qt::QueuedConnection);
    }

    updatePrimaryActiveVideoSource();

    return true;
}

QPlatformVideoSource *QFFmpegMediaCaptureSession::primaryActiveVideoSource()
{
    return m_primaryActiveVideoSource;
}

QT_END_NAMESPACE

#include "moc_qffmpegmediacapturesession_p.cpp"
