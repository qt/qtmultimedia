// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR
// GPL-2.0-only OR GPL-3.0-only

#include <qaudiodevice.h>
#include <qaudiooutput.h>
#include <qwasmaudiooutput_p.h>

#include <QMimeDatabase>
#include <QtCore/qloggingcategory.h>
#include <QMediaDevices>
#include <QUrl>
#include <QFile>
#include <QMimeDatabase>
#include <QFileInfo>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qWasmMediaAudioOutput, "qt.multimedia.wasm.audiooutput")

QWasmAudioOutput::QWasmAudioOutput(QAudioOutput *parent)
    : QPlatformAudioOutput(parent)
{
}

QWasmAudioOutput::~QWasmAudioOutput() = default;

void QWasmAudioOutput::setAudioDevice(const QAudioDevice &audioDevice)
{
    qCDebug(qWasmMediaAudioOutput) << Q_FUNC_INFO << device.id();
    device = audioDevice;
}

void QWasmAudioOutput::setMuted(bool muted)
{
    if (m_audio.isUndefined() || m_audio.isNull()) {
        qCDebug(qWasmMediaAudioOutput) << "Error"
                                       << "Audio element could not be created";
        emit errorOccured(QMediaPlayer::ResourceError,
                          QStringLiteral("Media file could not be opened"));
        return;
    }
    m_audio.set("mute", muted);
}

void QWasmAudioOutput::setVolume(float volume)
{
    if (m_audio.isUndefined() || m_audio.isNull()) {
        qCDebug(qWasmMediaAudioOutput) << "Error"
                                       << "Audio element could not be created";
        emit errorOccured(QMediaPlayer::ResourceError,
                          QStringLiteral("Media file could not be opened"));
        return;
    }
    volume = qBound(qreal(0.0), volume, qreal(1.0));
    m_audio.set("volume", volume);
}

void QWasmAudioOutput::setSource(const QUrl &url)
{
    qCDebug(qWasmMediaAudioOutput) << Q_FUNC_INFO << url;
    if (url.isEmpty()) {
        stop();
        return;
    }

    createAudioElement(device.id().toStdString());

    if (m_audio.isUndefined() || m_audio.isNull()) {
        qCDebug(qWasmMediaAudioOutput) << "Error"
                                       << "Audio element could not be created";
        emit errorOccured(QMediaPlayer::ResourceError,
                          QStringLiteral("Audio element could not be created"));
        return;
    }

    emscripten::val document = emscripten::val::global("document");
    emscripten::val body = document["body"];

    m_audio.set("id", device.id().toStdString());

    body.call<void>("appendChild", m_audio);


    if (url.isLocalFile()) { // is localfile
        qCDebug(qWasmMediaAudioOutput) << "is localfile";
        m_source = url.toLocalFile();

        QFile mediaFile(m_source);
        if (!mediaFile.open(QIODevice::ReadOnly)) {
            qCDebug(qWasmMediaAudioOutput) << "Error"
                                           << "Media file could not be opened";
            emit errorOccured(QMediaPlayer::ResourceError,
                              QStringLiteral("Media file could not be opened"));
            return;
        }

        // local files are relatively small due to browser filesystem being restricted
        QByteArray content = mediaFile.readAll();

        QMimeDatabase db;
        qCDebug(qWasmMediaAudioOutput) << db.mimeTypeForData(content).name();

        qstdweb::Blob contentBlob = qstdweb::Blob::copyFrom(content.constData(), content.size());
        emscripten::val contentUrl =
            qstdweb::window()["URL"].call<emscripten::val>("createObjectURL", contentBlob.val());

        emscripten::val audioSourceElement =
                document.call<emscripten::val>("createElement", std::string("source"));

        audioSourceElement.set("src", contentUrl);

        // work around Safari not being able to read audio from blob URLs.
        QFileInfo info(m_source);
        QMimeType mimeType = db.mimeTypeForFile(info);

        audioSourceElement.set("type", mimeType.name().toStdString());
        m_audio.call<void>("appendChild", audioSourceElement);

        m_audio.call<void>("setAttribute", emscripten::val("srcObject"), contentUrl);

    } else {
        m_source = url.toString();
        m_audio.set("src", m_source.toStdString());
    }
    m_audio.set("id", device.id().toStdString());

    body.call<void>("appendChild", m_audio);
    qCDebug(qWasmMediaAudioOutput) << Q_FUNC_INFO << device.id();

    doElementCallbacks();
}

void QWasmAudioOutput::setSource(QIODevice *stream)
{
    m_audioIODevice = stream;
}

void QWasmAudioOutput::start()
{
    if (m_audio.isNull() || m_audio.isUndefined()) {
        qCDebug(qWasmMediaAudioOutput) << "audio failed to start";
        emit errorOccured(QMediaPlayer::ResourceError,
                          QStringLiteral("Audio element resource error"));
        return;
    }

    m_audio.call<void>("play");
}

void QWasmAudioOutput::stop()
{
    if (m_audio.isNull() || m_audio.isUndefined()) {
        qCDebug(qWasmMediaAudioOutput) << "audio failed to start";
        emit errorOccured(QMediaPlayer::ResourceError,
                          QStringLiteral("Audio element resource error"));
        return;
    }
    if (!m_source.isEmpty()) {
        pause();
        m_audio.set("currentTime", emscripten::val(0));
    }
    if (m_audioIODevice) {
        m_audioIODevice->close();
        delete m_audioIODevice;
        m_audioIODevice = 0;
    }
}

void QWasmAudioOutput::pause()
{
    if (m_audio.isNull() || m_audio.isUndefined()) {
        qCDebug(qWasmMediaAudioOutput) << "audio failed to start";
        emit errorOccured(QMediaPlayer::ResourceError,
                          QStringLiteral("Audio element resource error"));
        return;
    }
    m_audio.call<emscripten::val>("pause");
}

void QWasmAudioOutput::createAudioElement(const std::string &id)
{
    emscripten::val document = emscripten::val::global("document");
    m_audio = document.call<emscripten::val>("createElement", std::string("audio"));

    // only works in chrome and firefox.
    // Firefox this feature is behind media.setsinkid.enabled preferences
    // allows user to choose audio output device

    if (!m_audio.hasOwnProperty("sinkId") || m_audio["sinkId"].isUndefined()) {
        return;
    }

    std::string usableId = id;
    if (usableId.empty())
        usableId = QMediaDevices::defaultAudioOutput().id();

    qstdweb::PromiseCallbacks sinkIdCallbacks{
        .thenFunc = [](emscripten::val) { qCWarning(qWasmMediaAudioOutput) << "setSinkId ok"; },
        .catchFunc =
                [](emscripten::val) {
                    qCWarning(qWasmMediaAudioOutput) << "Error while trying to setSinkId";
                }
    };
    qstdweb::Promise::make(m_audio, "setSinkId", std::move(sinkIdCallbacks), std::move(usableId));

    m_audio.set("id", usableId.c_str());
}

void QWasmAudioOutput::doElementCallbacks()
{
    // error
    auto errorCallback = [&](emscripten::val event) {
        qCDebug(qWasmMediaAudioOutput) << "error";
        if (event.isUndefined() || event.isNull())
            return;
        emit errorOccured(m_audio["error"]["code"].as<int>(),
                          QString::fromStdString(m_audio["error"]["message"].as<std::string>()));

        QString errorMessage =
                QString::fromStdString(m_audio["error"]["message"].as<std::string>());
        if (errorMessage.isEmpty()) {
            switch (m_audio["error"]["code"].as<int>()) {
            case AudioElementError::MEDIA_ERR_ABORTED:
                errorMessage = QStringLiteral("aborted by the user agent at the user's request.");
                break;
            case AudioElementError::MEDIA_ERR_NETWORK:
                errorMessage = QStringLiteral("network error.");
                break;
            case AudioElementError::MEDIA_ERR_DECODE:
                errorMessage = QStringLiteral("decoding error.");
                break;
            case AudioElementError::MEDIA_ERR_SRC_NOT_SUPPORTED:
                errorMessage = QStringLiteral("src attribute not suitable.");
                break;
            };
        }
        qCDebug(qWasmMediaAudioOutput) << m_audio["error"]["code"].as<int>() << errorMessage;

        emit errorOccured(m_audio["error"]["code"].as<int>(), errorMessage);
    };
    m_errorChangeEvent.reset(new qstdweb::EventCallback(m_audio, "error", errorCallback));

    // loadeddata
    auto loadedDataCallback = [&](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaAudioOutput) << "loaded data";
        qstdweb::window()["URL"].call<emscripten::val>("revokeObjectURL", m_audio["src"]);
    };
    m_loadedDataEvent.reset(new qstdweb::EventCallback(m_audio, "loadeddata", loadedDataCallback));

    // canplay
    auto canPlayCallback = [&](emscripten::val event) {
        if (event.isUndefined() || event.isNull())
            return;
        qCDebug(qWasmMediaAudioOutput) << "can play";
        emit readyChanged(true);
        emit stateChanged(QWasmMediaPlayer::Preparing);
    };
    m_canPlayChangeEvent.reset(new qstdweb::EventCallback(m_audio, "canplay", canPlayCallback));

    // canplaythrough
    auto canPlayThroughCallback = [&](emscripten::val event) {
        Q_UNUSED(event)
        emit stateChanged(QWasmMediaPlayer::Prepared);
    };
    m_canPlayThroughChangeEvent.reset(
            new qstdweb::EventCallback(m_audio, "canplaythrough", canPlayThroughCallback));

    // play
    auto playCallback = [&](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaAudioOutput) << "play";
        emit stateChanged(QWasmMediaPlayer::Started);
    };
    m_playEvent.reset(new qstdweb::EventCallback(m_audio, "play", playCallback));

    // durationchange
    auto durationChangeCallback = [&](emscripten::val event) {
        qCDebug(qWasmMediaAudioOutput) << "durationChange";

        // duration in ms
        emit durationChanged(event["target"]["duration"].as<double>() * 1000);
    };
    m_durationChangeEvent.reset(
            new qstdweb::EventCallback(m_audio, "durationchange", durationChangeCallback));

    // ended
    auto endedCallback = [&](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaAudioOutput) << "ended";
        m_currentMediaStatus = QMediaPlayer::EndOfMedia;
        emit statusChanged(m_currentMediaStatus);
    };
    m_endedEvent.reset(new qstdweb::EventCallback(m_audio, "ended", endedCallback));

    // progress (buffering progress)
    auto progesssCallback = [&](emscripten::val event) {
        if (event.isUndefined() || event.isNull())
            return;
        qCDebug(qWasmMediaAudioOutput) << "progress";
        float duration = event["target"]["duration"].as<int>();
        if (duration < 0) // track not exactly ready yet
            return;

        emscripten::val timeRanges = event["target"]["buffered"];

        if ((!timeRanges.isNull() || !timeRanges.isUndefined())
            && timeRanges["length"].as<int>() == 1) {
            emscripten::val dVal = timeRanges.call<emscripten::val>("end", 0);

            if (!dVal.isNull() || !dVal.isUndefined()) {
                double bufferedEnd = dVal.as<double>();

                if (duration > 0 && bufferedEnd > 0) {
                    float bufferedValue = (bufferedEnd / duration * 100);
                    qCDebug(qWasmMediaAudioOutput) << "progress buffered" << bufferedValue;

                    emit bufferingChanged(m_currentBufferedValue);
                    if (bufferedEnd == duration)
                        m_currentMediaStatus = QMediaPlayer::BufferedMedia;
                    else
                        m_currentMediaStatus = QMediaPlayer::BufferingMedia;

                    emit statusChanged(m_currentMediaStatus);
                }
            }
        }
    };
    m_progressChangeEvent.reset(new qstdweb::EventCallback(m_audio, "progress", progesssCallback));

    // timupdate
    auto timeUpdateCallback = [&](emscripten::val event) {
        qCDebug(qWasmMediaAudioOutput)
                << "timeupdate" << (event["target"]["currentTime"].as<double>() * 1000);

        // qt progress is ms
        emit progressChanged(event["target"]["currentTime"].as<double>() * 1000);
    };
    m_timeUpdateEvent.reset(new qstdweb::EventCallback(m_audio, "timeupdate", timeUpdateCallback));

    // pause
    auto pauseCallback = [&](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaAudioOutput) << "pause";

        int currentTime = m_audio["currentTime"].as<int>(); // in seconds
        int duration = m_audio["duration"].as<int>(); // in seconds
        if ((currentTime > 0 && currentTime < duration)) {
            emit stateChanged(QWasmMediaPlayer::Paused);
        } else {
            emit stateChanged(QWasmMediaPlayer::Stopped);
        }
    };
    m_pauseChangeEvent.reset(new qstdweb::EventCallback(m_audio, "pause", pauseCallback));
}

QT_END_NAMESPACE
