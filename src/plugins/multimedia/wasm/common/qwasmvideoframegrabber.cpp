// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmvideoframegrabber_p.h"
#include "qwasmvideooutput_p.h"
#include "qwasmgltexturevideobuffer_p.h"

#include <QDebug>
#include <QOpenGLContext>
#include <QtGui/rhi/qrhi_platform.h>
#include <qpa/qplatformwindow_p.h>

#include <GLES2/gl2.h>

#include <qvideosink.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qvideoframe_p.h>
#include <private/qstdweb_p.h>

#include <emscripten/emscripten.h>
#include <emscripten/em_js.h>
#include <emscripten/html5.h>
#include <emscripten/val.h>

QT_BEGIN_NAMESPACE

using namespace emscripten;
using namespace Qt::Literals;

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

EM_JS(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE, qwasm_find_webgl_context_for_canvas, (EM_VAL canvasHandle), {
    var canvas = Emval.toValue(canvasHandle);
    for (var id in GL.contexts) {
        var entry = GL.contexts[id];
        if (entry && entry.GLctx && entry.GLctx.canvas === canvas)
            return parseInt(id);
    }
    return 0;
});

QWasmVideoFrameGrabber::QWasmVideoFrameGrabber(QWasmVideoOutput *videoOutput)
    : m_videoOutput(videoOutput)
{
}

void QWasmVideoFrameGrabber::detectWebGLContext()
{
    m_glContextHandle = 0;
    m_hasWebGLContext = false;

    QRhi *rhi = m_videoOutput->m_wasmSink ? m_videoOutput->m_wasmSink->rhi() : nullptr;
    if (!rhi || rhi->backend() != QRhi::OpenGLES2)
        return;

    const auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    if (!nativeHandles || !nativeHandles->context)
        return;
    QOpenGLContext *openGLContext = nativeHandles->context;

    auto tryGetHandleFromSurface = [&]() -> bool {
        QSurface *surface = openGLContext->surface();
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

// software path, copies VideoFrame data into a QMemoryVideoBuffer
void QWasmVideoFrameGrabber::processVideoFrame()
{
    emscripten::val videoElement = m_videoOutput->currentVideoElement();

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
    QWasmVideoOutput *videoOutput = m_videoOutput;

    qstdweb::PromiseCallbacks copyToCallback;
    copyToCallback.thenFunc = [videoOutput, oneVideoFrame, frameBuffer,
                                displayWidth, displayHeight]
            (emscripten::val frameLayout)
    {
        if (frameLayout.isNull() || frameLayout.isUndefined()) {
            qCDebug(qWasmMediaVideoOutput) << "theres no frameLayout";
            return;
        }

        // frameBuffer now has a new frame, send to Qt
        const QSize frameSize(displayWidth, displayHeight);

        QByteArray frameBytes = QByteArray::fromEcmaUint8Array(frameBuffer);

        QVideoFrameFormat::PixelFormat pixelFormat =
                fromJsPixelFormat(oneVideoFrame["format"].as<std::string>());
        if (pixelFormat == QVideoFrameFormat::Format_Invalid)
            pixelFormat = QVideoFrameFormat::Format_RGBA8888;
        QVideoFrameFormat frameFormat = QVideoFrameFormat(frameSize, pixelFormat);

        if (videoOutput->m_useCameraRotation)
            frameFormat.setRotation(videoOutput->m_rotateBy);
        if (videoOutput->m_streamFrameRate > 0)
            frameFormat.setStreamFrameRate(videoOutput->m_streamFrameRate);
        auto buffer = std::make_unique<QMemoryVideoBuffer>(
                std::move(frameBytes),
                frameLayout[0]["stride"].as<int>());

        QVideoFrame vFrame =
                QVideoFramePrivate::createFrame(std::move(buffer), std::move(frameFormat));

        if (!videoOutput->m_wasmSink) {
            qWarning() << "ERROR ALERT!! video sink not set";
            return;
        }
        videoOutput->m_wasmSink->setVideoFrame(vFrame);
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

// zero-readback path, uploads the VideoFrame straight into a GL texture
void QWasmVideoFrameGrabber::processWebGLVideoFrame()
{
    emscripten_webgl_make_context_current(m_glContextHandle);

    GLuint rawTextureId = 0;
    glGenTextures(1, &rawTextureId);
    QGlTextureHandle textureHandle{ rawTextureId };

    glBindTexture(GL_TEXTURE_2D, textureHandle.get());

    int width = 0, height = 0;
    em_texImage2DFromVideo(m_videoOutput->m_videoSurfaceId.c_str(), &width, &height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (!textureHandle || width == 0 || height == 0) {
        qCWarning(qWasmMediaVideoOutput) << "VideoFrame upload failed";
        return;
    }

    std::unique_ptr<QHwVideoBuffer> hwBuffer =
            std::make_unique<QWasmGLTextureVideoBuffer>(
                    std::move(textureHandle), QSize(width, height),
                    m_glContextHandle,
                    m_videoOutput->m_wasmSink ? m_videoOutput->m_wasmSink->rhi() : nullptr);

    QVideoFrameFormat frameFormat(QSize(width, height), QVideoFrameFormat::Format_RGBA8888);
    if (m_videoOutput->m_streamFrameRate > 0)
        frameFormat.setStreamFrameRate(m_videoOutput->m_streamFrameRate);
    QVideoFrame vFrame =
            QVideoFramePrivate::createFrame(std::move(hwBuffer), std::move(frameFormat));

    m_videoOutput->m_wasmSink->setVideoFrame(vFrame);
}

void QWasmVideoFrameGrabber::startFrameLoop()
{
    if (!m_webGLContextChecked) {
        m_webGLContextChecked = true;
        detectWebGLContext();
    }

    m_videoOutput->detectIosCameraRotation();

    // Single-shot callback: re-registers each frame so multiple QWasmVideoOutput
    // instances can coexist. emscripten_request_animation_frame_loop allows only one
    // active loop globally and would cancel another instance.
    static EM_BOOL (*frame)(double, void *) = [](double frameTime, void *context) -> EM_BOOL {

        Q_UNUSED(frameTime);

        QWasmVideoFrameGrabber *frameGrabber = reinterpret_cast<QWasmVideoFrameGrabber *>(context);
        QWasmVideoOutput *videoOutput = frameGrabber ? frameGrabber->m_videoOutput : nullptr;
        if (!videoOutput || videoOutput->m_isStopped) {
            qCWarning(qWasmMediaVideoOutput) << "frame loop exit: isStopped=" << (videoOutput ? videoOutput->m_isStopped : true)
                                             << "mode=" << (videoOutput ? videoOutput->m_currentVideoMode : -1);
            return false;
        }

        if (videoOutput->m_currentVideoMode == QWasmVideoOutput::VideoDisplay
            && videoOutput->m_currentMediaStatus != QWasmVideoOutput::MediaStatus::LoadedMedia) {
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
                                       << "glHandle=" << frameGrabber->m_glContextHandle;

        if (frameGrabber->m_glContextHandle)
            frameGrabber->processWebGLVideoFrame();
        else
            frameGrabber->processVideoFrame();

        emscripten_request_animation_frame(frame, context);
        return true;
    };
    if ((!m_videoOutput->m_isStopped
         && m_videoOutput->m_video["className"].as<std::string>() == "Camera"
         && m_videoOutput->m_cameraIsReady)
        || (!m_videoOutput->m_isStopped
            && m_videoOutput->m_currentVideoMode == QWasmVideoOutput::SurfaceCapture)
        || m_videoOutput->isReady())
        emscripten_request_animation_frame(frame, this);
}

QVideoFrameFormat::PixelFormat QWasmVideoFrameGrabber::fromJsPixelFormat(std::string_view videoFormat)
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

QT_END_NAMESPACE
