// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>
#include <QUrl>
#include <QPoint>
#include <QRect>
#include <QMediaPlayer>
#include <QVideoFrame>
#include "qwasmvideooutput_p.h"

#include <qvideosink.h>
#include <private/qabstractvideobuffer_p.h>
#include <private/qplatformvideosink_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>

#include <emscripten/bind.h>
#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qWasmMediaVideoOutput, "qt.multimedia.wasm.videooutput")

// TODO unique videosurface ?
static std::string m_videoSurfaceId;

void qtVideoBeforeUnload(emscripten::val event)
{
    Q_UNUSED(event)
    // large videos will leave the unloading window
    // in a frozen state, so remove the video element first
    emscripten::val document = emscripten::val::global("document");
    emscripten::val videoElement =
            document.call<emscripten::val>("getElementById", std::string(m_videoSurfaceId));
    videoElement.call<void>("removeAttribute", emscripten::val("src"));
    videoElement.call<void>("load");
}

EMSCRIPTEN_BINDINGS(video_module)
{
    emscripten::function("mbeforeUnload", qtVideoBeforeUnload);
}

QWasmVideoOutput::QWasmVideoOutput(QObject *parent) : QObject{ parent } { }

void QWasmVideoOutput::setVideoSize(const QSize &newSize)
{
    if (m_pendingVideoSize == newSize)
        return;

    m_pendingVideoSize = newSize;
    updateVideoElementGeometry(QRect(0, 0, m_pendingVideoSize.width(), m_pendingVideoSize.height()));
}

void QWasmVideoOutput::setVideoMode(QWasmVideoOutput::WasmVideoMode mode)
{
    m_currentVideoMode = mode;
}

void QWasmVideoOutput::start()
{
    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }

    switch (m_currentVideoMode) {
    case QWasmVideoOutput::VideoOutput: {
        emscripten::val sourceObj =
                m_video.call<emscripten::val>("getAttribute", emscripten::val("src"));

        if ((sourceObj.isUndefined() || sourceObj.isNull()) && !m_source.isEmpty()) {
            qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << "calling load" << m_source;
            m_video.call<void>("setAttribute", emscripten::val("src"), m_source.toStdString());
            m_video.call<void>("load");
        }
    } break;
    case QWasmVideoOutput::Camera: {
        emscripten::val stream = m_video["srcObject"];
        if (stream.isNull() || stream.isUndefined()) { // camera  device
            qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << "ERROR";
            emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        } else {
            emscripten::val videoTracks = stream.call<emscripten::val>("getVideoTracks");
            if (videoTracks.isNull() || videoTracks.isUndefined()) {
                qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << "videoTracks is null";
                emit errorOccured(QMediaPlayer::ResourceError,
                                  QStringLiteral("video surface error"));
                return;
            }
            if (videoTracks["length"].as<int>() == 0) {
                qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << "videoTracks count is 0";
                emit errorOccured(QMediaPlayer::ResourceError,
                                  QStringLiteral("video surface error"));
                return;
            }
            emscripten::val videoSettings = videoTracks[0].call<emscripten::val>("getSettings");
            if (!videoSettings.isNull() || !videoSettings.isUndefined()) {
                // double fRate = videoSettings["frameRate"].as<double>(); TODO
                const int width = videoSettings["width"].as<int>();
                const int height = videoSettings["height"].as<int>();

                qCDebug(qWasmMediaVideoOutput)
                     << "width" << width << "height" << height;

                updateVideoElementGeometry(QRect(0, 0, width, height));
            }
        }
    } break;
    };

    m_shouldStop = false;
    m_toBePaused = false;
    m_video.call<void>("play");

    if (m_currentVideoMode == QWasmVideoOutput::Camera)
        videoFrameTimerCallback();
}

void QWasmVideoOutput::stop()
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO;

    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
        return;
    }
    m_shouldStop = true;
    if (m_toBePaused) {
        // we are stopped , need to reset
        m_toBePaused = false;
        m_video.call<void>("load");
    } else {
        m_video.call<void>("pause");
    }
}

void QWasmVideoOutput::pause()
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO;

    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }
    m_shouldStop = false;
    m_toBePaused = true;
    m_video.call<void>("pause");
}

void QWasmVideoOutput::reset()
{
    // flush pending frame
    if (m_wasmSink)
        m_wasmSink->platformVideoSink()->setVideoFrame(QVideoFrame());

    m_source = "";
    m_video.set("currentTime", emscripten::val(0));
    m_video.call<void>("load");
}

emscripten::val QWasmVideoOutput::surfaceElement()
{
    return m_video;
}

void QWasmVideoOutput::setSurface(QVideoSink *surface)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << surface << m_wasmSink;
    if (surface == m_wasmSink)
        return;

    m_wasmSink = surface;
}

bool QWasmVideoOutput::isReady() const
{
    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        return false;
    }

    constexpr int hasCurrentData = 2;
    if (!m_video.isUndefined() || !m_video.isNull())
        return m_video["readyState"].as<int>() >= hasCurrentData;
    else
        return true;
}

void QWasmVideoOutput::setSource(const QUrl &url)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << url;

    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }

    if (url.isEmpty()) {
        stop();
        return;
    }
    if (url.isLocalFile()) {
        qDebug() << "no local files allowed, so we need to blob";
        //        QFile mFile(url.toString());
        //        if (!mFile.open(QIODevice::readOnly)) {// some error
        //            return;
        //        QDataStream dStream(&mFile); // ?
        //        setSource(dStream);
        return;
    }

    // is network path
    m_source = url.toString();

    addSourceElement(m_source);
}

void QWasmVideoOutput::addSourceElement(const QString &urlString)
{
    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }
    emscripten::val document = emscripten::val::global("document");

    if (!urlString.isEmpty())
        m_video.set("src", m_source.toStdString());

    if (!urlString.isEmpty())
        m_video.call<void>("load");
}

void QWasmVideoOutput::addCameraSourceElement(const std::string &id)
{
    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qWarning() << "No media devices found";
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
        return;
    }

    qstdweb::PromiseCallbacks getUserMediaCallback{
        .thenFunc =
                [this](emscripten::val stream) {
            qCDebug(qWasmMediaVideoOutput) << "getUserMediaSuccess";

            m_video.set("srcObject", stream);

            emscripten::val videoTracksObject = stream.call<emscripten::val>("getVideoTracks");
            if (videoTracksObject.isNull() || videoTracksObject.isUndefined()) {
                emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
                return;
            }

            if (videoTracksObject.call<emscripten::val>("length").as<int>() == 0)  {
                emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
                return;
            }
            emscripten::val tracks = stream.call<emscripten::val>("getVideoTracks");
            if (tracks.isNull() || tracks.isUndefined())
                return;
            if (tracks["length"].as<int>() == 0)
                return;
            emscripten::val videoTrack = tracks[0];
            if (videoTrack.isNull() || videoTrack.isUndefined()) {
                emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
                return;
            }

            emscripten::val currentVideoCapabilities = videoTrack.call<emscripten::val>("getCapabilities");
            if (currentVideoCapabilities.isNull() || currentVideoCapabilities.isUndefined()) {
                emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
                return;
            }

            emscripten::val videoSettings = videoTrack.call<emscripten::val>("getSettings");
            if (videoSettings.isNull() || videoSettings.isUndefined()) {
                emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
                return;
            }

            // TODO double fRate = videoSettings["frameRate"].as<double>();
            // const int width = videoSettings["width"].as<int>();
            // const int height = videoSettings["height"].as<int>();

        },
        .catchFunc =
                [](emscripten::val error) {
            qCDebug(qWasmMediaVideoOutput)
                    << "getUserMedia fail"
                    << QString::fromStdString(error["name"].as<std::string>())
                    << QString::fromStdString(error["message"].as<std::string>());
        }
    };

    emscripten::val constraints = emscripten::val::object();

    constraints.set("audio", m_hasAudio);

    emscripten::val videoContraints = emscripten::val::object();
    videoContraints.set("exact", id);
    videoContraints.set("deviceId", id);
    constraints.set("video", videoContraints);

    // we do it this way as this prompts user for mic/camera permissions
    qstdweb::Promise::make(mediaDevices, QStringLiteral("getUserMedia"),
                           std::move(getUserMediaCallback), constraints);
}

void QWasmVideoOutput::setSource(QIODevice *stream)
{
    Q_UNUSED(stream)

    if (m_video.isUndefined() || m_video.isNull() || m_videoElementSource.isUndefined()
        || m_videoElementSource.isNull()) {
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }

    // src (depreciated) or srcObject
    // MediaStream, MediaSource Blob or File

    // TODO QIOStream to
    // SourceBuffer.appendBuffer() // ArrayBuffer
    // SourceBuffer.appendStream() // WriteableStream ReadableStream (to sink)
    // SourceBuffer.appendBufferAsync()
    //    emscripten::val document = emscripten::val::global("document");

    // stream to blob

    // Create/add video source
    m_video.call<void>("setAttribute", emscripten::val("srcObject"), m_source.toStdString());
    m_video.call<void>("load");
}

void QWasmVideoOutput::setVolume(qreal volume)
{ // between 0 - 1
    volume = qBound(qreal(0.0), volume, qreal(1.0));
    m_video.set("volume", volume);
}

void QWasmVideoOutput::setMuted(bool muted)
{
    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }
    m_video.set("muted", muted);
}

qint64 QWasmVideoOutput::getCurrentPosition()
{
    return (!m_video.isUndefined() || !m_video.isNull())
            ? (m_video["currentTime"].as<double>() * 1000)
            : 0;
}

void QWasmVideoOutput::seekTo(qint64 positionMSecs)
{
    if (isVideoSeekable()) {
        float positionToSetInSeconds = float(positionMSecs) / 1000;
        emscripten::val seekableTimeRange = m_video["seekable"];
        if (!seekableTimeRange.isNull() || !seekableTimeRange.isUndefined()) {
            // range user can seek
            if (seekableTimeRange["length"].as<int>() < 1)
                return;
            if (positionToSetInSeconds
                        >= seekableTimeRange.call<emscripten::val>("start", 0).as<int>()
                && positionToSetInSeconds
                        <= seekableTimeRange.call<emscripten::val>("end", 0).as<int>()) {
                m_requestedPosition = positionToSetInSeconds;

                m_video.set("currentTime", m_requestedPosition);
            }
        }
    }
    qCDebug(qWasmMediaVideoOutput) << "m_requestedPosition" << m_requestedPosition;
}

bool QWasmVideoOutput::isVideoSeekable()
{
    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return false;
    }

    emscripten::val seekableTimeRange = m_video["seekable"];
    if (seekableTimeRange["length"].as<int>() < 1)
        return false;
    if (!seekableTimeRange.isNull() || !seekableTimeRange.isUndefined()) {
        bool isit = !qFuzzyCompare(seekableTimeRange.call<emscripten::val>("start", 0).as<float>(),
                                   seekableTimeRange.call<emscripten::val>("end", 0).as<float>());
        return isit;
    }
    return false;
}

void QWasmVideoOutput::createVideoElement(const std::string &id)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << id;
    // Create <video> element and add it to the page body
    emscripten::val document = emscripten::val::global("document");
    emscripten::val body = document["body"];

    emscripten::val oldVideo = document.call<emscripten::val>("getElementsByClassName",
                                                              (m_currentVideoMode == QWasmVideoOutput::Camera
                                                               ? std::string("Camera")
                                                               : std::string("Video")));

    // we don't provide alternate tracks
    // but need to remove stale track
    if (oldVideo["length"].as<int>() > 0)
        oldVideo[0].call<void>("remove");

    m_videoSurfaceId = id;
    m_video = document.call<emscripten::val>("createElement", std::string("video"));

    m_video.set("id", m_videoSurfaceId.c_str());
    m_video.call<void>("setAttribute", std::string("class"),
                       (m_currentVideoMode == QWasmVideoOutput::Camera ? std::string("Camera")
                                                                       : std::string("Video")));

    // if video
    m_video.set("preload", "metadata");

    // Uncaught DOMException: Failed to execute 'getImageData' on
    // 'OffscreenCanvasRenderingContext2D': The canvas has been tainted by
    // cross-origin data.
    // TODO figure out somehow to let user choose between these
    std::string originString = "anonymous"; // requires server Access-Control-Allow-Origin *
    //    std::string originString = "use-credentials"; // must not
    //    Access-Control-Allow-Origin *

    m_video.call<void>("setAttribute", std::string("crossorigin"), originString);
    body.call<void>("appendChild", m_video);

    // Create/add video source
    emscripten::val videoElementGeometry =
            document.call<emscripten::val>("createElement", std::string("source"));

    videoElementGeometry.set("src", m_source.toStdString());

    // Set position:absolute, which makes it possible to position the video
    // element using x,y. coordinates, relative to its parent (the page's <body>
    // element)
    emscripten::val style = m_video["style"];
    style.set("position", "absolute");
    style.set("display", "none"); // hide
}

void QWasmVideoOutput::createOffscreenElement(const QSize &offscreenSize)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO;

    // create offscreen element for grabbing frames
    // OffscreenCanvas - no safari :(
    // https://developer.mozilla.org/en-US/docs/Web/API/OffscreenCanvas

    emscripten::val document = emscripten::val::global("document");

    // TODO use correct framesize?
    // offscreen render buffer
    m_offscreen = emscripten::val::global("OffscreenCanvas");

    if (m_offscreen.isUndefined()) {
        // Safari OffscreenCanvas not supported, try old skool way
        m_offscreen = document.call<emscripten::val>("createElement", std::string("canvas"));

        m_offscreen.set("style",
                      "position:absolute;left:-1000px;top:-1000px"); // offscreen
        m_offscreen.set("width", offscreenSize.width());
        m_offscreen.set("height", offscreenSize.height());
        m_offscreenContext = m_offscreen.call<emscripten::val>("getContext", std::string("2d"));
    } else {
        m_offscreen = emscripten::val::global("OffscreenCanvas")
                            .new_(offscreenSize.width(), offscreenSize.height());
        emscripten::val offscreenAttributes = emscripten::val::array();
        offscreenAttributes.set("willReadFrequently", true);
        m_offscreenContext = m_offscreen.call<emscripten::val>("getContext", std::string("2d"),
                                                             offscreenAttributes);
    }
    std::string offscreenId = m_videoSurfaceId + "_offscreenOutputSurface";
    m_offscreen.set("id", offscreenId.c_str());
}

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
    m_timeUpdateEvent.reset(new qstdweb::EventCallback(m_video, "timeupdate", timeUpdateCallback));

    // play
    auto playCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "play";
        if (!m_isSeeking)
            emit stateChanged(QWasmMediaPlayer::Preparing);
    };
    m_playEvent.reset(new qstdweb::EventCallback(m_video, "play", playCallback));

    // ended
    auto endedCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "ended";
        m_currentMediaStatus = QMediaPlayer::EndOfMedia;
        emit statusChanged(m_currentMediaStatus);
        m_shouldStop = true;
        stop();
    };
    m_endedEvent.reset(new qstdweb::EventCallback(m_video, "ended", endedCallback));

    // durationchange
    auto durationChangeCallback = [=](emscripten::val event) {
        qCDebug(qWasmMediaVideoOutput) << "durationChange";

        // qt duration is in milliseconds.
        qint64 dur = event["target"]["duration"].as<double>() * 1000;
        emit durationChanged(dur);
    };
    m_durationChangeEvent.reset(
            new qstdweb::EventCallback(m_video, "durationchange", durationChangeCallback));

    // loadeddata
    auto loadedDataCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "loaded data";

        emit stateChanged(QWasmMediaPlayer::Prepared);
    };
    m_loadedDataEvent.reset(new qstdweb::EventCallback(m_video, "loadeddata", loadedDataCallback));

    // error
    auto errorCallback = [=](emscripten::val event) {
        qCDebug(qWasmMediaVideoOutput) << "error";
        if (event.isUndefined() || event.isNull())
            return;
        emit errorOccured(m_video["error"]["code"].as<int>(),
                          QString::fromStdString(m_video["error"]["message"].as<std::string>()));
    };
    m_errorChangeEvent.reset(new qstdweb::EventCallback(m_video, "error", errorCallback));

    // resize
    auto resizeCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "resize";

        updateVideoElementGeometry(
                QRect(0, 0, m_video["videoWidth"].as<int>(), m_video["videoHeight"].as<int>()));
        emit sizeChange(m_video["videoWidth"].as<int>(), m_video["videoHeight"].as<int>());

    };
    m_resizeChangeEvent.reset(new qstdweb::EventCallback(m_video, "resize", resizeCallback));

    // loadedmetadata
    auto loadedMetadataCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "loaded meta data";

        emit metaDataLoaded();
    };
    m_loadedMetadataChangeEvent.reset(
            new qstdweb::EventCallback(m_video, "loadedmetadata", loadedMetadataCallback));

    // loadstart
    auto loadStartCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "load started";
        m_currentMediaStatus = QMediaPlayer::LoadingMedia;
        emit statusChanged(m_currentMediaStatus);
        m_shouldStop = false;
    };
    m_loadStartChangeEvent.reset(new qstdweb::EventCallback(m_video, "loadstart", loadStartCallback));

    // canplay

    auto canPlayCallback = [=](emscripten::val event) {
        if (event.isUndefined() || event.isNull())
            return;
        qCDebug(qWasmMediaVideoOutput) << "can play"
                                       << "m_requestedPosition" << m_requestedPosition;

        if (!m_shouldStop)
            emit readyChanged(true); // sets video available
    };
    m_canPlayChangeEvent.reset(new qstdweb::EventCallback(m_video, "canplay", canPlayCallback));

    // canplaythrough
    auto canPlayThroughCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "can play through"
                                       << "m_shouldStop" << m_shouldStop;

        if (m_currentMediaStatus == QMediaPlayer::EndOfMedia)
            return;
        if (!m_isSeeking && !m_shouldStop) {
            emscripten::val timeRanges = m_video["buffered"];
            if ((!timeRanges.isNull() || !timeRanges.isUndefined())
                    && timeRanges["length"].as<int>() == 1) {
                double buffered = m_video["buffered"].call<emscripten::val>("end", 0).as<double>();
                float duration = m_video["duration"].as<float>();

                if (duration == buffered) {
                    m_currentBufferedValue = 100;
                    emit bufferingChanged(m_currentBufferedValue);
                }
            }
            m_currentMediaStatus = QMediaPlayer::LoadedMedia;
            emit statusChanged(m_currentMediaStatus);
            videoFrameTimerCallback();
        } else {
            m_shouldStop = false;
        }
    };
    m_canPlayThroughChangeEvent.reset(
            new qstdweb::EventCallback(m_video, "canplaythrough", canPlayThroughCallback));

    // seeking
    auto seekingCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput)
                << "seeking started" << (m_video["currentTime"].as<double>() * 1000);
        m_isSeeking = true;
    };
    m_seekingChangeEvent.reset(new qstdweb::EventCallback(m_video, "seeking", seekingCallback));

    // seeked
    auto seekedCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "seeked" << (m_video["currentTime"].as<double>() * 1000);
        emit progressChanged(m_video["currentTime"].as<double>() * 1000);
        m_isSeeking = false;
    };
    m_seekedChangeEvent.reset(new qstdweb::EventCallback(m_video, "seeked", seekedCallback));

    // emptied
    auto emptiedCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        checkNetworkState();
        qCDebug(qWasmMediaVideoOutput) << "emptied";
        emit readyChanged(false);
        m_currentMediaStatus = QMediaPlayer::EndOfMedia;
        emit statusChanged(m_currentMediaStatus);
    };
    m_emptiedChangeEvent.reset(new qstdweb::EventCallback(m_video, "emptied", emptiedCallback));

    // stalled
    auto stalledCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "stalled";
        m_currentMediaStatus = QMediaPlayer::StalledMedia;
        emit statusChanged(m_currentMediaStatus);
    };
    m_stalledChangeEvent.reset(new qstdweb::EventCallback(m_video, "waiting", stalledCallback));

    // waiting
    auto waitingCallback = [=](emscripten::val event) {
        Q_UNUSED(event)

        qCDebug(qWasmMediaVideoOutput) << "waiting";
        // check buffer
    };
    m_waitingChangeEvent.reset(new qstdweb::EventCallback(m_video, "waiting", waitingCallback));

    // suspend

    // playing
    auto playingCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "playing";
        if (m_isSeeking)
            return;
        emit stateChanged(QWasmMediaPlayer::Started);
        if (m_toBePaused || !m_shouldStop) { // paused
            m_toBePaused = false;

            videoFrameTimerCallback(); // get the ball rolling
        }
    };
    m_playingChangeEvent.reset(new qstdweb::EventCallback(m_video, "playing", playingCallback));

    // progress (buffering progress)
    auto progesssCallback = [=](emscripten::val event) {
        if (event.isUndefined() || event.isNull())
            return;

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
                    qCDebug(qWasmMediaVideoOutput) << "progress buffered";
                    m_currentBufferedValue = bufferedValue;
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
    m_progressChangeEvent.reset(new qstdweb::EventCallback(m_video, "progress", progesssCallback));

    // pause
    auto pauseCallback = [=](emscripten::val event) {
        Q_UNUSED(event)
        qCDebug(qWasmMediaVideoOutput) << "pause";

        int currentTime = m_video["currentTime"].as<int>(); // in seconds
        int duration = m_video["duration"].as<int>(); // in seconds
        if ((currentTime > 0 && currentTime < duration) && (!m_shouldStop && m_toBePaused)) {
            emit stateChanged(QWasmMediaPlayer::Paused);
        } else {
            // stop this crazy thing!
            m_video.set("currentTime", emscripten::val(0));
            emit stateChanged(QWasmMediaPlayer::Stopped);
        }
    };
    m_pauseChangeEvent.reset(new qstdweb::EventCallback(m_video, "pause", pauseCallback));

    // onunload
    // we use lower level events here as to avert a crash on activate using the
    // qtdweb see _qt_beforeUnload
    emscripten::val window = emscripten::val::global("window");
    window.call<void>("addEventListener", std::string("beforeunload"),
                      emscripten::val::module_property("mbeforeUnload"));
}

void QWasmVideoOutput::updateVideoElementGeometry(const QRect &windowGeometry)
{
    QRect m_videoElementSource(windowGeometry.topLeft(), windowGeometry.size());

    emscripten::val style = m_video["style"];
    style.set("left", QString("%1px").arg(m_videoElementSource.left()).toStdString());
    style.set("top", QString("%1px").arg(m_videoElementSource.top()).toStdString());
    style.set("width", QString("%1px").arg(m_videoElementSource.width()).toStdString());
    style.set("height", QString("%1px").arg(m_videoElementSource.height()).toStdString());
    style.set("z-index", "999");

    // offscreen
    m_offscreen.set("width", m_videoElementSource.width());
    m_offscreen.set("height", m_videoElementSource.height());
}

qint64 QWasmVideoOutput::getDuration()
{
    // qt duration is in ms
    // js is sec

    if (m_video.isUndefined() || m_video.isNull())
        return 0;
    return m_video["duration"].as<int>() * 1000;
}

void QWasmVideoOutput::newFrame(const QVideoFrame &frame)
{
    m_wasmSink->setVideoFrame(frame);
}

void QWasmVideoOutput::setPlaybackRate(qreal rate)
{
    m_video.set("playbackRate", emscripten::val(rate));
}

qreal QWasmVideoOutput::playbackRate()
{
    return (m_video.isUndefined() || m_video.isNull()) ? 0 : m_video["playbackRate"].as<float>();
}

void QWasmVideoOutput::checkNetworkState()
{
    int netState = m_video["networkState"].as<int>();

    qCDebug(qWasmMediaVideoOutput) << netState;

    switch (netState) {
    case QWasmMediaPlayer::QWasmMediaNetworkState::NetworkEmpty: // no data
        break;
    case QWasmMediaPlayer::QWasmMediaNetworkState::NetworkIdle:
        break;
    case QWasmMediaPlayer::QWasmMediaNetworkState::NetworkLoading:
        break;
    case QWasmMediaPlayer::QWasmMediaNetworkState::NetworkNoSource: // no source
        emit errorOccured(netState, QStringLiteral("No media source found"));
        break;
    };
}

void QWasmVideoOutput::videoComputeFrame(void *context)
{
    if (m_offscreenContext.isUndefined() || m_offscreenContext.isNull()) {
        qCDebug(qWasmMediaVideoOutput) << "canvas context could not be found";
        return;
    }
    emscripten::val document = emscripten::val::global("document");

    emscripten::val videoElement =
            document.call<emscripten::val>("getElementById", std::string(m_videoSurfaceId));

    if (videoElement.isUndefined() || videoElement.isNull()) {
        qCDebug(qWasmMediaVideoOutput) << "video element could not be found";
        return;
    }

    const int videoWidth = videoElement["videoWidth"].as<int>();
    const int videoHeight = videoElement["videoHeight"].as<int>();

    if (videoWidth == 0 || videoHeight == 0)
        return;

    m_offscreenContext.call<void>("drawImage", videoElement, 0, 0, videoWidth, videoHeight);

    emscripten::val frame = // one frame, Uint8ClampedArray
            m_offscreenContext.call<emscripten::val>("getImageData", 0, 0, videoWidth, videoHeight);

    const QSize frameSize(videoWidth, videoHeight);

    // this seems to work ok, even though getImageData returns a Uint8ClampedArray
    QByteArray frameBytes = qstdweb::Uint8Array(frame["data"]).copyToQByteArray();

    QVideoFrameFormat frameFormat =
            QVideoFrameFormat(frameSize, QVideoFrameFormat::Format_RGBA8888);

    auto *textureDescription = QVideoTextureHelper::textureDescription(frameFormat.pixelFormat());

    QVideoFrame vFrame(
            new QMemoryVideoBuffer(frameBytes,
                                   textureDescription->strideForWidth(frameFormat.frameWidth())),
            frameFormat);
    QWasmVideoOutput *wasmVideoOutput = reinterpret_cast<QWasmVideoOutput *>(context);

    if (!wasmVideoOutput->m_wasmSink) {
        qWarning() << "ERROR ALERT!! video sink not set";
    }
    wasmVideoOutput->m_wasmSink->setVideoFrame(vFrame);
}

void QWasmVideoOutput::videoFrameTimerCallback()
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << m_videoSurfaceId;
    static auto frame = [](double frameTime, void *context) -> int {
        Q_UNUSED(frameTime);
        QWasmVideoOutput *videoOutput = reinterpret_cast<QWasmVideoOutput *>(context);

        emscripten::val document = emscripten::val::global("document");
        emscripten::val videoElement =
                document.call<emscripten::val>("getElementById", std::string(m_videoSurfaceId));

        if (videoElement["paused"].as<bool>() || videoElement["ended"].as<bool>())
            return false;

        videoOutput->videoComputeFrame(context);

        return true;
    };

    emscripten_request_animation_frame_loop(frame, this);
    // about 60 fps
}

emscripten::val QWasmVideoOutput::getDeviceCapabilities()
{
    emscripten::val stream = m_video["srcObject"];
    if (!stream.isUndefined() || !stream["getVideoTracks"].isUndefined()) {
        emscripten::val tracks = stream.call<emscripten::val>("getVideoTracks");
        if (!tracks.isUndefined()) {
            if (tracks["length"].as<int>() == 0)
                return emscripten::val::undefined();

            emscripten::val track = tracks[0];
            if (!track.isUndefined()) {
                emscripten::val trackCaps = emscripten::val::undefined();
                if (!track["getCapabilities"].isUndefined())
                    trackCaps = track.call<emscripten::val>("getCapabilities");
                else // firefox does not support getCapabilities
                    trackCaps = track.call<emscripten::val>("getSettings");

                if (!trackCaps.isUndefined())
                    return trackCaps;
            }
        }
    } else {
        // camera not started track capabilities not available
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("capabilities not available"));
    }

    return emscripten::val::undefined();
}

bool QWasmVideoOutput::setDeviceSetting(const std::string &key, emscripten::val value)
{
    emscripten::val stream = m_video["srcObject"];
    if (stream.isNull() || stream.isUndefined()
            || stream["getVideoTracks"].isUndefined())
        return false;

    emscripten::val tracks = stream.call<emscripten::val>("getVideoTracks");
    if (!tracks.isNull() || !tracks.isUndefined()) {
        if (tracks["length"].as<int>() == 0)
            return false;

        emscripten::val track = tracks[0];
        emscripten::val contraint = emscripten::val::object();
        contraint.set(std::move(key), value);
        track.call<emscripten::val>("applyConstraints", contraint);
        return true;
    }

    return false;
}

QT_END_NAMESPACE

#include "moc_qwasmvideooutput_p.cpp"
