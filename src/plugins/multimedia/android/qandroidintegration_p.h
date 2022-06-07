// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDINTEGRATION_H
#define QANDROIDINTEGRATION_H

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

#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

class QAndroidMediaDevices;

class QAndroidIntegration : public QPlatformMediaIntegration
{
public:
    QAndroidIntegration();
    ~QAndroidIntegration();

    QPlatformMediaFormatInfo *formatInfo() override;

    QPlatformAudioDecoder *createAudioDecoder(QAudioDecoder *decoder) override;
    QPlatformMediaCaptureSession *createCaptureSession() override;
    QPlatformMediaPlayer *createPlayer(QMediaPlayer *player) override;
    QPlatformCamera *createCamera(QCamera *camera) override;
    QPlatformMediaRecorder *createRecorder(QMediaRecorder *recorder) override;
    QPlatformImageCapture *createImageCapture(QImageCapture *imageCapture) override;

    QPlatformAudioOutput *createAudioOutput(QAudioOutput *q) override;
    QPlatformAudioInput *createAudioInput(QAudioInput *audioInput) override;

    QPlatformVideoSink *createVideoSink(QVideoSink *) override;
    QList<QCameraDevice> videoInputs() override;

    QPlatformMediaFormatInfo  *m_formatInfo = nullptr;
};

QT_END_NAMESPACE

#endif
