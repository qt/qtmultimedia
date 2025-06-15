// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGMEDIAINTEGRATION_H
#define QFFMPEGMEDIAINTEGRATION_H

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

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

extern bool thread_local FFmpegLogsEnabledInThread;

class QFFmpegMediaFormatInfo;

class QFFmpegMediaIntegration : public QPlatformMediaIntegration
{
public:
    QFFmpegMediaIntegration();

    q23::expected<QPlatformAudioDecoder *, QString> createAudioDecoder(QAudioDecoder *decoder) override;
    q23::expected<std::unique_ptr<QPlatformAudioResampler>, QString>
    createAudioResampler(const QAudioFormat &inputFormat,
                         const QAudioFormat &outputFormat) override;
    q23::expected<QPlatformMediaCaptureSession *, QString> createCaptureSession() override;
    q23::expected<QPlatformMediaPlayer *, QString> createPlayer(QMediaPlayer *player) override;
    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *) override;
    QPlatformSurfaceCapture *createScreenCapture(QScreenCapture *) override;
    QPlatformSurfaceCapture *createWindowCapture(QWindowCapture *) override;
    q23::expected<QPlatformMediaRecorder *, QString> createRecorder(QMediaRecorder *) override;
    q23::expected<QPlatformImageCapture *, QString> createImageCapture(QImageCapture *) override;

    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *sink) override;

    q23::expected<QPlatformAudioInput *, QString> createAudioInput(QAudioInput *input) override;
//    QPlatformAudioOutput *createAudioOutput(QAudioOutput *) override;

    QVideoFrame convertVideoFrame(QVideoFrame &srcFrame,
                                  const QVideoFrameFormat &destFormat) override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;

    QPlatformVideoDevices *createVideoDevices() override;

    QPlatformCapturableWindows *createCapturableWindows() override;
};

QT_END_NAMESPACE

#endif
