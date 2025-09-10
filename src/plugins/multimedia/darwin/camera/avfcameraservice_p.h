// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef AVFCAMERASERVICE_H
#define AVFCAMERASERVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qset.h>
#include <private/qplatformmediacapture_p.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);

QT_BEGIN_NAMESPACE
class QPlatformCamera;
class QPlatformMediaRecorder;
class AVFCamera;
class AVFImageCapture;
class AVFCameraSession;
class AVFMediaEncoder;

class AVFCameraService : public QPlatformMediaCaptureSession
{
    Q_OBJECT
public:
    AVFCameraService();
    ~AVFCameraService() override;

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *) override;
    void setAudioOutput(QPlatformAudioOutput *) override;

    void setVideoPreview(QVideoSink *sink) override;

    AVFCameraSession *session() const { return m_session; }
    AVFCamera *avfCameraControl() const { return m_cameraControl; }
    AVFMediaEncoder *recorderControl() const { return m_encoder; }
    AVFImageCapture *avfImageCaptureControl() const { return m_imageCaptureControl; }

    QPlatformAudioInput *audioInput() { return m_audioInput; }
    QPlatformAudioOutput *audioOutput() { return m_audioOutput; }

public Q_SLOTS:
    void audioInputDestroyed() { setAudioInput(nullptr); }
    void audioInputChanged();
    void audioOutputDestroyed() { setAudioOutput(nullptr); }
    void audioOutputChanged();

    void setAudioInputMuted(bool muted);
    void setAudioInputVolume(float volume);
    void setAudioOutputMuted(bool muted);
    void setAudioOutputVolume(float volume);

private:
    QPlatformAudioInput *m_audioInput = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;

    AVFCameraSession *m_session = nullptr;
    AVFCamera *m_cameraControl = nullptr;
    AVFMediaEncoder *m_encoder = nullptr;
    AVFImageCapture *m_imageCaptureControl = nullptr;
};

QT_END_NAMESPACE

#endif
