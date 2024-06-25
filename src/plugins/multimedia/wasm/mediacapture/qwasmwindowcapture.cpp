// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmwindowcapture_p.h"
#include "qwasmmediacapturesession_p.h"
#include "qwasmcamera_p.h"
#include "qwasmvideosink_p.h"
#include <QMediaCaptureSession>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcWasmWindowcapture, "qt.multimedia.wasm.windowcapture")

QWasmWindowCapture::QWasmWindowCapture(QWindowCapture *winCap) :
      QPlatformSurfaceCapture(WindowSource{})
{
    setCaptureSession(dynamic_cast<QPlatformMediaCaptureSession *>(winCap->captureSession()));
}

QWasmWindowCapture::~QWasmWindowCapture() = default;

QVideoFrameFormat QWasmWindowCapture::frameFormat() const
{
    return {};
}

bool QWasmWindowCapture::setActiveInternal(bool active)
{
    if (active)
        m_captureSession->setVideoSource("window");
    else
        m_captureSession->setVideoSource("");

    return true;
}
void QWasmWindowCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    m_captureSession = dynamic_cast<QWasmMediaCaptureSession *>(session);
}

void QWasmWindowCapture::setVideoStream(emscripten::val stream)
{
    if (m_videoOutput) {
        emscripten::val videoTracks = stream.call<emscripten::val>("getVideoTracks");
        if (videoTracks.isNull() || videoTracks.isUndefined()) {
            qWarning() << Q_FUNC_INFO << "videoTracks is null";

            return;
        }

        emscripten::val videoSettings = videoTracks[0].call<emscripten::val>("getSettings");

        if (!videoSettings.isNull() || !videoSettings.isUndefined()) {
            int width = videoSettings["width"].as<int>();
            int height = videoSettings["height"].as<int>();

            QSize initialSize(width, height);
            QRect initialRect(QPoint(0, 0),initialSize);

            m_videoOutput->updateVideoElementGeometry(initialRect);

        }
        emscripten::val videoElement = m_videoOutput->currentVideoElement();

        videoElement.set("srcObject", stream);
        m_videoOutput->start();
    }
}

void QWasmWindowCapture::setVideoOutput(QWasmVideoOutput *object)
{
    if (object)
        m_videoOutput = object;
}

QT_END_NAMESPACE
