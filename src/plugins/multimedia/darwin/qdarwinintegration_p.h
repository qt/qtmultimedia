// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QDARWININTEGRATION_H
#define QDARWININTEGRATION_H

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

Q_FORWARD_DECLARE_OBJC_CLASS(NSObject);

QT_BEGIN_NAMESPACE

class QDarwinIntegration : public QPlatformMediaIntegration
{
public:
    QDarwinIntegration();

    q23::expected<QPlatformAudioDecoder *, QString> createAudioDecoder(QAudioDecoder *) override;
    q23::expected<QPlatformMediaCaptureSession *, QString> createCaptureSession() override;
    q23::expected<QPlatformMediaPlayer *, QString> createPlayer(QMediaPlayer *player) override;
    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *camera) override;
    q23::expected<QPlatformMediaRecorder *, QString> createRecorder(QMediaRecorder *) override;
    q23::expected<QPlatformImageCapture *, QString> createImageCapture(QImageCapture *) override;

    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *) override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;
    QPlatformVideoDevices *createVideoDevices() override;
};

QT_END_NAMESPACE

#endif
