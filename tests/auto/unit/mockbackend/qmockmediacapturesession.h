// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMOCKMEDIACAPTURESESSION_H
#define QMOCKMEDIACAPTURESESSION_H

#include "qmockmediaencoder.h"
#include "qmockimagecapture.h"
#include "qmockcamera.h"
#include "qmockimagecapture.h"
#include "qmocksurfacecapture.h"
#include <private/qplatformmediacapture_p.h>

QT_BEGIN_NAMESPACE

class QMockMediaCaptureSession : public QPlatformMediaCaptureSession
{
public:
    QMockMediaCaptureSession()
        : hasControls(true)
    {
    }
    ~QMockMediaCaptureSession()
    {
    }

    QPlatformCamera *camera() override { return hasControls ? mockCameraControl : nullptr; }

    void setCamera(QPlatformCamera *camera) override
    {
        QMockCamera *control = static_cast<QMockCamera *>(camera);
        if (mockCameraControl == control)
            return;

        mockCameraControl = control;
    }

    void setImageCapture(QPlatformImageCapture *imageCapture) override
    {
        mockImageCapture = imageCapture;
    }
    QPlatformImageCapture *imageCapture() override { return hasControls ? mockImageCapture : nullptr; }

    QPlatformMediaRecorder *mediaRecorder() override { return hasControls ? mockControl : nullptr; }
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override
    {
        if (!hasControls) {
            mockControl = nullptr;
            return;
        }
        QMockMediaEncoder *control = static_cast<QMockMediaEncoder *>(recorder);
        if (mockControl == control)
            return;

        mockControl = control;
    }

    void setVideoPreview(QVideoSink *) override {}

    void setAudioInput(QPlatformAudioInput *input) override
    {
        m_audioInput = input;
    }

    QPlatformSurfaceCapture *screenCapture() override { return m_screenCapture; }
    void setScreenCapture(QPlatformSurfaceCapture *capture) override { m_screenCapture = capture; }

    QMockCamera *mockCameraControl = nullptr;
    QPlatformImageCapture *mockImageCapture = nullptr;
    QMockMediaEncoder *mockControl = nullptr;
    QPlatformAudioInput *m_audioInput = nullptr;
    QPlatformSurfaceCapture *m_screenCapture = nullptr;
    bool hasControls;
};

QT_END_NAMESPACE

#endif // QMOCKMEDIACAPTURESESSION_H
