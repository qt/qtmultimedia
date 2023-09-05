// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERINTEGRATION_H
#define QGSTREAMERINTEGRATION_H

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

class QFFmpegMediaFormatInfo;

class QFFmpegMediaIntegration : public QPlatformMediaIntegration
{
public:
    QFFmpegMediaIntegration();

    static QFFmpegMediaIntegration *instance()
    {
        return static_cast<QFFmpegMediaIntegration *>(QPlatformMediaIntegration::instance());
    }

    QMaybe<QPlatformAudioDecoder *> createAudioDecoder(QAudioDecoder *decoder) override;
    QMaybe<QPlatformMediaCaptureSession *> createCaptureSession() override;
    QMaybe<QPlatformMediaPlayer *> createPlayer(QMediaPlayer *player) override;
    QMaybe<QPlatformCamera *> createCamera(QCamera *) override;
    QPlatformSurfaceCapture *createScreenCapture(QScreenCapture *) override;
    QPlatformSurfaceCapture *createWindowCapture(QWindowCapture *) override;
    QMaybe<QPlatformMediaRecorder *> createRecorder(QMediaRecorder *) override;
    QMaybe<QPlatformImageCapture *> createImageCapture(QImageCapture *) override;

    QMaybe<QPlatformVideoSink *> createVideoSink(QVideoSink *sink) override;

    QMaybe<QPlatformAudioInput *> createAudioInput(QAudioInput *input) override;
//    QPlatformAudioOutput *createAudioOutput(QAudioOutput *) override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;
};

QT_END_NAMESPACE

#endif
