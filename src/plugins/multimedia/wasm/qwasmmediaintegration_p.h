// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMMEDIAINTEGRATION_H
#define QWASMMEDIAINTEGRATION_H

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

#include <private/qwasmmediadevices_p.h>

QT_BEGIN_NAMESPACE

class QWasmMediaDevices;

class QWasmMediaIntegration : public QPlatformMediaIntegration
{
public:
    QWasmMediaIntegration();

    QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *player) override;
    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *sink) override;

    QMaybe<QPlatformAudioInput *> createAudioInput(QAudioInput *audioInput) override;
    QMaybe<QPlatformAudioOutput *> createAudioOutput(QAudioOutput *q) override;

    QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() override;
    QMaybe<QPlatformCamera *> createCamera(QCamera *camera) override;
    QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *recorder) override;
    QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *imageCapture) override;
    QList<QCameraDevice> videoInputs() override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;
};

QT_END_NAMESPACE

#endif // QWASMMEDIAINTEGRATION_H
