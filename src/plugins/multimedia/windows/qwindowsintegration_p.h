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

#include <QtMultimedia/private/qcominitializer_p.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qwindowsvideodevices_p.h>

QT_BEGIN_NAMESPACE

class QWindowsAudioDevices;
class QWindowsFormatInfo;

class QWindowsMediaIntegration : public QPlatformMediaIntegration
{
    Q_OBJECT
public:
    QWindowsMediaIntegration();
    ~QWindowsMediaIntegration();

    q23::expected<QPlatformMediaCaptureSession *, QString> createCaptureSession() override;

    q23::expected<QPlatformAudioDecoder *, QString> createAudioDecoder(QAudioDecoder *decoder) override;
    q23::expected<QPlatformMediaPlayer *, QString> createPlayer(QMediaPlayer *parent) override;
    q23::expected<QPlatformCamera *, QString> createCamera(QCamera *camera) override;
    q23::expected<QPlatformMediaRecorder *, QString> createRecorder(QMediaRecorder *recorder) override;
    q23::expected<QPlatformImageCapture *, QString> createImageCapture(QImageCapture *imageCapture) override;

    q23::expected<QPlatformVideoSink *, QString> createVideoSink(QVideoSink *sink) override;

protected:
    QPlatformMediaFormatInfo *createFormatInfo() override;

    QPlatformVideoDevices *createVideoDevices() override;

private:
    QComInitializer m_comInitializer;
};

QT_END_NAMESPACE

#endif
