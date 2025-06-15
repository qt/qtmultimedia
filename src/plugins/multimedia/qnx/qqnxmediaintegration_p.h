// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQnxMediaIntegration_H
#define QQnxMediaIntegration_H

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

class QQnxPlayerInterface;
class QQnxFormatInfo;

class QQnxMediaIntegration : public QPlatformMediaIntegration
{
public:
    QQnxMediaIntegration();

    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *sink) override;

    q23::expected<QPlatformMediaPlayer *, QString> createPlayer(QMediaPlayer *parent) override;

    q23::expected<QPlatformMediaCaptureSession *, QString> createCaptureSession() override;

    q23::expected<QPlatformMediaRecorder *, QString> createRecorder(QMediaRecorder *parent) override;

    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *parent) override;

    q23::expected<QPlatformImageCapture *, QString> createImageCapture(QImageCapture *parent) override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;

    QPlatformVideoDevices *createVideoDevices() override;
};

QT_END_NAMESPACE

#endif
