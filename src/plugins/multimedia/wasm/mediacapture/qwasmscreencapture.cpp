// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmscreencapture_p.h"
#include "qwasmmediacapturesession_p.h"
#include "qwasmcamera_p.h"
#include "qwasmvideosink_p.h"
#include <QMediaCaptureSession>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcWasmsSreenCapture, "qt.multimedia.wasm.screencapture")

QWasmScreenCapture::QWasmScreenCapture(QScreenCapture *screenCap)
    : QPlatformSurfaceCapture(ScreenSource{})
{
    m_screenCapture = screenCap;
}

QWasmScreenCapture::~QWasmScreenCapture() = default;

QVideoFrameFormat QWasmScreenCapture::frameFormat() const
{
    return {};
}

bool QWasmScreenCapture::setActiveInternal(bool active)
{
    if (active)
        m_captureSession->setVideoSource("monitor");
    else
        m_captureSession->setVideoSource("");
    return true;
}

void QWasmScreenCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_captureSession = static_cast<QWasmMediaCaptureSession *>(session);
}

void QWasmScreenCapture::setVideoStream(emscripten::val stream)
{
    if (m_videoOutput) {
        emscripten::val videoTracks = stream.call<emscripten::val>("getVideoTracks");
        if (videoTracks.isNull() || videoTracks.isUndefined()) {
            return;
        }
        emscripten::val videoSettings = videoTracks[0].call<emscripten::val>("getSettings");
        float pixelRatio = emscripten::val::global("window")["devicePixelRatio"].as<float>();

        if (!videoSettings.isNull() || !videoSettings.isUndefined()) {
            // double fRate = videoSettings["frameRate"].as<double>(); TODO
            int width = static_cast<int>(videoSettings["width"].as<int>() / pixelRatio);
            int height = static_cast<int>(videoSettings["height"].as<int>() / pixelRatio);

            QSize initialSize(width, height);
            QRect initialRect(QPoint(0, 0),initialSize);

            m_videoOutput->updateVideoElementGeometry(initialRect);
        }

        emscripten::val videoElement = m_videoOutput->currentVideoElement();
        if (videoElement.isUndefined()) {
            qWarning() << "Could not find video element";
            return;
        }
        videoElement.set("srcObject", stream);
        m_videoOutput->start();
    }
}

void QWasmScreenCapture::setVideoOutput(QWasmVideoOutput *object)
{
    if (object)
        m_videoOutput = object;
}

QT_END_NAMESPACE
