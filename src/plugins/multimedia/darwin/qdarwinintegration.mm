// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdarwinintegration_p.h"
#include <avfmediaplayer_p.h>
#if !defined(Q_OS_VISIONOS)
#include <avfcameraservice_p.h>
#include <avfcamera_p.h>
#include <QtMultimedia/private/qavfvideodevices_p.h>
#include <avfimagecapture_p.h>
#include <avfmediaencoder_p.h>
#endif
#include <qdarwinformatsinfo_p.h>
#include <avfvideosink_p.h>
#include <avfaudiodecoder_p.h>
#include <VideoToolbox/VideoToolbox.h>
#include <qdebug.h>
#include <private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QDarwinMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "darwin.json")

public:
    QDarwinMediaPlugin() = default;

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == u"darwin")
            return new QDarwinIntegration;
        return nullptr;
    }
};

QDarwinIntegration::QDarwinIntegration() : QPlatformMediaIntegration(QLatin1String("darwin"))
{
#if defined(Q_OS_MACOS) && QT_MACOS_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_11_0)
    if (__builtin_available(macOS 11.0, *))
        VTRegisterSupplementalVideoDecoderIfAvailable(kCMVideoCodecType_VP9);
#endif
}

QPlatformMediaFormatInfo *QDarwinIntegration::createFormatInfo()
{
    return new QDarwinFormatInfo();
}

QPlatformVideoDevices *QDarwinIntegration::createVideoDevices()
{
#if defined(Q_OS_VISIONOS)
    return nullptr;
#else
    return new QAVFVideoDevices(this);
#endif
}

q23::expected<QPlatformAudioDecoder *, QString> QDarwinIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new AVFAudioDecoder(decoder);
}

q23::expected<QPlatformMediaCaptureSession *, QString> QDarwinIntegration::createCaptureSession()
{
#if defined(Q_OS_VISIONOS)
    return q23::unexpected{ notAvailable };
#else
    return new AVFCameraService;
#endif
}

q23::expected<QPlatformMediaPlayer *, QString> QDarwinIntegration::createPlayer(QMediaPlayer *player)
{
    return new AVFMediaPlayer(player);
}

q23::expected<QPlatformCamera *, QString> QDarwinIntegration::createCamera(QCamera *camera)
{
#if defined(Q_OS_VISIONOS)
    Q_UNUSED(camera);
    return q23::unexpected{ notAvailable };
#else
    return new AVFCamera(camera);
#endif
}

q23::expected<QPlatformMediaRecorder *, QString> QDarwinIntegration::createRecorder(QMediaRecorder *recorder)
{
#if defined(Q_OS_VISIONOS)
    Q_UNUSED(recorder);
    return q23::unexpected{ notAvailable };
#else
    return new AVFMediaEncoder(recorder);
#endif
}

q23::expected<QPlatformImageCapture *, QString> QDarwinIntegration::createImageCapture(QImageCapture *imageCapture)
{
#if defined(Q_OS_VISIONOS)
    Q_UNUSED(imageCapture);
    return q23::unexpected{ notAvailable };
#else
    return new AVFImageCapture(imageCapture);
#endif
}

q23::expected<QPlatformVideoSink *, QString> QDarwinIntegration::createVideoSink(QVideoSink *sink)
{
    return new AVFVideoSink(sink);
}

QT_END_NAMESPACE

#include "qdarwinintegration.moc"
