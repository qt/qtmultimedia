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
#include <QPointer>
#include <QGuiApplication>
#include <QOpenGLContext>

#include <QtGui/rhi/qrhi_platform.h>
#include <qpa/qplatformwindow_p.h>

#include <GLES2/gl2.h>

#include "qwasmvideooutput_p.h"

#include <qvideosink.h>
#include <private/qplatformvideosink_p.h>
#include <private/qstdweb_p.h>
#include <QTimer>

#include <emscripten/bind.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE


using namespace emscripten;
using namespace Qt::Literals;

Q_LOGGING_CATEGORY(qWasmMediaVideoOutput, "qt.multimedia.wasm.videooutput")

bool QWasmVideoOutput::isPlatformiOs()
{
    emscripten::val platformObject = emscripten::val::global("navigator")["platform"];
    if (platformObject.call<bool>("includes", emscripten::val("iPhone"))
        || platformObject.call<bool>("includes", emscripten::val("iPad")))
        return true;
    return false;
}

QWasmVideoOutput::QWasmVideoOutput(QObject *parent)
    : QObject{ parent }, m_frameGrabber(this)
{
    if (isPlatformiOs()) {
        connect(this, &QWasmVideoOutput::orientationChanged, this,
                &QWasmVideoOutput::applyIosRotation);

        emscripten_set_orientationchange_callback(this, false, &QWasmVideoOutput::orientationchangeCallback);
    }
}

QWasmVideoOutput::~QWasmVideoOutput()
{
    if (m_mediaInputStream) {
        if (m_streamStarted) {
            m_streamStarted = false;
            m_mediaInputStream->unregisterConsumer();
        }
        JsMediaInputStream::releaseInstance(m_cameraId);
    }
}

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

// Calls play() on the video element and handles the returned promise.
//
// A play() that is interrupted by a new load request (e.g. a fresh srcObject or
// a load() call) rejects with AbortError. That rejection is benign, but if the
// promise is left unhandled the browser reports it as an uncaught rejection.
//
// A play() that is refused outright is not benign, the element stays unplayable
// and fires no further media events, so nothing else would take the player out
// of the state it is in. Report those instead of swallowing them.
void QWasmVideoOutput::playVideoElement()
{
    emscripten::val promise = m_video.call<emscripten::val>("play");
    // Older browsers return undefined from play() instead of a Promise. Only
    // attach handlers when we actually got a Promise back.
    if (promise.isUndefined() || promise.isNull() || promise["then"].isUndefined())
        return;

    QPointer<QWasmVideoOutput> videoOutput(this);
    qstdweb::Promise::adoptPromise(promise, {
        .catchFunc = [videoOutput](emscripten::val error) {
            if (!videoOutput || videoOutput->m_isStopped)
                return;

            // The rejection value is not guaranteed to be a DOMException, so do
            // not assume either property is there.
            const auto stringProperty = [&error](const char *key) {
                return error[key].isUndefined() || error[key].isNull()
                        ? QString()
                        : QString::fromStdString(error[key].as<std::string>());
            };
            const QString errorName = stringProperty("name");
            const QString errorMessage = stringProperty("message");
            // The browsers word this differently and some do not mention play()
            // at all, so keep the origin in the string the application sees.
            const QString errorString =
                    "video.play(): "_L1 + (errorMessage.isEmpty() ? errorName : errorMessage);

            // Not being allowed to play is a policy refusal, not an interruption.
            // The element is not sitting at a resume point, it is unplayable
            // until the user acts, so report it regardless of what the paused
            // attribute happens to say.
            if (errorName == "NotAllowedError"_L1) {
                qCWarning(qWasmMediaVideoOutput) << "video.play() not allowed:" << errorMessage;
                emit videoOutput->readyChanged(false);
                emit videoOutput->stateChanged(QWasmMediaPlayer::Stopped);
                emit videoOutput->errorOccured(QMediaPlayer::AccessDeniedError, errorString);
                return;
            }

            // Anything else is delivered a microtask late, and an AbortError from
            // a new load request is routinely followed by a fresh play() that has
            // already succeeded by now. Trust the element over the rejection.
            if (!videoOutput->m_video["paused"].as<bool>()) {
                qCDebug(qWasmMediaVideoOutput) << "video.play() rejected with" << errorName
                                               << "but playback resumed, ignoring";
                return;
            }

            qCWarning(qWasmMediaVideoOutput) << "video.play() rejected:" << errorName
                                             << errorMessage;
            emit videoOutput->readyChanged(false);
            emit videoOutput->stateChanged(QWasmMediaPlayer::Stopped);
            emit videoOutput->errorOccured(QMediaPlayer::ResourceError, errorString);
        }
    });
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
        playVideoElement();
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
                    emscripten::val newStream = m_mediaInputStream->getMediaStream();
                    // mediaVideoStreamReady may fire again for an already-attached
                    // stream (e.g. a restart on a still-active camera). Re-setting
                    // srcObject would interrupt the in-flight play() and reject its
                    // promise with AbortError, so skip it when nothing changed.
                    if (m_video["srcObject"].equals(newStream)) {
                        emit readyChanged(true);
                        return;
                    }
                    m_video.set("srcObject", newStream);

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

                    playVideoElement();

                    emit readyChanged(true);

                });
        m_mediaInputStream->setUseAudio(false);
        m_shouldBeStarted = true;
        m_mediaInputStream->setVideoConstraints(m_videoResolution, m_minFrameRate, m_maxFrameRate);
        if (!m_streamStarted) {
            m_streamStarted = true;
            m_mediaInputStream->registerConsumer();
        }
        m_mediaInputStream->setStreamDevice(m_cameraId);

    } break;
    };

    m_isStopped = false;

    if (m_currentVideoMode != QWasmVideoOutput::Camera
        && m_currentVideoMode != QWasmVideoOutput::SurfaceCapture) {
        playVideoElement();
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
        } else if (m_mediaInputStream && m_streamStarted) {
            // Only stop the shared MediaStream once the last consumer of this
            // camera goes away; other displays of the same camera keep running.
            m_streamStarted = false;
            m_mediaInputStream->unregisterConsumer();
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

void QWasmVideoOutput::removeCurrentVideoElement()
{
    if (!m_video.isUndefined() && !m_video.isNull())
        m_video.call<void>("remove");
}

void QWasmVideoOutput::updateVideoElementGeometry(const QRect &windowGeometry)
{
    QRect videoElementRect(windowGeometry.topLeft(), windowGeometry.size());

    emscripten::val style = m_video["style"];
    style.set("left", QStringLiteral("%1px").arg(videoElementRect.left()).toStdString());
    style.set("top", QStringLiteral("%1px").arg(videoElementRect.top()).toStdString());
    m_video.set("width", videoElementRect.width());
    m_video.set("height", videoElementRect.height());
    style.set("z-index", "999");
}

qint64 QWasmVideoOutput::getDuration()
{
    // qt duration is in ms
    // js is sec

    if (m_video.isUndefined() || m_video.isNull())
        return 0;
    return m_video["duration"].as<double>() * 1000;
}

void QWasmVideoOutput::setPlaybackRate(qreal rate)
{
    m_video.set("playbackRate", emscripten::val(rate));
}

qreal QWasmVideoOutput::playbackRate()
{
    return (m_video.isUndefined() || m_video.isNull()) ? 0 : m_video["playbackRate"].as<float>();
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
