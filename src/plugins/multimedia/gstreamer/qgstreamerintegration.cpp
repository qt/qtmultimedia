// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamerintegration_p.h"
#include "qgstreamervideodevices_p.h"
#include "qgstreamermediaplayer_p.h"
#include "qgstreamermediacapture_p.h"
#include "qgstreameraudiodecoder_p.h"
#include "qgstreamercamera_p.h"
#include "qgstreamermediaencoder_p.h"
#include "qgstreamerimagecapture_p.h"
#include "qgstreamerformatinfo_p.h"
#include "qgstreamervideosink_p.h"
#include "qgstreameraudioinput_p.h"
#include "qgstreameraudiooutput_p.h"
#include <QtMultimedia/private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "gstreamer.json")

public:
    QGstreamerMediaPlugin() = default;

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("gstreamer"))
            return new QGstreamerIntegration;
        return nullptr;
    }
};

QGstreamerIntegration::QGstreamerIntegration()
{
    gst_init(nullptr, nullptr);
}

QPlatformMediaFormatInfo *QGstreamerIntegration::createFormatInfo()
{
    return new QGstreamerFormatInfo();
}

QPlatformVideoDevices *QGstreamerIntegration::createVideoDevices()
{
    return new QGstreamerVideoDevices(this);
}

const QGstreamerFormatInfo *QGstreamerIntegration::gstFormatsInfo()
{
    return static_cast<const QGstreamerFormatInfo *>(formatInfo());
}

QMaybe<QPlatformAudioDecoder *> QGstreamerIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return QGstreamerAudioDecoder::create(decoder);
}

QMaybe<QPlatformMediaCaptureSession *> QGstreamerIntegration::createCaptureSession()
{
    return QGstreamerMediaCapture::create();
}

QMaybe<QPlatformMediaPlayer *> QGstreamerIntegration::createPlayer(QMediaPlayer *player)
{
    return QGstreamerMediaPlayer::create(player);
}

QMaybe<QPlatformCamera *> QGstreamerIntegration::createCamera(QCamera *camera)
{
    return QGstreamerCamera::create(camera);
}

QMaybe<QPlatformMediaRecorder *> QGstreamerIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QGstreamerMediaEncoder(recorder);
}

QMaybe<QPlatformImageCapture *> QGstreamerIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return QGstreamerImageCapture::create(imageCapture);
}

QMaybe<QPlatformVideoSink *> QGstreamerIntegration::createVideoSink(QVideoSink *sink)
{
    return new QGstreamerVideoSink(sink);
}

QMaybe<QPlatformAudioInput *> QGstreamerIntegration::createAudioInput(QAudioInput *q)
{
    return QGstreamerAudioInput::create(q);
}

QMaybe<QPlatformAudioOutput *> QGstreamerIntegration::createAudioOutput(QAudioOutput *q)
{
    return QGstreamerAudioOutput::create(q);
}

GstDevice *QGstreamerIntegration::videoDevice(const QByteArray &id)
{
    const auto devices = videoDevices();
    return devices ? static_cast<QGstreamerVideoDevices *>(devices)->videoDevice(id) : nullptr;
}

QT_END_NAMESPACE

#include "qgstreamerintegration.moc"
