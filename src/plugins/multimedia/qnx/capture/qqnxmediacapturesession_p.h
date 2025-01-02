// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QQNXMEDIACAPTURESESSION_H
#define QQNXMEDIACAPTURESESSION_H

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

#include <QObject>

#include <private/qplatformmediacapture_p.h>

QT_BEGIN_NAMESPACE

class QQnxAudioInput;
class QQnxPlatformCamera;
class QQnxImageCapture;
class QQnxMediaRecorder;
class QQnxVideoSink;

class QQnxMediaCaptureSession : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    QQnxMediaCaptureSession();
    ~QQnxMediaCaptureSession();

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *mediaRecorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;

    void setVideoPreview(QVideoSink *sink) override;

    void setAudioOutput(QPlatformAudioOutput *output) override;

    QQnxAudioInput *audioInput() const;

    QQnxVideoSink *videoSink() const;

private:
    QQnxPlatformCamera *m_camera = nullptr;
    QQnxImageCapture *m_imageCapture = nullptr;
    QQnxMediaRecorder *m_mediaRecorder = nullptr;
    QQnxAudioInput *m_audioInput = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;
    QQnxVideoSink *m_videoSink = nullptr;
};

QT_END_NAMESPACE

#endif
