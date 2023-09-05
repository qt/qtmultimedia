// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSINTEGRATION_H
#define QWINDOWSINTEGRATION_H

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
#include "qwindowsvideodevices_p.h"

QT_BEGIN_NAMESPACE

class QWindowsMediaDevices;
class QWindowsFormatInfo;

class QWindowsMediaIntegration : public QPlatformMediaIntegration
{
public:
    QWindowsMediaIntegration();
    ~QWindowsMediaIntegration();

    QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() override;

    QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *decoder) override;
    QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *parent) override;
    QMaybe<QPlatformCamera *> createCamera(QCamera *camera) override;
    QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *recorder) override;
    QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *imageCapture) override;

    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *sink) override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;
};

QT_END_NAMESPACE

#endif
