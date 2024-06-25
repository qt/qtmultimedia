// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMMEDIACAPTURESESSION_H
#define QWASMMEDIACAPTURESESSION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwasmimagecapture_p.h"

#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediaintegration_p.h>
#include "qwasmmediarecorder_p.h"
#include "qwasmvideooutput_p.h"

#include "private/qcapturablewindow_p.h"

#include <QScopedPointer>
#include <QtCore/qloggingcategory.h>
#include <common/qwasmvideooutput_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmMediaCaptureSession)

class QAudioInput;
class QWasmCamera;

class QWasmMediaCaptureSession : public QPlatformMediaCaptureSession
{

public:
    explicit QWasmMediaCaptureSession();
    ~QWasmMediaCaptureSession() override;

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformSurfaceCapture *screenCapture() override;
    void setScreenCapture(QPlatformSurfaceCapture *) override;

    QPlatformSurfaceCapture *windowCapture() override;
    void setWindowCapture(QPlatformSurfaceCapture *) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;
    QPlatformAudioInput * audioInput() const { return m_audioInput; }
    void setVideoPreview(QVideoSink *sink) override;
    void setAudioOutput(QPlatformAudioOutput *output) override;

    bool hasAudio();
    QVideoSink *videoSink() { return m_wasmSink; }
    void setReadyForCapture(bool ready);
    void setVideoSource(std::string surfacetype);

private:
    QWasmMediaRecorder *m_mediaRecorder = nullptr;

    QWasmCamera *m_camera = nullptr;
    QWasmImageCapture *m_imageCapture = nullptr;
    QPlatformSurfaceCapture *m_windowCapture = nullptr;
    QPlatformSurfaceCapture *m_screenCapture = nullptr;

    QPlatformAudioInput *m_audioInput = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;
    std::unique_ptr<QWasmVideoOutput> m_videoOutput;

    bool m_needsAudio = false;
    QVideoSink *m_wasmSink = nullptr;

    emscripten::val m_mediaCaptureStream;
    std::string m_displaySurface;
    QList<QCapturableWindow> m_capuredWindows;
};

QT_END_NAMESPACE

#endif // QWASMMEDIACAPTURESESSION_H
