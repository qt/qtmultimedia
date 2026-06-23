// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDebug>
#include <QUrl>
#include <QPoint>
#include <QRect>
#include <QMediaPlayer>
#include <QVideoFrame>
#include <QFile>
#include <QBuffer>
#include <QMimeDatabase>
#include <QGuiApplication>
#include <QOpenGLContext>

#include <QtGui/rhi/qrhi_platform.h>
#include <qpa/qplatformwindow_p.h>

#include <GLES2/gl2.h>

#include "qwasmvideooutput_p.h"

#include <qvideosink.h>
#include <private/qplatformvideosink_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <private/qvideoframe_p.h>
#include <private/qstdweb_p.h>
#include <QTimer>

#include <emscripten/bind.h>
#include <emscripten/val.h>

// Upload the current video frame to the already-bound TEXTURE_2D.
// The canvas is passed as an EM_VAL handle; Emval.toValue() here refers to
// Emscripten's internal Emval object, not Module.Emval — no EXPORTED_RUNTIME_METHODS entry needed.
EM_JS(void, em_texImage2DFromVideo, (const char *videoId, int *pW, int *pH), {
    var gl = GL.currentContext.GLctx;
    var video = document.getElementById(UTF8ToString(videoId));
    if (!video) { return; }
    var frame;
    try { frame = new VideoFrame(video); } catch(e) { return; }
    HEAP32[pW >> 2] = frame.displayWidth;
    HEAP32[pH >> 2] = frame.displayHeight;
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, frame);
    frame.close();
});

QT_BEGIN_NAMESPACE


using namespace emscripten;
using namespace Qt::Literals;

Q_LOGGING_CATEGORY(qWasmMediaVideoOutput, "qt.multimedia.wasm.videooutput")

static bool checkForVideoFrame()
{
    emscripten::val videoFrame = emscripten::val::global("VideoFrame");
    return (!videoFrame.isNull() && !videoFrame.isUndefined());
}

bool QWasmVideoOutput::isPlatformiOs()
{
    emscripten::val platformObject = emscripten::val::global("navigator")["platform"];
    if (platformObject.call<bool>("includes", emscripten::val("iPhone"))
        || platformObject.call<bool>("includes", emscripten::val("iPad")))
        return true;
    return false;
}

QWasmVideoOutput::QWasmVideoOutput(QObject *parent) : QObject{ parent }
{
    m_hasVideoFrame = checkForVideoFrame();

    if (m_hasVideoFrame) {
        if (isPlatformiOs()) {
            // iOS has [broken] camera driver
            connect(this, &QWasmVideoOutput::orientationChanged, this,
                    [&](int orientationIndex) {

                if (orientationIndex & EMSCRIPTEN_ORIENTATION_PORTRAIT_PRIMARY) {// 1
                    m_rotateBy = QtVideo::Rotation::Clockwise90;
                } else if (orientationIndex & EMSCRIPTEN_ORIENTATION_LANDSCAPE_PRIMARY) {// 4
                    if (m_cameraMode == QWasmVideoOutput::Front) {
                        m_rotateBy = QtVideo::Rotation::Clockwise180;
                    } else {
                        m_rotateBy = QtVideo::Rotation::None;
                    }
                } else if (orientationIndex & EMSCRIPTEN_ORIENTATION_PORTRAIT_SECONDARY) {// 2
                    m_rotateBy = QtVideo::Rotation::Clockwise270;
                } else if (orientationIndex & EMSCRIPTEN_ORIENTATION_LANDSCAPE_SECONDARY) {// 8
                    if (m_cameraMode == QWasmVideoOutput::Front) {
                        m_rotateBy = QtVideo::Rotation::None;
                    } else {
                        m_rotateBy = QtVideo::Rotation::Clockwise180;
                    }
                }
            });

            emscripten_set_orientationchange_callback(this,false, &QWasmVideoOutput::orientationchangeCallback);
        }
    }
}

QWasmVideoOutput::~QWasmVideoOutput()
{
    if (m_mediaInputStream)
        JsMediaInputStream::releaseInstance(m_cameraId);
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

void QWasmVideoOutput::setVideoSize(const QSize &newSize)
{
    if (m_pendingVideoSize == newSize)
        return;

    m_pendingVideoSize = newSize;
    updateVideoElementGeometry(QRect(0, 0, m_pendingVideoSize.width(), m_pendingVideoSize.height()));
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

void QWasmVideoOutput::setVideoMode(QWasmVideoOutput::WasmVideoMode mode)
{
    m_currentVideoMode = mode;
}

void QWasmVideoOutput::start()
{
    if (m_video.isUndefined() || m_video.isNull()
        || !m_wasmSink) {
        // error
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }

    switch (m_currentVideoMode) {
    case QWasmVideoOutput::VideoDisplay: {
        emscripten::val sourceObj = m_video["src"];
        if ((sourceObj.isUndefined() || sourceObj.isNull()) && !m_source.isEmpty()) {
            m_video.set("src", m_source);
        }
        if (!isReady())
            m_video.call<void>("load");
    } break;
    case QWasmVideoOutput::SurfaceCapture: {
        m_video.call<void>("play");
        emit readyChanged(true);
    } break;
    case QWasmVideoOutput::Camera: {
        {
            emscripten::val document = emscripten::val::global("document");
            if (m_video["parentNode"].isNull() || m_video["parentNode"].isUndefined())
                document["body"].call<void>("appendChild", m_video);
        }
        if (!m_cameraIsReady) {
            m_shouldBeStarted = true;
        }

       if (!m_connection)
            m_connection = connect(m_mediaInputStream, &JsMediaInputStream::mediaVideoStreamReady, this,
                [=]( ) {
                    m_video.set("srcObject", m_mediaInputStream->getMediaStream());

                    emscripten::val stream = m_video["srcObject"];
                    if (stream.isNull() || stream.isUndefined()) { // camera  device
                        qCDebug(qWasmMediaVideoOutput) << "srcObject ERROR";
                        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
                        return;
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
                        if (!videoSettings.isNull() && !videoSettings.isUndefined()) {
                            const int width = videoSettings["width"].as<int>();
                            const int height = videoSettings["height"].as<int>();
                            updateVideoElementGeometry(QRect(0, 0, width, height));
                            if (!videoSettings["frameRate"].isUndefined())
                                m_streamFrameRate = videoSettings["frameRate"].as<double>();
                        }
                    }

                    m_video.call<void>("play");

                    emit readyChanged(true);

                });
        m_mediaInputStream->setUseAudio(false);
        m_shouldBeStarted = true;
        m_mediaInputStream->setVideoConstraints(m_videoResolution, m_minFrameRate, m_maxFrameRate);
        m_mediaInputStream->setStreamDevice(m_cameraId);

    } break;
    };

    m_isStopped = false;

    if (m_currentVideoMode != QWasmVideoOutput::Camera
        && m_currentVideoMode != QWasmVideoOutput::SurfaceCapture) {
        m_video.call<void>("play");
    }
}

void QWasmVideoOutput::stop()
{
    if (m_isStopped)
        return;
    qCWarning(qWasmMediaVideoOutput) << Q_FUNC_INFO << "mode=" << m_currentVideoMode;
    if (m_video.isUndefined() || m_video.isNull()) {
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("Resource error"));
        return;
    }
    m_isStopped = true;
    if (!m_toBePaused) {
        if (m_currentVideoMode == QWasmVideoOutput::SurfaceCapture) {
            emscripten::val stream = m_video["srcObject"];
            if (!stream.isNull() && !stream.isUndefined()) {
                emscripten::val tracks = stream.call<emscripten::val>("getTracks");
                const int count = tracks["length"].as<int>();
                for (int i = 0; i < count; ++i)
                    tracks[i].call<void>("stop");
            }
        } else if (m_mediaInputStream && m_mediaInputStream->isActive()) {
            m_mediaInputStream->stopMediaStream(m_mediaInputStream->getMediaStream());
        }


        m_video.set("srcObject", emscripten::val::null());
        disconnect(m_connection);
        m_connection = {};
        m_video.call<void>("remove");
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
    m_isStopped = false;
    m_toBePaused = true;
    m_video.call<void>("pause");
}

void QWasmVideoOutput::reset()
{
    // flush pending frame
    if (m_wasmSink)
        m_wasmSink->platformVideoSink()->setVideoFrame(QVideoFrame());

    m_source.clear();
    m_video.set("currentTime", emscripten::val(0));
    m_video.call<void>("load");
}

emscripten::val QWasmVideoOutput::surfaceElement()
{
    return m_video;
}

void QWasmVideoOutput::setSurface(QVideoSink *surface)
{
    if (!surface || surface == m_wasmSink) {
        return;
    }

    m_wasmSink = surface;
}

bool QWasmVideoOutput::isReady() const
{
    if (m_video.isUndefined() || m_video.isNull()) {
        // error
        return false;
    }

    return m_currentMediaStatus == MediaStatus::LoadedMedia;
 }

void QWasmVideoOutput::setSource(const QUrl &url)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << url;

    m_source = url.toString();

    if (m_video.isUndefined() || m_video.isNull()) {
        return;
    }

    if (url.isEmpty()) {
        stop();
        return;
    }
    if (url.isLocalFile()) {
        QFile localFile(url.toLocalFile());
        if (localFile.open(QIODevice::ReadOnly)) {
            setSource(&localFile);
        } else {
            qWarning() << "Failed to open file";
        }
        return;
    }

    updateVideoElementSource(m_source);
}

void QWasmVideoOutput::updateVideoElementSource(const QString &src)
{
    m_video.set("src", src.toStdString());
    m_video.call<void>("load");
}

void QWasmVideoOutput::addCameraSourceElement(const std::string &id)
{
    m_cameraIsReady = false;
    if (m_mediaInputStream)
        JsMediaInputStream::releaseInstance(m_cameraId);
    m_mediaInputStream = JsMediaInputStream::instance(id);

    m_mediaInputStream->setUseAudio(m_hasAudio);
    m_mediaInputStream->setUseVideo(true);

    connect(m_mediaInputStream, &JsMediaInputStream::mediaVideoStreamReady, this,
        [this]() {
            qCDebug(qWasmMediaVideoOutput) << "mediaVideoStreamReady" << m_shouldBeStarted;

            m_cameraIsReady = true;
            if (m_shouldBeStarted) {
                start();
                m_shouldBeStarted = false;
            }
        });

    m_cameraId = id;
}

void QWasmVideoOutput::setVideoConstraints(QSize resolution, float minFrameRate, float maxFrameRate)
{
    m_videoResolution = resolution;
    m_minFrameRate = minFrameRate;
    m_maxFrameRate = maxFrameRate;
}

void QWasmVideoOutput::setSource(QIODevice *stream)
{
    if (stream->bytesAvailable() == 0) {
        qWarning() << "data not available";
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("data not available"));
        return;
    }
    if (m_video.isUndefined() || m_video.isNull()) {
        emit errorOccured(QMediaPlayer::ResourceError, QStringLiteral("video surface error"));
        return;
    }

    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForData(stream);

    QByteArray buffer = stream->readAll();

    qstdweb::Blob contentBlob = qstdweb::Blob::copyFrom(buffer.data(), buffer.size(), mime.name().toStdString());

    emscripten::val window = qstdweb::window();

    if (window["safari"].isUndefined()) {
        emscripten::val contentUrl = window["URL"].call<emscripten::val>("createObjectURL", contentBlob.val());
        m_video.set("src", contentUrl);
        m_source = QString::fromStdString(contentUrl.as<std::string>());
    } else {
        // only Safari currently supports Blob with srcObject
        m_video.set("srcObject", contentBlob.val());
    }
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
                        >= seekableTimeRange.call<emscripten::val>("start", 0).as<double>()
                && positionToSetInSeconds
                        <= seekableTimeRange.call<emscripten::val>("end", 0).as<double>()) {
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
        bool isit = !QtPrivate::fuzzyCompare(
                seekableTimeRange.call<emscripten::val>("start", 0).as<double>(),
                seekableTimeRange.call<emscripten::val>("end", 0).as<double>());
        return isit;
    }
    return false;
}

void QWasmVideoOutput::createVideoElement(const std::string &id)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << this << id;
    // Create <video> element and add it to the page body

    emscripten::val document = emscripten::val::global("document");
    emscripten::val body = document["body"];

    // remove any previously created video element for this output
    if (!m_video.isUndefined() && !m_video.isNull())
        m_video.call<void>("remove");

    m_videoSurfaceId = id;
    m_video = document.call<emscripten::val>("createElement", std::string("video"));

    m_video.set("id", m_videoSurfaceId.c_str());
    m_video.call<void>("setAttribute", std::string("class"),
                       (m_currentVideoMode == QWasmVideoOutput::Camera ? std::string("Camera")
                                                                       : std::string("Video")));
    m_video.set("data-qvideocontext",
                emscripten::val(quintptr(reinterpret_cast<void *>(this))));

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
    document.call<emscripten::val>("createElement",
                                   std::string("source")).set("src", m_source.toStdString());

    // Set position:absolute, which makes it possible to position the video
    // element using x,y. coordinates, relative to its parent (the page's <body>
    // element)
    emscripten::val style = m_video["style"];
    style.set("position", "absolute");
    style.set("display", "none"); // hide

    if (!m_source.isEmpty())
        updateVideoElementSource(m_source);
}

void QWasmVideoOutput::createOffscreenElement(const QSize &offscreenSize)
{
    qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO;

    if (m_hasVideoFrame) // VideoFrame does not require offscreen canvas/context
        return;

    // create offscreen element for grabbing frames
    // OffscreenCanvas - no safari :(
    // https://developer.mozilla.org/en-US/docs/Web/API/OffscreenCanvas

    emscripten::val document = emscripten::val::global("document");

    // TODO use correct frameBytesAllocationSize?
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

void QWasmVideoOutput::removeCurrentVideoElement()
{
    if (!m_video.isUndefined() && !m_video.isNull())
        m_video.call<void>("remove");
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
                videoFrameTimerCallback();
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
            videoFrameTimerCallback();
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

void QWasmVideoOutput::updateVideoElementGeometry(const QRect &windowGeometry)
{
    QRect m_videoElementSource(windowGeometry.topLeft(), windowGeometry.size());

    emscripten::val style = m_video["style"];
    style.set("left", QStringLiteral("%1px").arg(m_videoElementSource.left()).toStdString());
    style.set("top", QStringLiteral("%1px").arg(m_videoElementSource.top()).toStdString());
    m_video.set("width", m_videoElementSource.width());
    m_video.set("height", m_videoElementSource.height());
    style.set("z-index", "999");

    if (!m_hasVideoFrame) {
        // offscreen
        m_offscreen.set("width", m_videoElementSource.width());
        m_offscreen.set("height", m_videoElementSource.height());
    }
}

qint64 QWasmVideoOutput::getDuration()
{
    // qt duration is in ms
    // js is sec

    if (m_video.isUndefined() || m_video.isNull())
        return 0;
    return m_video["duration"].as<double>() * 1000;
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
        qCDebug(qWasmMediaVideoOutput) << "offscreen canvas context could not be found";
        return;
    }
    emscripten::val document = emscripten::val::global("document");

    if (m_video.isUndefined() || m_video.isNull()) {
        qCDebug(qWasmMediaVideoOutput) << "video element could not be found";
        return;
    }

    const int videoWidth = m_video["videoWidth"].as<int>();
    const int videoHeight = m_video["videoHeight"].as<int>();

    if (videoWidth == 0 || videoHeight == 0)
        return;

    m_offscreenContext.call<void>("drawImage", m_video, 0, 0, videoWidth, videoHeight);

    emscripten::val frame = // one frame, Uint8ClampedArray
            m_offscreenContext.call<emscripten::val>("getImageData", 0, 0, videoWidth, videoHeight);

    const QSize frameBytesAllocationSize(videoWidth, videoHeight);

    // this seems to work ok, even though getImageData returns a Uint8ClampedArray
    QByteArray frameBytes = qstdweb::Uint8Array(frame["data"]).copyToQByteArray();

    QVideoFrameFormat frameFormat =
            QVideoFrameFormat(frameBytesAllocationSize, QVideoFrameFormat::Format_RGBA8888);

    QWasmVideoOutput *wasmVideoOutput = reinterpret_cast<QWasmVideoOutput *>(context);

    if (m_useCameraRotation)
        frameFormat.setRotation(wasmVideoOutput->m_rotateBy);
    if (m_streamFrameRate > 0)
        frameFormat.setStreamFrameRate(m_streamFrameRate);

    auto *textureDescription = QVideoTextureHelper::textureDescription(frameFormat.pixelFormat());

    QVideoFrame vFrame = QVideoFramePrivate::createFrame(
            std::make_unique<QMemoryVideoBuffer>(
                    std::move(frameBytes),
                    textureDescription->strideForWidth(frameFormat.frameWidth())), // width of line with padding
            frameFormat);

    if (!wasmVideoOutput->m_wasmSink) {
        qWarning() << "ERROR ALERT!! video sink not set";
    }
    wasmVideoOutput->m_wasmSink->setVideoFrame(vFrame);
}

// non webgl context with VideoFrame
void QWasmVideoOutput::videoFrameCallback(void *context)
{
    QWasmVideoOutput *videoOutput = reinterpret_cast<QWasmVideoOutput *>(context);
    if (!videoOutput)
        return;
    emscripten::val videoElement = videoOutput->currentVideoElement();

    // The VideoFrame constructor throws InvalidStateError when the browser compositor
    // has not yet committed the first decoded frame, even if readyState == 4 and
    // videoWidth > 0. Use a JS try-catch so the exception does not propagate into
    // the wasm runtime and abort the application.
    emscripten::val oneVideoFrame = emscripten::val::take_ownership(
            (EM_VAL)EM_ASM_INT({
                try {
                    return Emval.toHandle(new VideoFrame(Emval.toValue($0)));
                } catch(e) {
                    return Emval.toHandle(null);
                }
            }, videoElement.as_handle()));

    if (oneVideoFrame.isNull() || oneVideoFrame.isUndefined()) {
        qCDebug(qWasmMediaVideoOutput) << Q_FUNC_INFO << "VideoFrame not ready yet, skipping";
        return;
    }

    emscripten::val options = emscripten::val::object();
    emscripten::val rectOptions = emscripten::val::object();

    int displayWidth = oneVideoFrame["displayWidth"].as<int>();
    int displayHeight = oneVideoFrame["displayHeight"].as<int>();

    rectOptions.set("width", displayWidth);
    rectOptions.set("height", displayHeight);
    options.set("rect", rectOptions);

    emscripten::val frameBytesAllocationSize = oneVideoFrame.call<emscripten::val>("allocationSize", options);
    emscripten::val frameBuffer =
            emscripten::val::global("Uint8Array").new_(frameBytesAllocationSize);
    QWasmVideoOutput *wasmVideoOutput =
            reinterpret_cast<QWasmVideoOutput*>(videoElement["data-qvideocontext"].as<quintptr>());

    qstdweb::PromiseCallbacks copyToCallback;
    copyToCallback.thenFunc = [this, wasmVideoOutput, oneVideoFrame, frameBuffer,
                                displayWidth, displayHeight]
            (emscripten::val frameLayout)
    {
        if (frameLayout.isNull() || frameLayout.isUndefined()) {
            qCDebug(qWasmMediaVideoOutput) << "theres no frameLayout";
            return;
        }

        // frameBuffer now has a new frame, send to Qt
        const QSize frameSize(displayWidth,
                              displayHeight);

        QByteArray frameBytes = QByteArray::fromEcmaUint8Array(frameBuffer);

        constexpr auto pixelFormat = QVideoFrameFormat::Format_RGBA8888;
        QVideoFrameFormat frameFormat = QVideoFrameFormat(frameSize, pixelFormat);

        if (m_useCameraRotation)
            frameFormat.setRotation(wasmVideoOutput->m_rotateBy);
        if (m_streamFrameRate > 0)
            frameFormat.setStreamFrameRate(m_streamFrameRate);
        auto buffer = std::make_unique<QMemoryVideoBuffer>(
                std::move(frameBytes),
                frameLayout[0]["stride"].as<int>());

        QVideoFrame vFrame =
                QVideoFramePrivate::createFrame(std::move(buffer), std::move(frameFormat));

        if (!wasmVideoOutput) {
            qCDebug(qWasmMediaVideoOutput) << "ERROR:"
                                           << "data-qvideocontext not found";
            return;
        }
        if (!wasmVideoOutput->m_wasmSink) {
            qWarning() << "ERROR ALERT!! video sink not set";
            return;
        }
        wasmVideoOutput->m_wasmSink->setVideoFrame(vFrame);
        oneVideoFrame.call<emscripten::val>("close");
    };
    copyToCallback.catchFunc = [oneVideoFrame](emscripten::val error)
    {
        qCDebug(qWasmMediaVideoOutput) << "copyTo error"
                               << QString::fromStdString(error["name"].as<std::string>())
                               << QString::fromStdString(error["message"].as<std::string>());
        oneVideoFrame.call<emscripten::val>("close");
    };

    qstdweb::Promise::make(oneVideoFrame, u"copyTo"_s, std::move(copyToCallback), frameBuffer, options);
}

EM_JS(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE, qwasm_find_webgl_context_for_canvas, (EM_VAL canvasHandle), {
    var canvas = Emval.toValue(canvasHandle);
    for (var id in GL.contexts) {
        var entry = GL.contexts[id];
        if (entry && entry.GLctx && entry.GLctx.canvas === canvas)
            return parseInt(id);
    }
    return 0;
});

void QWasmVideoOutput::getWebGLContext()
{
    m_glContextHandle = 0;
    m_hasWebGLContext = false;

    QRhi *rhi = m_wasmSink ? m_wasmSink->rhi() : nullptr;
    if (!rhi || rhi->backend() != QRhi::OpenGLES2)
        return;

    const auto *nh = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    if (!nh || !nh->context)
        return;

    QOpenGLContext *ctx = nh->context;

    auto tryGetHandleFromSurface = [&]() -> bool {
        QSurface *surface = ctx->surface();
        if (!surface || surface->surfaceClass() != QSurface::Window)
            return false;
        QWindow *window = static_cast<QWindow *>(surface);
        if (!window->handle())
            return false;
        auto *wasmIface = window->nativeInterface<QNativeInterface::Private::QWasmWindow>();
        if (!wasmIface)
            return false;
        emscripten::val canvas = wasmIface->canvas();
        emscripten::val glCtx = canvas.call<emscripten::val>("getContext", std::string("webgl2"));
        if (glCtx.isNull() || glCtx.isUndefined())
            glCtx = canvas.call<emscripten::val>("getContext", std::string("webgl"));
        if (glCtx.isNull() || glCtx.isUndefined())
            return false;
        m_glContextHandle = qwasm_find_webgl_context_for_canvas(canvas.as_handle());
        m_hasWebGLContext = (m_glContextHandle > 0);
        return m_hasWebGLContext;
    };

    if (!tryGetHandleFromSurface())
        qWarning() << Q_FUNC_INFO << "could not locate WebGL canvas for the current RHI context";
}

// framemaker for webgl context
void QWasmVideoOutput::webglVideoFrameCallback(void *context)
{
    QWasmVideoOutput *wasmVideoOutput = reinterpret_cast<QWasmVideoOutput *>(context);
    if (!wasmVideoOutput)
        return;

    emscripten_webgl_make_context_current(wasmVideoOutput->m_glContextHandle);

    GLuint rawTexId = 0;
    glGenTextures(1, &rawTexId);
    QGlTextureHandle texHandle{ rawTexId };

    glBindTexture(GL_TEXTURE_2D, texHandle.get());

    int w = 0, h = 0;
    em_texImage2DFromVideo(wasmVideoOutput->m_videoSurfaceId.c_str(), &w, &h);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!texHandle || w == 0 || h == 0) {
        qCWarning(qWasmMediaVideoOutput) << "VideoFrame upload failed";
        return;
    }

    std::unique_ptr<QHwVideoBuffer> hwBuffer =
            std::make_unique<QWasmGLTextureVideoBuffer>(std::move(texHandle), QSize(w, h));

    QVideoFrameFormat frameFormat(QSize(w, h), QVideoFrameFormat::Format_RGBA8888);
    if (wasmVideoOutput->m_streamFrameRate > 0)
        frameFormat.setStreamFrameRate(wasmVideoOutput->m_streamFrameRate);
    QVideoFrame vFrame =
            QVideoFramePrivate::createFrame(std::move(hwBuffer), std::move(frameFormat));

    wasmVideoOutput->m_wasmSink->setVideoFrame(vFrame);
}

// default fallback for non VideoFrame
void QWasmVideoOutput::videoFrameTimerCallback()
{
    if (m_hasVideoFrame && !m_hasWebGLContext && !m_webGLContextChecked) {
        m_webGLContextChecked = true;
        getWebGLContext();
    }

    if (isPlatformiOs()) {
        m_useCameraRotation = true;
        emscripten::val stream = m_video["srcObject"];
        emscripten::val vTraks = stream.call<emscripten::val>("getVideoTracks");

        if (!vTraks.isUndefined() && vTraks["length"].as<int>() > 0) {
            emscripten::val trak = vTraks[0];
            emscripten::val settings = trak.call<emscripten::val>("getSettings");

            if (settings["facingMode"].as<std::string>() == "user")
                m_cameraMode = QWasmVideoOutput::Front;
            else
                m_cameraMode = QWasmVideoOutput::Back;
            // now we know camera, set m_rotateBy
            orientationChanged(getCurrentOrientationIndex());
        }
    }

    // Single-shot callback: re-registers each frame so multiple QWasmVideoOutput
    // instances can coexist. emscripten_request_animation_frame_loop allows only one
    // active loop globally and would cancel another instance.
    static EM_BOOL (*frame)(double, void *) = [](double frameTime, void *context) -> EM_BOOL {

        Q_UNUSED(frameTime);

        QWasmVideoOutput *videoOutput = reinterpret_cast<QWasmVideoOutput *>(context);
        if (!videoOutput || videoOutput->m_isStopped) {
            qCWarning(qWasmMediaVideoOutput) << "frame loop exit: isStopped=" << (videoOutput ? videoOutput->m_isStopped : true)
                                             << "mode=" << (videoOutput ? videoOutput->m_currentVideoMode : -1);
            return false;
        }

        if (videoOutput->m_currentVideoMode == QWasmVideoOutput::VideoDisplay
            && videoOutput->m_currentMediaStatus != MediaStatus::LoadedMedia) {
            emscripten_request_animation_frame(frame, context);
            return true;
        }

        emscripten::val videoElement = videoOutput->currentVideoElement();
        if (videoElement.isNull() || videoElement.isUndefined()) {
            qCWarning(qWasmMediaVideoOutput) << "frame loop exit: video element null, mode=" << videoOutput->m_currentVideoMode;
            return false;
        }

        if (videoElement["paused"].as<bool>() || videoElement["ended"].as<bool>()
            || videoElement["readyState"].as<int>() < 2) {
            qCDebug(qWasmMediaVideoOutput) << "frame loop waiting: mode=" << videoOutput->m_currentVideoMode
                                           << "paused=" << videoElement["paused"].as<bool>()
                                           << "ended=" << videoElement["ended"].as<bool>()
                                           << "readyState=" << videoElement["readyState"].as<int>();
            emscripten_request_animation_frame(frame, context);
            return true;
        }

        qCDebug(qWasmMediaVideoOutput) << "frame loop render: mode=" << videoOutput->m_currentVideoMode
                                       << "glHandle=" << videoOutput->m_glContextHandle;

        if (videoOutput->m_hasVideoFrame) {
            if (videoOutput->m_glContextHandle)
                videoOutput->webglVideoFrameCallback(context);
            else
                videoOutput->videoFrameCallback(context);
        } else {
            videoOutput->videoComputeFrame(context);
        }

        emscripten_request_animation_frame(frame, context);
        return true;
    };
    if ((!m_isStopped  && m_video["className"].as<std::string>() == "Camera" && m_cameraIsReady)
        || (!m_isStopped  && m_currentVideoMode == QWasmVideoOutput::SurfaceCapture)
        || isReady())
        emscripten_request_animation_frame(frame, this);
}

QVideoFrameFormat::PixelFormat QWasmVideoOutput::fromJsPixelFormat(std::string_view videoFormat)
{
    if (videoFormat == "I420")
        return QVideoFrameFormat::Format_YUV420P;
    // no equivalent pixel format
    //   else if (videoFormat == "I420A") // AYUV ?
    else if (videoFormat == "I422")
        return QVideoFrameFormat::Format_YUV422P;
    // no equivalent pixel format
    //     else if (videoFormat == "I444")
    else if (videoFormat == "NV12")
        return QVideoFrameFormat::Format_NV12;
    else if (videoFormat == "RGBA")
        return QVideoFrameFormat::Format_RGBA8888;
    else if (videoFormat == "RGBX")
        return QVideoFrameFormat::Format_RGBX8888;
    else if (videoFormat == "BGRA")
        return QVideoFrameFormat::Format_BGRA8888;
    else if (videoFormat == "BGRX")
        return QVideoFrameFormat::Format_BGRX8888;

    return QVideoFrameFormat::Format_Invalid;
}

emscripten::val QWasmVideoOutput::getDeviceCapabilities()
{
    emscripten::val stream = m_video["srcObject"];
    if ((!stream.isNull() && !stream.isUndefined()) && stream["active"].as<bool>()) {
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
