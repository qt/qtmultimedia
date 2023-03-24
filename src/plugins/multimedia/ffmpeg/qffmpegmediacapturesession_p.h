// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGMEDIACAPTURESESSION_H
#define QFFMPEGMEDIACAPTURESESSION_H

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

#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediaintegration_p.h>
#include "qpointer.h"
#include "qiodevice.h"

QT_BEGIN_NAMESPACE

class QFFmpegMediaRecorder;
class QFFmpegImageCapture;
class QVideoFrame;
class QAudioSink;
class QFFmpegAudioInput;
class QAudioBuffer;
class QPlatformVideoSource;

class QFFmpegMediaCaptureSession : public QPlatformMediaCaptureSession
{
    Q_OBJECT

public:
    QFFmpegMediaCaptureSession();
    virtual ~QFFmpegMediaCaptureSession();

    QPlatformCamera *camera() override;
    void setCamera(QPlatformCamera *camera) override;

    QPlatformScreenCapture *screenCapture() override;
    void setScreenCapture(QPlatformScreenCapture *) override;

    QPlatformImageCapture *imageCapture() override;
    void setImageCapture(QPlatformImageCapture *imageCapture) override;

    QPlatformMediaRecorder *mediaRecorder() override;
    void setMediaRecorder(QPlatformMediaRecorder *recorder) override;

    void setAudioInput(QPlatformAudioInput *input) override;
    QPlatformAudioInput *audioInput();

    void setVideoPreview(QVideoSink *sink) override;
    void setAudioOutput(QPlatformAudioOutput *output) override;

public Q_SLOTS:
    void newCameraVideoFrame(const QVideoFrame &frame);
    void newScreenCaptureVideoFrame(const QVideoFrame &frame);
    void updateAudioSink();
    void updateVolume();

private:
    QPlatformCamera *m_camera = nullptr;
    QPlatformScreenCapture *m_screenCapture = nullptr;
    QFFmpegAudioInput *m_audioInput = nullptr;
    QFFmpegImageCapture *m_imageCapture = nullptr;
    QFFmpegMediaRecorder *m_mediaRecorder = nullptr;
    QPlatformAudioOutput *m_audioOutput = nullptr;
    QVideoSink *m_videoSink = nullptr;
    std::unique_ptr<QAudioSink> m_audioSink;
    QPointer<QIODevice> m_audioIODevice;
    qsizetype m_audioBufferSize = 0;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURESERVICE_H
