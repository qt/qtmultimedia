// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegmediacapturesession_p.h"

#include <QtCore/qloggingcategory.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpegaudioinput_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegimagecapture_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegmediarecorder_p.h>

#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qplatformaudioinput_p.h>
#include <QtMultimedia/private/qplatformaudiobufferinput_p.h>
#include <QtMultimedia/private/qplatformaudiooutput_p.h>
#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include <QtMultimedia/private/qplatformvideoframeinput_p.h>
#include <QtMultimedia/qaudiobuffer.h>
#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qaudiosink.h>
#include <QtMultimedia/qvideosink.h>

#include <array>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
namespace ranges = QtMultimediaPrivate::ranges;

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegMediaCaptureSession, "qt.multimedia.ffmpeg.mediacapturesession")

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
    connect(
        this,
        &QFFmpegMediaCaptureSession::primaryActiveVideoSourceChanged,
        this,
        [this] {
            updateVideoFrameConnection(nullptr);
        });
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

QPlatformVideoFrameInput *QFFmpegMediaCaptureSession::videoFrameInput()
{
    return m_videoFrameInput;
}

void QFFmpegMediaCaptureSession::setVideoFrameInput(QPlatformVideoFrameInput *input)
{
    if (setVideoSource(m_videoFrameInput, input))
        emit videoFrameInputChanged();
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
            << "set audio input:" << (input ? input->device.description() : u"null"_s);

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

void QFFmpegMediaCaptureSession::setAudioBufferInput(QPlatformAudioBufferInput *input)
{
    // TODO: implement binding to audio sink like setAudioInput does
    m_audioBufferInput = input;
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
        auto writeToDevice = [this](const QAudioBuffer &buffer) {
            if (m_audioBufferSize < preferredAudioSinkBufferSize(*m_audioInput)) {
                qCDebug(qLcFFmpegMediaCaptureSession)
                        << "Recreate audiosink due to small buffer size:" << m_audioBufferSize;

                updateAudioSink();
            }

            const auto written =
                    m_audioIODevice->write(buffer.data<const char>(), buffer.byteCount());

            if (written < buffer.byteCount())
                qCWarning(qLcFFmpegMediaCaptureSession)
                        << "Not all bytes written:" << written << "vs" << buffer.byteCount();
        };
        connect(m_audioInput, &QFFmpegAudioInput::newAudioBuffer, m_audioSink.get(), writeToDevice);
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

QPlatformAudioInput *QFFmpegMediaCaptureSession::audioInput() const
{
    return m_audioInput;
}

void QFFmpegMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    if (std::exchange(m_videoSink, sink) == sink)
        return;

    updateVideoFrameConnection(nullptr);
}

void QFFmpegMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    qCDebug(qLcFFmpegMediaCaptureSession)
            << "set audio output:" << (output ? output->device.description() : u"null"_s);

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

void QFFmpegMediaCaptureSession::onSourceActivating(QPlatformVideoSource &source)
{
    // The source is about to go active and may emit frames from a
    // background thread before setActive() is done executing.
    // We set up the connections ahead of time to make sure
    // always catch the first frame(s).
    updateVideoFrameConnection(&source);
}

void QFFmpegMediaCaptureSession::onSourceActivationFailure(QPlatformVideoSource &)
{
    // The source failed to activate, so it will not become active. Revert to the
    // connection implied by the currently active sources.
    updateVideoFrameConnection(nullptr);
}

// Returns the highest-priority active video source. If pendingActiveSource
// is set, it is treated as active even if it does not report isActive() yet.
QPlatformVideoSource *
QFFmpegMediaCaptureSession::primaryVideoSource(QPlatformVideoSource *pendingActiveSource)
{
    // Priority order must match QPlatformMediaCaptureSession::activeVideoSources().
    const std::array<QPlatformVideoSource *, 4> sourcesByPriority{
        videoFrameInput(), camera(), screenCapture(), windowCapture()
    };

    auto it = ranges::find_if(
        sourcesByPriority,
        [&](QPlatformVideoSource *item) {
            return item && (item == pendingActiveSource || item->isActive());
        });
    if (it != sourcesByPriority.end())
        return *it;

    return nullptr;
}

void QFFmpegMediaCaptureSession::updateVideoFrameConnection(QPlatformVideoSource *pendingActiveSource)
{
    QPlatformVideoSource *source = primaryVideoSource(pendingActiveSource);

    if (m_videoSinkConnection.source == source && m_videoSinkConnection.sink == m_videoSink)
        return;

    disconnect(m_videoSinkConnection.connection);
    m_videoSinkConnection = {};

    if (source && m_videoSink) {
        // AutoConnection type might be a pessimization due to an extra queuing
        // TODO: investigate and integrate direct connection
        m_videoSinkConnection.source = source;
        m_videoSinkConnection.sink = m_videoSink;
        m_videoSinkConnection.connection = connect(
            source,
            &QPlatformVideoSource::newVideoFrame,
            m_videoSink,
            &QVideoSink::setVideoFrame);
    }
}

void QFFmpegMediaCaptureSession::updatePrimaryActiveVideoSource()
{
    QPlatformVideoSource *source = primaryVideoSource(nullptr);
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

std::vector<QAudioBufferSource *> QFFmpegMediaCaptureSession::activeAudioInputs() const
{
    std::vector<QAudioBufferSource *> result;
    if (m_audioInput)
        result.push_back(m_audioInput);

    if (m_audioBufferInput)
        result.push_back(m_audioBufferInput);

    return result;
}

QT_END_NAMESPACE

#include "moc_qffmpegmediacapturesession_p.cpp"
