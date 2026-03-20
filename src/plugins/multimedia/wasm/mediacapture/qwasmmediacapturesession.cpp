// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediacapturesession_p.h"
#include "mediacapture/qwasmimagecapture_p.h"
#include "private/qcapturablewindow_p.h"
#include <QUuid>
#include "private/qplatformcapturablewindows_p.h"

#include "qwasmcamera_p.h"
#include "qwasmscreencapture_p.h"
#include "qwasmwindowcapture_p.h"
#include "qwasmaudioinput_p.h"
#include <private/qplatformmediaintegration_p.h>
#include <private/qwasmmediadevices_p.h>

#include <private/qplatformaudioinput_p.h>
#include <QAudioDevice>


Q_LOGGING_CATEGORY(qWasmMediaCaptureSession, "qt.multimedia.wasm.capturesession")

QWasmMediaCaptureSession::QWasmMediaCaptureSession() :
      QPlatformMediaCaptureSession()
{
    QWasmMediaDevices::instance()->initDevices();
}

QWasmMediaCaptureSession::~QWasmMediaCaptureSession() = default;

QPlatformCamera *QWasmMediaCaptureSession::camera()
{
    return m_camera;
}

void QWasmMediaCaptureSession::setCamera(QPlatformCamera *camera)
{
    if (!camera) {
        if (m_camera == nullptr)
            return;
        m_camera->setActive(false);
        m_camera = nullptr;
    } else {
        QWasmCamera *wasmCamera = static_cast<QWasmCamera *>(camera);
        if (m_camera == wasmCamera)
            return;
        m_camera = wasmCamera;
        m_camera->setCaptureSession(this);
    }
    emit cameraChanged();
}

QPlatformImageCapture *QWasmMediaCaptureSession::imageCapture()
{
    return m_imageCapture;
}

void QWasmMediaCaptureSession::setImageCapture(QPlatformImageCapture *imageCapture)
{
    if (m_imageCapture == imageCapture)
        return;

    if (m_imageCapture)
        m_imageCapture->setCaptureSession(nullptr);

    m_imageCapture = static_cast<QWasmImageCapture *>(imageCapture);

    if (m_imageCapture) {
        m_imageCapture->setCaptureSession(this);

        emit imageCaptureChanged();
    }
}

QPlatformMediaRecorder *QWasmMediaCaptureSession::mediaRecorder()
{
    return m_mediaRecorder;
}

void QWasmMediaCaptureSession::setMediaRecorder(QPlatformMediaRecorder *mediaRecorder)
{
    if (m_mediaRecorder == mediaRecorder)
        return;

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(nullptr);

    m_mediaRecorder = static_cast<QWasmMediaRecorder *>(mediaRecorder);

    if (m_mediaRecorder)
        m_mediaRecorder->setCaptureSession(this);
}

void QWasmMediaCaptureSession::setAudioInput(QPlatformAudioInput *input)
{
    if (m_audioInput == input)
        return;

    m_needsAudio = (bool)input;
    m_audioInput = input;
}

bool QWasmMediaCaptureSession::hasAudio()
{
    return m_needsAudio;
}

void QWasmMediaCaptureSession::setVideoPreview(QVideoSink *sink)
{
    if (!sink || m_wasmSink == sink)
        return;
    m_wasmSink = sink;
}

void QWasmMediaCaptureSession::setAudioOutput(QPlatformAudioOutput *output)
{
    if (m_audioOutput == output)
        return;
    m_audioOutput = output;
}

void QWasmMediaCaptureSession::setReadyForCapture(bool ready)
{
    if (m_imageCapture)
        m_imageCapture->setReadyForCapture(ready);
}

QPlatformSurfaceCapture *QWasmMediaCaptureSession::screenCapture()
{
    return m_screenCapture;
}

void QWasmMediaCaptureSession::setScreenCapture(QPlatformSurfaceCapture *surfaceCapture)
{
    m_screenCapture = surfaceCapture;
    if (surfaceCapture == nullptr) {
        // clear stuff
        m_displaySurface = "";
        if (m_videoOutput && !m_videoOutput->currentVideoElement().isUndefined()) {
            m_videoOutput->stop();
        }
    } else {
        m_displaySurface = "browser";
        QWasmScreenCapture *wasmScreenCapture =
            static_cast<QWasmScreenCapture *>(m_screenCapture);
        wasmScreenCapture->setCaptureSession(this);
    }
}

QPlatformSurfaceCapture *QWasmMediaCaptureSession::windowCapture()
{
    return m_windowCapture;
}

void QWasmMediaCaptureSession::setWindowCapture(QPlatformSurfaceCapture *surfaceCapture)
{
    m_windowCapture = surfaceCapture;

    if (surfaceCapture == nullptr) {
        // clear stuff
        m_displaySurface = "";
        if (m_videoOutput && !m_videoOutput->currentVideoElement().isUndefined()) {
            m_videoOutput->stop();
            m_videoOutput->removeCurrentVideoElement();
        }
        return;
    } else {
        // is window capture window or monitor ?
        m_displaySurface = "window";
        QWasmWindowCapture *wasmWindowCapture =
            static_cast<QWasmWindowCapture *>(m_windowCapture);
        wasmWindowCapture->setCaptureSession(this);
    }
}

void QWasmMediaCaptureSession::setVideoSource(std::string surfacetype)
{
    m_displaySurface = surfacetype;
    if (surfacetype.empty()) {
        // Jane, stop this crazy thing!
        if (m_videoOutput && !m_videoOutput->currentVideoElement().isUndefined()) {
            m_videoOutput->stop();
            m_videoOutput->removeCurrentVideoElement();
        }
        return;
    }
    emscripten::val navigator = emscripten::val::global("navigator");
    emscripten::val mediaDevices = navigator["mediaDevices"];

    if (mediaDevices.isNull() || mediaDevices.isUndefined()) {
        qWarning() << "No media devices found";
        return;
    }
    m_videoOutput = std::make_unique<QWasmVideoOutput>();
    m_videoOutput->setVideoMode(QWasmVideoOutput::SurfaceCapture);

    m_videoOutput->setSurface(m_wasmSink);

    emscripten::val constraints = emscripten::val::object();
    emscripten::val videoConstraint = emscripten::val::object();
    emscripten::val audioConstraint = emscripten::val::object();

    videoConstraint.set("displaySurface", m_displaySurface);
    videoConstraint.set("resizeMode", std::string("crop-and-scale"));

    constraints.set("monitorTypeSurfaces", surfacetype == "window" ?
         std::string("exclude") : std::string("include")); // whole monitor

    constraints.set("surfaceSwitching", std::string("exclude"));
    constraints.set("selfBrowserSurface", "include"); // mirror effect

    constraints.set("logicalSurface", true);

    audioConstraint.set("systemAudio", true);

    constraints.set("video", videoConstraint);
    constraints.set("audio", audioConstraint);

    // will ask for permissions!
    qstdweb::PromiseCallbacks getDisplayMediaCallback{
        // default
        .thenFunc =
        [this](emscripten::val stream) {
            qCDebug(qWasmMediaCaptureSession) << "getDisplayMediaSuccess"
                                                      << m_displaySurface;
            if (stream.isUndefined() || stream.isNull()) {
                qWarning() << "No media stream found error";
                return;
            }
            m_mediaCaptureStream = stream;

            // new Video element
            constexpr QSize initialSize(0, 0);
            constexpr QRect initialRect(QPoint(0, 0), initialSize);
            QUuid videoElementId = QUuid::createUuid();

            m_videoOutput->createVideoElement(m_displaySurface + "_capture_"
                                              + videoElementId.toString(QUuid::WithoutBraces).toStdString()); // videoElementId
            m_videoOutput->doElementCallbacks();
            m_videoOutput->createOffscreenElement(initialSize);
            m_videoOutput->updateVideoElementGeometry(initialRect);

             // set video element src to this
            if (m_displaySurface == "window") {

                QWasmWindowCapture *wasmWindowCapture =
                    static_cast<QWasmWindowCapture *>(m_windowCapture);
                wasmWindowCapture->setVideoOutput(m_videoOutput.get());

               QUuid uid(QString::fromStdString(stream["id"].as<std::string>()));

                m_capuredWindows.push_back(QCapturableWindowPrivate::create(
                    static_cast<QCapturableWindowPrivate::Id>(uid.toByteArray().toLong()),
                    QString::fromStdString(stream["id"].as<std::string>())));

                wasmWindowCapture->setVideoStream(stream);
                emit windowCaptureChanged();
            } else {

                QWasmScreenCapture *wasmScreenCapture =
                    static_cast<QWasmScreenCapture *>(m_screenCapture);
                wasmScreenCapture->setVideoOutput(m_videoOutput.get());

                wasmScreenCapture->setVideoStream(stream);
                emit screenCaptureChanged();
            }
        },
        .catchFunc =
        [](emscripten::val error) {
            qCDebug(qWasmMediaCaptureSession)
                    << "getDisplayMedia  fail"
                    << QString::fromStdString(error["name"].as<std::string>())
                    << QString::fromStdString(error["message"].as<std::string>());

            // permission denied or error
            // "NotAllowedError "Permission Denied"
        }
    };

    qstdweb::Promise::make(mediaDevices, QStringLiteral("getDisplayMedia"),
                           std::move(getDisplayMediaCallback), constraints);
}
