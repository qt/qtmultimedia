// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSMEDIACAPTURESESSION_P_H
#define QOHOSMEDIACAPTURESESSION_P_H

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

#include <private/qplatformmediacapture_p.h>

#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

class QOhosCamera;
class QOhosCameraSession;
class QOhosImageCapture;
class QOhosMediaRecorder;

class QOhosMediaCaptureSession : public QPlatformMediaCaptureSession
{
    Q_OBJECT
public:
    QOhosMediaCaptureSession();
    ~QOhosMediaCaptureSession() override;

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;
    QOhosCameraSession *cameraSession() const;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;
    void setAudioOutput(QPlatformAudioOutput *output) override;

    QPlatformAudioInput *audioInput() const { return m_audioInput; }

    void setVideoPreview(QVideoSink *sink) override;
    QVideoSink *videoSink() const { return m_videoSink; }

private:
    QPointer<QOhosCamera> m_camera;
    QPointer<QOhosImageCapture> m_imageCapture;
    QPointer<QOhosMediaRecorder> m_recorder;
    QPointer<QVideoSink> m_videoSink;
    QPlatformAudioInput *m_audioInput{ nullptr };
    QPlatformAudioOutput *m_audioOutput{ nullptr };
};

QT_END_NAMESPACE

#endif // QOHOSMEDIACAPTURESESSION_P_H
