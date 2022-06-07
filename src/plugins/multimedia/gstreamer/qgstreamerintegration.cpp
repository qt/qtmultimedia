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

#include <memory>

QT_BEGIN_NAMESPACE

class QGstreamerMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "gstreamer.json")

public:
    QGstreamerMediaPlugin()
        : QPlatformMediaPlugin()
    {}

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
    m_videoDevices = new QGstreamerVideoDevices(this);
    m_formatsInfo = new QGstreamerFormatInfo();
}

QGstreamerIntegration::~QGstreamerIntegration()
{
    delete m_formatsInfo;
}

QPlatformMediaFormatInfo *QGstreamerIntegration::formatInfo()
{
    return m_formatsInfo;
}

const QGstreamerFormatInfo *QGstreamerIntegration::gstFormatsInfo() const
{
    return m_formatsInfo;
}

QPlatformAudioDecoder *QGstreamerIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QGstreamerAudioDecoder(decoder);
}

QPlatformMediaCaptureSession *QGstreamerIntegration::createCaptureSession()
{
    return new QGstreamerMediaCapture();
}

QPlatformMediaPlayer *QGstreamerIntegration::createPlayer(QMediaPlayer *player)
{
    return new QGstreamerMediaPlayer(player);
}

QPlatformCamera *QGstreamerIntegration::createCamera(QCamera *camera)
{
    return new QGstreamerCamera(camera);
}

QPlatformMediaRecorder *QGstreamerIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QGstreamerMediaEncoder(recorder);
}

QPlatformImageCapture *QGstreamerIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QGstreamerImageCapture(imageCapture);
}

QPlatformVideoSink *QGstreamerIntegration::createVideoSink(QVideoSink *sink)
{
    return new QGstreamerVideoSink(sink);
}

QPlatformAudioInput *QGstreamerIntegration::createAudioInput(QAudioInput *q)
{
    return new QGstreamerAudioInput(q);
}

QPlatformAudioOutput *QGstreamerIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QGstreamerAudioOutput(q);
}

GstDevice *QGstreamerIntegration::videoDevice(const QByteArray &id) const
{
    return m_videoDevices ? static_cast<QGstreamerVideoDevices*>(m_videoDevices)->videoDevice(id) : nullptr;
}

QT_END_NAMESPACE

#include "qgstreamerintegration.moc"
