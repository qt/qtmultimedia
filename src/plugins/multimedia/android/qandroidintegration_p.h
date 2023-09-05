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

    QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *decoder) override;
    QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() override;
    QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *player) override;
    QMaybe<QPlatformCamera *> createCamera(QCamera *camera) override;
    QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *recorder) override;
    QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *imageCapture) override;

    QMaybe<QPlatformAudioOutput *> createAudioOutput(QAudioOutput *q) override;
    QMaybe<QPlatformAudioInput *> createAudioInput(QAudioInput *audioInput) override;

    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *) override;
    QList<QCameraDevice> videoInputs() override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;
};

QT_END_NAMESPACE

#endif
