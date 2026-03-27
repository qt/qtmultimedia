// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediarecorder_p.h"
#include "qwasmmediacapturesession_p.h"
#include <private/qplatformaudiodevices_p.h>
#include <private/qplatformmediaintegration_p.h>
#include "qwasmcamera_p.h"
#include "qwasmaudioinput_p.h"

#include <private/qstdweb_p.h>
#include <QtCore/QIODevice>
#include <QFile>
#include <QDir>
#include <QTimer>
#include <QDebug>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals;

Q_LOGGING_CATEGORY(qWasmMediaRecorder, "qt.multimedia.wasm.mediarecorder")

QWasmMediaRecorder::QWasmMediaRecorder(QMediaRecorder *parent)
    : QPlatformMediaRecorder(parent)
{
    m_durationTimer.reset(new QElapsedTimer());
    QPlatformMediaIntegration::instance()->audioDevices(); // initialize getUserMedia

    m_jsMediaRecorderDevice.reset(new JsMediaRecorder());

    connect(m_jsMediaRecorderDevice.get(), &JsMediaRecorder::started, this,
            [this]() {
                m_isRecording = true;
                m_durationTimer->start();
                emit stateChanged(QMediaRecorder::RecordingState);
            });

    connect(m_jsMediaRecorderDevice.get(), &JsMediaRecorder::stopped, this,
            [this]() {
                m_isRecording = false;
                m_durationMs = m_durationTimer->elapsed();
                emit durationChanged(m_durationMs);

                m_durationTimer->invalidate();
                emit stateChanged(QMediaRecorder::StoppedState);
            });

    connect(m_jsMediaRecorderDevice.get(), &JsMediaRecorder::paused, this,
            [this]() {
                m_isRecording = false;
                m_durationMs = m_durationTimer->elapsed();
                emit durationChanged(m_durationMs);

                m_durationTimer->invalidate();
                emit stateChanged(QMediaRecorder::PausedState);
            });

    connect(m_jsMediaRecorderDevice.get(), &JsMediaRecorder::resumed, this,
            [this]() {
                m_isRecording = true;
                m_durationTimer->start();
            });

    connect(m_jsMediaRecorderDevice.get(), &JsMediaRecorder::streamError, this,
            [this](QMediaRecorder::Error errorCode, const QString &errorMessage) {
                updateError(errorCode, errorMessage);
                emit stateChanged(state());
            });
}

QWasmMediaRecorder::~QWasmMediaRecorder()
{
    if (m_outputTarget->isOpen())
        m_outputTarget->close();

    if (m_jsMediaRecorderDevice->isOpen()) {
        qWarning() << " bytes still available" << m_jsMediaRecorderDevice->bytesAvailable();
        m_jsMediaRecorderDevice->close();
    }

    if (!m_mediaRecorder.isNull()) {
        m_mediaStreamDataAvailable.reset(nullptr);
        m_mediaStreamStopped.reset(nullptr);
        m_mediaStreamError.reset(nullptr);
        m_mediaStreamStart.reset(nullptr);
    }
}

bool QWasmMediaRecorder::isLocationWritable(const QUrl &location) const
{
    return location.isValid() && (location.isLocalFile() || location.isRelative());
}

QMediaRecorder::RecorderState QWasmMediaRecorder::state() const
{
    return m_jsMediaRecorderDevice->currentState();
}

qint64 QWasmMediaRecorder::duration() const
{ // milliseconds
    return m_durationMs;
}

void QWasmMediaRecorder::record(QMediaEncoderSettings &settings)
{
    if (!m_session)
        return;

    m_mediaSettings = settings;
    if (!m_jsMediaRecorderDevice->open(QIODeviceBase::ReadOnly)) {
        qWarning() << "m_jsMediaRecorderDevice is not open";
        return;
    }

    initUserMedia();
    m_jsMediaRecorderDevice->startStreaming();
}

void QWasmMediaRecorder::pause()
{
    if (!m_session) {
        qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "could not find MediaRecorder";
        return;
    }
    m_jsMediaRecorderDevice->pauseStream();
}

void QWasmMediaRecorder::resume()
{
    if (!m_session) {
        qCDebug(qWasmMediaRecorder)<< Q_FUNC_INFO << "could not find MediaRecorder";
        return;
    }
    m_jsMediaRecorderDevice->resumeStream();
}

void QWasmMediaRecorder::stop()
{
    if (!m_session) {
        qCDebug(qWasmMediaRecorder)<< Q_FUNC_INFO << "could not find MediaRecorder";
        return;
    }
    m_jsMediaRecorderDevice->stopStream();
}

void QWasmMediaRecorder::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_session = static_cast<QWasmMediaCaptureSession *>(session);
}

bool QWasmMediaRecorder::hasCamera() const
{
    return m_session && m_session->camera();
}

void QWasmMediaRecorder::initUserMedia()
{
    setUpFileSink();
    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qCDebug(qWasmMediaRecorder) << "MediaDevices are undefined or null";
        return;
    }

    if (!m_session)
        return;
    qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << m_session;

    emscripten::val stream = emscripten::val::undefined();
    if (hasCamera()) {
        qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "has camera";
        QWasmCamera *wasmCamera = reinterpret_cast<QWasmCamera *>(m_session->camera());

        if (wasmCamera) {
            m_jsMediaRecorderDevice->setNeedsCamera(true);
            emscripten::val m_video = wasmCamera->cameraOutput()->surfaceElement();
            if (m_video.isNull() || m_video.isUndefined()) {
                qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO  << "video element not found";
                return;
            }

            stream = m_video["srcObject"];
            if (stream.isNull() || stream.isUndefined()) {
                qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO  << "Video input stream not found";
                return;
            }
        }
    } else if (m_session->hasAudio()) {
        qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "has audio";
        stream = static_cast<QWasmAudioInput *>(m_session->audioInput())->mediaStream();

        if (stream.isNull() || stream.isUndefined()) {
            qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "Audio input stream not found";
            return;
        }
        m_jsMediaRecorderDevice->setNeedsAudio(m_session->hasAudio());
    }
    if (stream.isNull() || stream.isUndefined()) {
         qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "No input stream found";
         return;
    }
    if (!m_jsMediaRecorderDevice->open(QIODeviceBase::ReadOnly)) {
        qWarning() << "m_jsMediaRecorderDevice is not open";
        return;
    }

    QObject::connect(m_jsMediaRecorderDevice.get(), &JsMediaRecorder::readyRead, this, [this]() {

        if (m_jsMediaRecorderDevice->bytesAvailable()) {

            QByteArray mediaData = m_jsMediaRecorderDevice->read(m_jsMediaRecorderDevice->bytesAvailable());

            m_durationMs = m_durationTimer->elapsed();
            if (m_outputTarget->isOpen())
                m_outputTarget->write(mediaData.constData(), mediaData.length());
            // we've read everything
            if (m_durationMs > 0) {
                emit durationChanged(m_durationMs);
                qCDebug(qWasmMediaRecorder) << "duration changed" << m_durationMs;
            }
        }
    });

    m_jsMediaRecorderDevice->setStream(stream);
}

void QWasmMediaRecorder::startAudioRecording()
{
    startStream();
}

void QWasmMediaRecorder::initMediaSettings()
{
    m_hasMediaSettings = true;
}

void QWasmMediaRecorder::setStream(emscripten::val stream)
{
    // set up what options we can
    if (hasCamera())
        setTrackContraints(m_mediaSettings, stream);
    else
        startStream();
}

// constraints are suggestions, as not all hardware supports all settings
void QWasmMediaRecorder::setTrackContraints(QMediaEncoderSettings &settings, emscripten::val stream)
{
    qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO
                                << settings.audioSampleRate()
                                << settings.videoResolution();

    if (stream.isUndefined() || stream.isNull()) {
        qCDebug(qWasmMediaRecorder)<< Q_FUNC_INFO << "could not find MediaStream";
        return;
    }

    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    // check which ones are supported
    emscripten::val allConstraints = mediaDevices.call<emscripten::val>("getSupportedConstraints");
//    browsers only support some settings

    emscripten::val videoParams = emscripten::val::object();
    emscripten::val constraints = emscripten::val::object();
    videoParams.set("resizeMode",std::string("crop-and-scale"));

    if (hasCamera()) {
        if (settings.videoFrameRate() > 0)
            videoParams.set("frameRate", emscripten::val(settings.videoFrameRate()));
        if (settings.videoResolution().height() > 0)
            videoParams.set("height",
                            emscripten::val(settings.videoResolution().height())); // viewportHeight?
        if (settings.videoResolution().width() > 0)
            videoParams.set("width", emscripten::val(settings.videoResolution().width()));

        constraints.set("video", videoParams); // only video here
    }

    emscripten::val audioParams = emscripten::val::object();
    if (settings.audioSampleRate() > 0)
        audioParams.set("sampleRate", emscripten::val(settings.audioSampleRate())); // may not work
    if (settings.audioBitRate() > 0)
        audioParams.set("sampleSize", emscripten::val(settings.audioBitRate())); // may not work
    if (settings.audioChannelCount() > 0)
        audioParams.set("channelCount", emscripten::val(settings.audioChannelCount()));

    constraints.set("audio", audioParams); // only audio here

    if (hasCamera() && stream["active"].as<bool>()) {
        emscripten::val videoTracks = emscripten::val::undefined();
        videoTracks = stream.call<emscripten::val>("getVideoTracks");
        if (videoTracks.isNull() || videoTracks.isUndefined()) {
            qCDebug(qWasmMediaRecorder) << "no video tracks";
            return;
        }
        if (videoTracks["length"].as<int>() > 0) {
            // try to apply the video options
            qstdweb::Promise::make(videoTracks[0], QStringLiteral("applyConstraints"),
                                   { .thenFunc =
                                    [this](emscripten::val result) {
                                        Q_UNUSED(result)
                                        startStream();
                                    },
                                    .catchFunc =
                                    [this](emscripten::val theError) {
                                        qCDebug(qWasmMediaRecorder)
                                        << theError["code"].as<int>()
                                        << QString::fromStdString(theError["message"].as<std::string>());
                                        updateError(QMediaRecorder::ResourceError,
                                                    QString::fromStdString(theError["message"].as<std::string>()));
                                    },
                                    .finallyFunc = []() {},
                                    },
                                   constraints);
        }
    }
    startStream();
}

// this starts the recording stream
void QWasmMediaRecorder::startStream()
{
    m_jsMediaRecorderDevice->startStreaming();
}

void QWasmMediaRecorder::setUpFileSink()
{
    QString m_targetFileName = outputLocation().toLocalFile();

    QString suffix = m_mediaSettings.mimeType().preferredSuffix();
    // what if no mimeType yet?

    if (!m_mediaSettings.mimeType().isValid()) {
        qWarning() << "mimetype not valid, using m4v";
        suffix = QStringLiteral(".m4v");
    }
    if (m_targetFileName.isEmpty()) {
        m_targetFileName =  QDir::homePath() + u"/tmp."_s + suffix;
        QPlatformMediaRecorder::setOutputLocation(QUrl::fromLocalFile(m_targetFileName));
    }

    m_outputTarget = new QFile(m_targetFileName, this);
    if (!m_outputTarget->open(QIODevice::WriteOnly)) {
        qWarning() << "target file is not writable";
        return;
    }
}

QT_END_NAMESPACE
