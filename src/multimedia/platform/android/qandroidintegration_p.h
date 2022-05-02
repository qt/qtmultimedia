/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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

    QPlatformMediaDevices *devices() override;
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

    QAndroidMediaDevices *m_devices = nullptr;
    QPlatformMediaFormatInfo  *m_formatInfo = nullptr;
};

QT_END_NAMESPACE

#endif
