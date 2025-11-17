// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmjs_p.h"
#include <qaudiodevice.h>
#include <qcameradevice.h>

QT_BEGIN_NAMESPACE


JsMediaRecorder::JsMediaRecorder() = default;

bool JsMediaRecorder::open(QIODevice::OpenMode mode)
{
    if (mode.testFlag(QIODevice::WriteOnly))
        return false;
    return QIODevice::open(mode);
}

bool JsMediaRecorder::isSequential() const
{
    return false;
}

qint64 JsMediaRecorder::size() const
{
    return m_buffer.size();
}

bool JsMediaRecorder::seek(qint64 pos)
{
    if (pos >= size())
        return false;
    return QIODevice::seek(pos);
}

qint64 JsMediaRecorder::readData(char *data, qint64 maxSize)
{
    qint64 bytesToRead = qMin(maxSize, (qint64)m_buffer.size());
    memcpy(data, m_buffer.constData(), bytesToRead);
    m_buffer = m_buffer.right(m_buffer.size() - bytesToRead);
    return bytesToRead;
}

qint64 JsMediaRecorder::writeData(const char *, qint64)
{
    Q_UNREACHABLE_RETURN(0);
}

void JsMediaRecorder::audioDataAvailable(emscripten::val aBlob, double timeCodeDifference)
{
    Q_UNUSED(timeCodeDifference)
    if (aBlob.isUndefined() || aBlob.isNull()) {
        qWarning() << "blob is null";
        return;
    }

    auto fileReader = std::make_shared<qstdweb::FileReader>();

    fileReader->onError([=](emscripten::val theError) {
        emit streamError(QMediaRecorder::ResourceError,
                         QString::fromStdString(theError["message"].as<std::string>()));
    });

    fileReader->onAbort([=](emscripten::val) {
        emit streamError(QMediaRecorder::ResourceError, QStringLiteral("File read aborted"));
    });

    fileReader->onLoad([=](emscripten::val) {
        if (fileReader->val().isNull() || fileReader->val().isUndefined())
            return;
        qstdweb::ArrayBuffer result = fileReader->result();
        if (result.val().isNull() || result.val().isUndefined())
            return;

        m_buffer.append(qstdweb::Uint8Array(result).copyToQByteArray());
        emit readyRead();
    });

    fileReader->readAsArrayBuffer(qstdweb::Blob(aBlob));
}

void JsMediaRecorder::setTrackContraints(QMediaEncoderSettings &settings, emscripten::val stream)
{
    if (stream.isUndefined() || stream.isNull()) {
        qWarning()<<  "could not find MediaStream";
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

    if (m_needsCamera) {
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

    if (m_needsCamera && stream["active"].as<bool>()) {
        emscripten::val videoTracks = emscripten::val::undefined();
        videoTracks = stream.call<emscripten::val>("getVideoTracks");
        if (videoTracks.isNull() || videoTracks.isUndefined()) {
            qWarning() << "no video tracks";
            return;
        }
        if (videoTracks["length"].as<int>() > 0) {
            // try to apply the video options, async
            qstdweb::Promise::make(videoTracks[0],
                                   QStringLiteral("applyConstraints"), {
                                        .thenFunc =
                                        [this]([[maybe_unused]] emscripten::val result) {
                                            startStreaming();
                                    },
                                        .catchFunc =
                                        [this](emscripten::val theError) {
                                               qWarning()
                                               << theError["code"].as<int>()
                                               << theError["message"].as<std::string>();
                                               emit streamError(QMediaRecorder::ResourceError,
                                                            QString::fromStdString(theError["message"].as<std::string>()));
                                    },
                                        .finallyFunc = []() {},
                                    },
                                constraints);
        }
    }
}

void JsMediaRecorder::pauseStream()
{
    if (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull()) {
        qWarning() << "could not find MediaRecorder";
        return;
    }
    m_mediaRecorder.call<void>("pause");
}

void JsMediaRecorder::resumeStream()
{
    if (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull()) {
        qWarning() << "could not find MediaRecorder";
        return;
    }

    m_mediaRecorder.call<void>("resume");
}

void JsMediaRecorder::stopStream()
{
    if (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull()) {
        qWarning()<<  "could not find MediaRecorder";
        return;
    }
    if (m_mediaRecorder["state"].as<std::string>() == "recording")
        m_mediaRecorder.call<void>("stop");

}

void JsMediaRecorder::startStreaming()
{
    if (m_mediaRecorder.isUndefined() || m_mediaRecorder.isNull()) {
        qWarning() <<  "could not find MediaStream";
        return;
    }

    constexpr int sliceSizeInMs = 256;
    // AudioWorklets uses 128 by default
    m_mediaRecorder.call<void>("start", emscripten::val(sliceSizeInMs));
}

void JsMediaRecorder::setStream(emscripten::val stream)
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
            qWarning() << "blob is null";
            return;
        }
        if (blob["target"].isUndefined() || blob["target"].isNull())
            return;
        if (blob["data"].isUndefined() || blob["data"].isNull())
            return;
        if (blob["target"]["data-mediarecordercontext"].isUndefined()
            || blob["target"]["data-mediarecordercontext"].isNull())
            return;

        JsMediaRecorder *recorder = reinterpret_cast<JsMediaRecorder *>(
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
    auto stoppedCallback = [this](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qWarning() << "event is null";
            return;
        }
            m_currentState = QMediaRecorder::StoppedState;
        JsMediaRecorder *recorder = reinterpret_cast<JsMediaRecorder *>(
            event["target"]["data-mediarecordercontext"].as<quintptr>());
        emit recorder->stopped();
    };

    m_mediaStreamStopped.reset(
        new qstdweb::EventCallback(m_mediaRecorder, "stop", stoppedCallback));

    // error
    auto errorCallback = [this](emscripten::val theError) {
        if (theError.isUndefined() || theError.isNull()) {
            qWarning() << "error is null";
            return;
        }

        emit streamError(QMediaRecorder::ResourceError,
                         QString::fromStdString(theError["message"].as<std::string>()));
    };

    m_mediaStreamError.reset(new qstdweb::EventCallback(m_mediaRecorder, "error", errorCallback));

    // start
    auto startCallback = [this](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qWarning() << "event is null";
            return;
        }

        JsMediaRecorder *recorder = reinterpret_cast<JsMediaRecorder *>(
            event["target"]["data-mediarecordercontext"].as<quintptr>());
            m_currentState = QMediaRecorder::RecordingState;
        emit recorder->started();
    };

    m_mediaStreamStart.reset(new qstdweb::EventCallback(m_mediaRecorder, "start", startCallback));

    // pause
    auto pauseCallback = [this](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qWarning() << "event is null";
            return;
        }

        JsMediaRecorder *recorder = reinterpret_cast<JsMediaRecorder *>(
            event["target"]["data-mediarecordercontext"].as<quintptr>());
            m_currentState = QMediaRecorder::PausedState;
        emit recorder->paused();
    };

    m_mediaStreamPause.reset(new qstdweb::EventCallback(m_mediaRecorder, "pause", pauseCallback));

    // resume
    auto resumeCallback = [this](emscripten::val event) {
        if (event.isUndefined() || event.isNull()) {
            qWarning() << "event is null";
            return;
        }
            m_currentState = QMediaRecorder::RecordingState;

        JsMediaRecorder *recorder = reinterpret_cast<JsMediaRecorder *>(
            event["target"]["data-mediarecordercontext"].as<quintptr>());
        emit recorder->resumed();
    };

    m_mediaStreamResume.reset(
        new qstdweb::EventCallback(m_mediaRecorder, "resume", resumeCallback));
}

qint64 JsMediaRecorder::bytesAvailable() const
{
    return m_buffer.size();
}


JsMediaInputStream::JsMediaInputStream(QObject *parent)
    : QObject{parent}
{
}

void JsMediaInputStream::setStreamDevice(const std::string &id)
{
    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qWarning() << "No media devices found";
        return;
    }

    qstdweb::PromiseCallbacks getUserMediaCallback{
        // default
        .thenFunc =
        [this](emscripten::val stream) {
            setupMediaStream(stream);
            },
        .catchFunc =
        [](emscripten::val error) {
                qWarning()
                << "setStreamDevice getUserMedia  fail"
                << error["name"].as<std::string>()
                << error["message"].as<std::string>();
            }
    };

    emscripten::val constraints = emscripten::val::object();
    if (m_needsAudio) {
        if (!id.empty() && !m_needsVideo) {
            emscripten::val audioConstraints = emscripten::val::object();
            emscripten::val exactDeviceId = emscripten::val::object();
            exactDeviceId.set("exact", id);
            audioConstraints.set("deviceId", exactDeviceId);
            constraints.set("audio", audioConstraints);
        } else {
            constraints.set("audio", true);
        }
    }

    if (m_needsVideo) {
        emscripten::val videoContraints = emscripten::val::object();
        emscripten::val exactDeviceId = emscripten::val::object();

        if (!id.empty()) {
            exactDeviceId.set("exact", id);
            videoContraints.set("deviceId", exactDeviceId);
        }
        videoContraints.set("resizeMode", std::string("crop-and-scale"));
        constraints.set("video", videoContraints);
    }
    // we do it this way as this prompts user for permissions
    qstdweb::Promise::make(mediaDevices, QStringLiteral("getUserMedia"),
                           std::move(getUserMediaCallback), constraints);
}

void JsMediaInputStream::setupMediaStream(emscripten::val mStream)
{
    m_mediaStream = mStream.call<emscripten::val>("clone");
    auto activeStreamCallback = [=](emscripten::val event) {
        m_active = true;
        emit activated(m_active);
    };
    m_activeStreamEvent.reset(new qstdweb::EventCallback(m_mediaStream, "active", activeStreamCallback));

    auto inactiveStreamCallback = [=](emscripten::val event) {
        m_active = false;
        emit activated(m_active);
    };
    m_inactiveStreamEvent.reset(new qstdweb::EventCallback(m_mediaStream, "inactive", inactiveStreamCallback));
    emit mediaStreamReady();
}

QT_END_NAMESPACE
