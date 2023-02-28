// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediarecorder_p.h"
#include "qwasmmediacapturesession_p.h"
#include <private/qplatformmediadevices_p.h>
#include "qwasmcamera_p.h"
#include "qwasmaudioinput_p.h"

#include <private/qstdweb_p.h>
#include <QtCore/QIODevice>
#include <QFile>
#include <QTimer>
#include <QDebug>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qWasmMediaRecorder, "qt.multimedia.wasm.mediarecorder")

QWasmMediaRecorder::QWasmMediaRecorder(QMediaRecorder *parent)
    : QPlatformMediaRecorder(parent)
{
    m_durationTimer.reset(new QElapsedTimer());
    QPlatformMediaDevices::instance(); // initialize getUserMedia
}

QWasmMediaRecorder::~QWasmMediaRecorder()
{
    if (m_outputTarget->isOpen())
        m_outputTarget->close();

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
    QMediaRecorder::RecorderState recordingState = QMediaRecorder::StoppedState;

    if (!m_mediaRecorder.isUndefined()) {
        std::string state = m_mediaRecorder["state"].as<std::string>();
        if (state == "recording")
            recordingState = QMediaRecorder::RecordingState;
        else if (state == "paused")
            recordingState = QMediaRecorder::PausedState;
    }
    return recordingState;
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
    initUserMedia();
}

void QWasmMediaRecorder::pause()
{
    if (!m_session || (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull())) {
        qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "could not find MediaRecorder";
        return;
    }
    m_mediaRecorder.call<void>("pause");
    emit stateChanged(state());
}

void QWasmMediaRecorder::resume()
{
    if (!m_session || (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull())) {
        qCDebug(qWasmMediaRecorder)<< Q_FUNC_INFO << "could not find MediaRecorder";
        return;
    }

    m_mediaRecorder.call<void>("resume");
    emit stateChanged(state());
}

void QWasmMediaRecorder::stop()
{
    if (!m_session || (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull())) {
        qCDebug(qWasmMediaRecorder)<< Q_FUNC_INFO << "could not find MediaRecorder";
        return;
    }
    if (m_mediaRecorder["state"].as<std::string>() == "recording")
        m_mediaRecorder.call<void>("stop");
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
    } else {
        qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "has audio";
        stream = static_cast<QWasmAudioInput *>(m_session->audioInput())->mediaStream();

        if (stream.isNull() || stream.isUndefined()) {
            qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "Audio input stream not found";
            return;
        }
    }
    if (stream.isNull() || stream.isUndefined()) {
         qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "No input stream found";
         return;
    }

    setStream(stream);
}

void QWasmMediaRecorder::startAudioRecording()
{
    startStream();
}

void QWasmMediaRecorder::setStream(emscripten::val stream)
{
    emscripten::val emMediaSettings = emscripten::val::object();
    QMediaFormat::VideoCodec videoCodec = m_mediaSettings.videoCodec();
    QMediaFormat::AudioCodec audioCodec = m_mediaSettings.audioCodec();
    QMediaFormat::FileFormat fileFormat = m_mediaSettings.fileFormat();

    // mime and codecs
    QString mimeCodec;
    if (!m_mediaSettings.mimeType().name().isEmpty()) {
        mimeCodec = m_mediaSettings.mimeType().name();

        if (videoCodec != QMediaFormat::VideoCodec::Unspecified)
            mimeCodec += QStringLiteral(": codecs=");

        if (audioCodec != QMediaFormat::AudioCodec::Unspecified) {
            // TODO
        }

        if (fileFormat != QMediaFormat::UnspecifiedFormat)
            mimeCodec += QMediaFormat::fileFormatName(m_mediaSettings.fileFormat());

        emMediaSettings.set("mimeType", mimeCodec.toStdString());
    }

    if (m_mediaSettings.audioBitRate() > 0)
        emMediaSettings.set("audioBitsPerSecond", emscripten::val(m_mediaSettings.audioBitRate()));

    if (m_mediaSettings.videoBitRate() > 0)
        emMediaSettings.set("videoBitsPerSecond", emscripten::val(m_mediaSettings.videoBitRate()));

    // create the MediaRecorder, and set up data callback
    m_mediaRecorder = emscripten::val::global("MediaRecorder").new_(stream, emMediaSettings);

    qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "m_mediaRecorder state:"
        << QString::fromStdString(m_mediaRecorder["state"].as<std::string>());

    if (m_mediaRecorder.isNull() || m_mediaRecorder.isUndefined()) {
        qWarning() << "MediaRecorder could not be found";
        return;
    }
    m_mediaRecorder.set("data-mediarecordercontext",
                        emscripten::val(quintptr(reinterpret_cast<void *>(this))));

    if (!m_mediaStreamDataAvailable.isNull()) {
        m_mediaStreamDataAvailable.reset();
        m_mediaStreamStopped.reset();
        m_mediaStreamError.reset();
        m_mediaStreamStart.reset();
        m_mediaStreamPause.reset();
        m_mediaStreamResume.reset();
    }

    // dataavailable
    auto callback = [](emscripten::val blob) {
        if (blob.isUndefined() || blob.isNull()) {
            qCDebug(qWasmMediaRecorder) << "blob is null";
            return;
        }
        if (blob["target"].isUndefined() || blob["target"].isNull())
            return;
        if (blob["data"].isUndefined() || blob["data"].isNull())
            return;
        if (blob["target"]["data-mediarecordercontext"].isUndefined()
            || blob["target"]["data-mediarecordercontext"].isNull())
            return;

        QWasmMediaRecorder *recorder = reinterpret_cast<QWasmMediaRecorder *>(
                blob["target"]["data-mediarecordercontext"].as<quintptr>());

        if (recorder) {
            const double timeCode =
                    blob.hasOwnProperty("timecode") ? blob["timecode"].as<double>() : 0;
            recorder->audioDataAvailable(blob["data"], timeCode);
        }
    };

    m_mediaStreamDataAvailable.reset(
            new qstdweb::EventCallback(m_mediaRecorder, "dataavailable", callback));

    // stopped
    auto stoppedCallback = [](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qCDebug(qWasmMediaRecorder) << "event is null";
            return;
        }
        qCDebug(qWasmMediaRecorder)
                << "STOPPED: state changed"
                << QString::fromStdString(event["target"]["state"].as<std::string>());

        QWasmMediaRecorder *recorder = reinterpret_cast<QWasmMediaRecorder *>(
                event["target"]["data-mediarecordercontext"].as<quintptr>());

        if (recorder) {
            recorder->m_isRecording = false;
            recorder->m_durationTimer->invalidate();

            emit recorder->stateChanged(recorder->state());
        }
    };

    m_mediaStreamStopped.reset(
            new qstdweb::EventCallback(m_mediaRecorder, "stop", stoppedCallback));

    // error
    auto errorCallback = [](emscripten::val theError) {
        if (theError.isUndefined() || theError.isNull()) {
            qCDebug(qWasmMediaRecorder) << "error is null";
            return;
        }
        qCDebug(qWasmMediaRecorder)
                << theError["code"].as<int>()
                << QString::fromStdString(theError["message"].as<std::string>());

        QWasmMediaRecorder *recorder = reinterpret_cast<QWasmMediaRecorder *>(
                theError["target"]["data-mediarecordercontext"].as<quintptr>());

        if (recorder) {
            recorder->error(QMediaRecorder::ResourceError,
                            QString::fromStdString(theError["message"].as<std::string>()));
            emit recorder->stateChanged(recorder->state());
        }
    };

    m_mediaStreamError.reset(new qstdweb::EventCallback(m_mediaRecorder, "error", errorCallback));

    // start
    auto startCallback = [](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qCDebug(qWasmMediaRecorder) << "event is null";
            return;
        }

        qCDebug(qWasmMediaRecorder)
                << "START: state changed"
                << QString::fromStdString(event["target"]["state"].as<std::string>());

        QWasmMediaRecorder *recorder = reinterpret_cast<QWasmMediaRecorder *>(
                event["target"]["data-mediarecordercontext"].as<quintptr>());

        if (recorder) {
            recorder->m_isRecording = true;
            recorder->m_durationTimer->start();
            emit recorder->stateChanged(recorder->state());
        }
    };

    m_mediaStreamStart.reset(new qstdweb::EventCallback(m_mediaRecorder, "start", startCallback));

    // pause
    auto pauseCallback = [](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qCDebug(qWasmMediaRecorder) << "event is null";
            return;
        }

        qCDebug(qWasmMediaRecorder)
                << "pause: state changed"
                << QString::fromStdString(event["target"]["state"].as<std::string>());

        QWasmMediaRecorder *recorder = reinterpret_cast<QWasmMediaRecorder *>(
                event["target"]["data-mediarecordercontext"].as<quintptr>());

        if (recorder) {
            recorder->m_isRecording = true;
            recorder->m_durationTimer->start();
            emit recorder->stateChanged(recorder->state());
        }
    };

    m_mediaStreamPause.reset(new qstdweb::EventCallback(m_mediaRecorder, "pause", pauseCallback));

    // resume
    auto resumeCallback = [](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qCDebug(qWasmMediaRecorder) << "event is null";
            return;
        }

        qCDebug(qWasmMediaRecorder)
                << "resume: state changed"
                << QString::fromStdString(event["target"]["state"].as<std::string>());

        QWasmMediaRecorder *recorder = reinterpret_cast<QWasmMediaRecorder *>(
                event["target"]["data-mediarecordercontext"].as<quintptr>());

        if (recorder) {
            recorder->m_isRecording = true;
            recorder->m_durationTimer->start();
            emit recorder->stateChanged(recorder->state());
        }
    };

    m_mediaStreamResume.reset(
            new qstdweb::EventCallback(m_mediaRecorder, "resume", resumeCallback));

    // set up what options we can
    if (hasCamera())
        setTrackContraints(m_mediaSettings, stream);
    else
        startStream();
}

void QWasmMediaRecorder::audioDataAvailable(emscripten::val blob, double timeCodeDifference)
{
    Q_UNUSED(timeCodeDifference)
    if (blob.isUndefined() || blob.isNull()) {
        qCDebug(qWasmMediaRecorder) << "blob is null";
        return;
    }

    auto fileReader = std::make_shared<qstdweb::FileReader>();

    fileReader->onError([=](emscripten::val theError) {
        error(QMediaRecorder::ResourceError,
              QString::fromStdString(theError["message"].as<std::string>()));
    });

    fileReader->onAbort([=](emscripten::val) {
        error(QMediaRecorder::ResourceError, QStringLiteral("File read aborted"));
    });

    fileReader->onLoad([=](emscripten::val) {
        if (fileReader->val().isNull() || fileReader->val().isUndefined())
            return;
        qstdweb::ArrayBuffer result = fileReader->result();
        if (result.val().isNull() || result.val().isUndefined())
            return;
        QByteArray fileContent = qstdweb::Uint8Array(result).copyToQByteArray();

        if (m_isRecording && !fileContent.isEmpty()) {
            m_durationMs = m_durationTimer->elapsed();
            if (m_outputTarget->isOpen())
                m_outputTarget->write(fileContent, fileContent.length());
            // we've read everything
            emit durationChanged(m_durationMs);
            qCDebug(qWasmMediaRecorder) << "duration changed" << m_durationMs;
        }
    });

    fileReader->readAsArrayBuffer(qstdweb::Blob(blob));
}

// constraints are suggestions, as not all hardware supports all settings
void QWasmMediaRecorder::setTrackContraints(QMediaEncoderSettings &settings, emscripten::val stream)
{
    qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << settings.audioSampleRate();

    if (stream.isUndefined() || stream.isNull()) {
        qCDebug(qWasmMediaRecorder)<< Q_FUNC_INFO << "could not find MediaStream";
        return;
    }

    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    // check which ones are supported
    // emscripten::val allConstraints = mediaDevices.call<emscripten::val>("getSupportedConstraints");
    // browsers only support some settings

    emscripten::val videoParams = emscripten::val::object();
    emscripten::val constraints = emscripten::val::object();

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
                    qWarning() << "setting video params failed error";
                    qCDebug(qWasmMediaRecorder)
                            << theError["code"].as<int>()
                            << QString::fromStdString(theError["message"].as<std::string>());
                    error(QMediaRecorder::ResourceError, QString::fromStdString(theError["message"].as<std::string>()));
                } },
            constraints);
        }
    }
}

// this starts the recording stream
void QWasmMediaRecorder::startStream()
{
    if (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull()) {
        qCDebug(qWasmMediaRecorder) << Q_FUNC_INFO << "could not find MediaStream";
        return;
    }
    qCDebug(qWasmMediaRecorder) << "m_mediaRecorder state:" <<
               QString::fromStdString(m_mediaRecorder["state"].as<std::string>());

    constexpr int sliceSizeInMs = 250; // TODO find what size is best
    m_mediaRecorder.call<void>("start", emscripten::val(sliceSizeInMs));

    /* this method can optionally be passed a timeslice argument with a value in milliseconds.
     * If this is specified, the media will be captured in separate chunks of that duration,
     * rather than the default behavior of recording the media in a single large chunk.*/

    emit stateChanged(state());
}

void QWasmMediaRecorder::setUpFileSink()
{
    QString m_targetFileName = outputLocation().toLocalFile();
    QString suffix = m_mediaSettings.mimeType().preferredSuffix();
    if (m_targetFileName.isEmpty()) {
        m_targetFileName = "/home/web_user/tmp." + suffix;
        QPlatformMediaRecorder::setOutputLocation(m_targetFileName);
    }

    m_outputTarget = new QFile(m_targetFileName, this);
    if (!m_outputTarget->open(QIODevice::WriteOnly)) {
        qWarning() << "target file is not writable";
        return;
    }
}

QT_END_NAMESPACE
