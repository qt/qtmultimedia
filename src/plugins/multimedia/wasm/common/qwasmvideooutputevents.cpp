// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// Event wiring for QWasmVideoOutput: the HTML media element callbacks
// and the iOS orientation-change handling. Split into its own translation
// unit to keep qwasmvideooutput.cpp focused on lifecycle and playback control.

#include "qwasmvideooutput_p.h"

#include <QDebug>
#include <QRect>

#include <emscripten/val.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

void QWasmVideoOutput::doElementCallbacks()
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO;

    // event callbacks
    // timupdate
    auto timeUpdateCallback = [=](emscripten::val event) {
       qCDebug(qWasmMediaVideoOutput) << "timeupdate";

        // qt progress is ms
        emit progressChanged(event["target"]["currentTime"].as<double>() * 1000);
    };
    m_timeUpdateEvent.reset(new QWasmEventHandler(m_video, "timeupdate", timeUpdateCallback));

    // play
    auto playCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "play" << m_video["src"].as<std::string>();
        if (!m_isSeeking)
            emit stateChanged(QWasmMediaPlayer::Preparing);
    };
    m_playEvent.reset(new QWasmEventHandler(m_video, "play", playCallback));

    // ended
    auto endedCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "ended";
        m_currentMediaStatus = MediaStatus::EndOfMedia;
        emit statusChanged(m_currentMediaStatus);
    };
    m_endedEvent.reset(new QWasmEventHandler(m_video, "ended", endedCallback));

    // durationchange
    auto durationChangeCallback = [=](emscripten::val event) {
        qCDebug(qWasmMediaVideoOutput) << "durationChange";

        // qt duration is in milliseconds.
        qint64 dur = event["target"]["duration"].as<double>() * 1000;
        emit durationChanged(dur);
    };
    m_durationChangeEvent.reset(
            new QWasmEventHandler(m_video, "durationchange", durationChangeCallback));

    // loadeddata
    auto loadedDataCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "loaded data";

        emit stateChanged(QWasmMediaPlayer::Prepared);
        if (m_isSeekable != isVideoSeekable()) {
            m_isSeekable = isVideoSeekable();
            emit seekableChanged(m_isSeekable);
        }
    };
    m_loadedDataEvent.reset(new QWasmEventHandler(m_video, "loadeddata", loadedDataCallback));

    // error
    auto errorCallback = [=](emscripten::val event) {
        qCDebug(qWasmMediaVideoOutput) << "error";
        if (event.isUndefined() || event.isNull())
            return;
        emit errorOccured(m_video["error"]["code"].as<int>(),
                          QString::fromStdString(m_video["error"]["message"].as<std::string>()));
    };
    m_errorChangeEvent.reset(new QWasmEventHandler(m_video, "error", errorCallback));

    // resize
    auto resizeCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "resize";

        updateVideoElementGeometry(
                QRect(0, 0, m_video["videoWidth"].as<int>(), m_video["videoHeight"].as<int>()));
        emit sizeChange(m_video["videoWidth"].as<int>(), m_video["videoHeight"].as<int>());

    };
    m_resizeChangeEvent.reset(new QWasmEventHandler(m_video, "resize", resizeCallback));

    // loadedmetadata
    auto loadedMetadataCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "loaded meta data";

        emit metaDataLoaded();
    };
    m_loadedMetadataChangeEvent.reset(
            new QWasmEventHandler(m_video, "loadedmetadata", loadedMetadataCallback));

    // loadstart
    auto loadStartCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "load started";
        m_currentMediaStatus = MediaStatus::LoadingMedia;
        emit statusChanged(m_currentMediaStatus);
        m_isStopped = false;
    };
    m_loadStartChangeEvent.reset(new QWasmEventHandler(m_video, "loadstart", loadStartCallback));

    // canplay

    auto canPlayCallback = [=](emscripten::val event) {
        if (event.isUndefined() || event.isNull())
            return;
        qCDebug(qWasmMediaVideoOutput) << "can play"
                                       << "m_requestedPosition" << m_requestedPosition;

        if (!m_isStopped)
            emit readyChanged(true); // sets video available
    };
    m_canPlayChangeEvent.reset(new QWasmEventHandler(m_video, "canplay", canPlayCallback));

    // canplaythrough
    auto canPlayThroughCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "can play through"
                                       << "m_isStopped" << m_isStopped;

        if (m_currentMediaStatus == MediaStatus::EndOfMedia)
            return;
        bool seekable = isVideoSeekable();
        if (m_isSeekable != seekable) {
            m_isSeekable = seekable;
            emit seekableChanged(m_isSeekable);
        }
        if (!m_isSeeking && !m_isStopped) {
            emscripten::val timeRanges = m_video["buffered"];
            if ((!timeRanges.isNull() || !timeRanges.isUndefined())
                    && timeRanges["length"].as<int>() == 1) {
                double buffered = m_video["buffered"].call<emscripten::val>("end", 0).as<double>();
                const double duration = m_video["duration"].as<double>();

                if (duration == buffered) {
                    m_currentBufferedValue = 100;
                    emit bufferingChanged(m_currentBufferedValue);
                }
            }
            constexpr int hasEnoughData = 4;
            if (m_video["readyState"].as<int>() == hasEnoughData) {
                m_currentMediaStatus = MediaStatus::LoadedMedia;
                emit statusChanged(m_currentMediaStatus);
                m_frameGrabber.startFrameLoop();
            }
        } else {
            m_isStopped = false;
        }
    };
    m_canPlayThroughChangeEvent.reset(
            new QWasmEventHandler(m_video, "canplaythrough", canPlayThroughCallback));

    // seeking
    auto seekingCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput)
                << "seeking started" << (m_video["currentTime"].as<double>() * 1000);
        m_isSeeking = true;
    };
    m_seekingChangeEvent.reset(new QWasmEventHandler(m_video, "seeking", seekingCallback));

    // seeked
    auto seekedCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "seeked" << (m_video["currentTime"].as<double>() * 1000);
        emit progressChanged(m_video["currentTime"].as<double>() * 1000);
        m_isSeeking = false;
    };
    m_seekedChangeEvent.reset(new QWasmEventHandler(m_video, "seeked", seekedCallback));

    // emptied
    auto emptiedCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "emptied";
        emit readyChanged(false);
        m_currentMediaStatus = MediaStatus::EndOfMedia;
        emit statusChanged(m_currentMediaStatus);
    };
    m_emptiedChangeEvent.reset(new QWasmEventHandler(m_video, "emptied", emptiedCallback));

    // stalled
    auto stalledCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "stalled";
        m_currentMediaStatus = MediaStatus::StalledMedia;
        emit statusChanged(m_currentMediaStatus);
    };
    m_stalledChangeEvent.reset(new QWasmEventHandler(m_video, "stalled", stalledCallback));

    // waiting
    auto waitingCallback = [=](emscripten::val event) {
        Q_UNUSED(event)

        qCDebug(qWasmMediaVideoOutput) << "waiting";
        // check buffer
    };
    m_waitingChangeEvent.reset(new QWasmEventHandler(m_video, "waiting", waitingCallback));

    // suspend

    // playing
    auto playingCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "playing";
        if (m_isSeeking)
            return;
        emit stateChanged(QWasmMediaPlayer::Started);
        if (m_toBePaused) { // paused
            m_toBePaused = false;
            m_frameGrabber.startFrameLoop();
        }
    };
    m_playingChangeEvent.reset(new QWasmEventHandler(m_video, "playing", playingCallback));

    // progress (buffering progress)
    auto progesssCallback = [=](emscripten::val event) {
        if (event.isUndefined() || event.isNull())
            return;

        const double duration = event["target"]["duration"].as<double>();
        if (duration < 0) // track not exactly ready yet
            return;

        emscripten::val timeRanges = event["target"]["buffered"];

        if ((!timeRanges.isNull() || !timeRanges.isUndefined())
                && timeRanges["length"].as<int>() == 1) {
            emscripten::val dVal = timeRanges.call<emscripten::val>("end", 0);
            if (!dVal.isNull() || !dVal.isUndefined()) {
                double bufferedEnd = dVal.as<double>();

                if (duration > 0 && bufferedEnd > 0) {
                    const double bufferedValue = (bufferedEnd / duration * 100);
                    qCDebug(qWasmMediaVideoOutput) << "progress buffered";
                    m_currentBufferedValue = bufferedValue;
                    emit bufferingChanged(m_currentBufferedValue);
                    if (bufferedEnd == duration)
                        m_currentMediaStatus = MediaStatus::BufferedMedia;
                    else
                        m_currentMediaStatus = MediaStatus::BufferingMedia;
                    emit statusChanged(m_currentMediaStatus);
                }
            }
        }
    };
    m_progressChangeEvent.reset(new QWasmEventHandler(m_video, "progress", progesssCallback));

    // pause
    auto pauseCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "pause";
        m_toBePaused = true;
        const double currentTime = m_video["currentTime"].as<double>(); // in seconds
        const double duration = m_video["duration"].as<double>(); // in seconds
        if ((currentTime > 0 && currentTime < duration) && (!m_isStopped)) {
            emit stateChanged(QWasmMediaPlayer::Paused);
        } else {
            // stop this crazy thing!
            m_video.set("currentTime", emscripten::val(0));
            emit stateChanged(QWasmMediaPlayer::Stopped);
        }
    };
    m_pauseChangeEvent.reset(new QWasmEventHandler(m_video, "pause", pauseCallback));

    // onunload
    // we use lower level events here as to avert a crash on activate using the
    // qtdweb see _qt_beforeUnload
    emscripten::val window = emscripten::val::global("window");

    auto beforeUnloadCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        // large videos will leave the unloading window
        // in a frozen state, so remove the video element src first
        m_video.call<void>("removeAttribute", emscripten::val("src"));
        m_video.call<void>("load");
    };
    m_beforeUnloadEvent.reset(new QWasmEventHandler(window, "beforeunload", beforeUnloadCallback));
}

// iOS has [broken] camera driver, so rotate the frames ourselves
// based on the device orientation and which camera is facing.
void QWasmVideoOutput::applyIosRotation(int orientationIndex)
{
    if (orientationIndex & EMSCRIPTEN_ORIENTATION_PORTRAIT_PRIMARY) { // 1
        m_rotateBy = QtVideo::Rotation::Clockwise90;
    } else if (orientationIndex & EMSCRIPTEN_ORIENTATION_LANDSCAPE_PRIMARY) { // 4
        if (m_cameraMode == QWasmVideoOutput::Front) {
            m_rotateBy = QtVideo::Rotation::Clockwise180;
        } else {
            m_rotateBy = QtVideo::Rotation::None;
        }
    } else if (orientationIndex & EMSCRIPTEN_ORIENTATION_PORTRAIT_SECONDARY) { // 2
        m_rotateBy = QtVideo::Rotation::Clockwise270;
    } else if (orientationIndex & EMSCRIPTEN_ORIENTATION_LANDSCAPE_SECONDARY) { // 8
        if (m_cameraMode == QWasmVideoOutput::Front) {
            m_rotateBy = QtVideo::Rotation::None;
        } else {
            m_rotateBy = QtVideo::Rotation::Clockwise180;
        }
    }
}

bool QWasmVideoOutput::orientationchangeCallback(int eventType,
                                                 const EmscriptenOrientationChangeEvent *event,
                                                 void *userData)
{
    Q_UNUSED(eventType)

    QWasmVideoOutput *videoOutput = static_cast<QWasmVideoOutput *>(userData);
    emit videoOutput->orientationChanged(event->orientationIndex);

    return true;
}

int QWasmVideoOutput::getCurrentOrientationIndex()
{
    //get current status
    EmscriptenOrientationChangeEvent status;
    EMSCRIPTEN_RESULT result = emscripten_get_orientation_status(&status);
    if (result == EMSCRIPTEN_RESULT_SUCCESS)
        return status.orientationIndex;
    return 0;
}

void QWasmVideoOutput::detectIosCameraRotation()
{
    if (!isPlatformiOs())
        return;

    m_useCameraRotation = true;
    emscripten::val stream = m_video["srcObject"];
    emscripten::val videoTracks = stream.call<emscripten::val>("getVideoTracks");

    if (!videoTracks.isUndefined() && videoTracks["length"].as<int>() > 0) {
        emscripten::val track = videoTracks[0];
        emscripten::val settings = track.call<emscripten::val>("getSettings");

        if (settings["facingMode"].as<std::string>() == "user")
            m_cameraMode = QWasmVideoOutput::Front;
        else
            m_cameraMode = QWasmVideoOutput::Back;
        // now we know camera, set m_rotateBy
        orientationChanged(getCurrentOrientationIndex());
    }
}

QT_END_NAMESPACE
