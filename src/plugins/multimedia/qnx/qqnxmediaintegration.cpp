// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxmediaintegration_p.h"
#include "qqnxmediacapturesession_p.h"
#include "qqnxmediarecorder_p.h"
#include "qqnxformatinfo_p.h"
#include "qqnxvideodevices_p.h"
#include "qqnxvideosink_p.h"
#include "qqnxmediaplayer_p.h"
#include "qqnximagecapture_p.h"
#include "qqnxplatformcamera_p.h"
#include <QtMultimedia/private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QQnxMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "qnx.json")

public:
    QQnxMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("qnx"))
            return new QQnxMediaIntegration;
        return nullptr;
    }
};

QQnxMediaIntegration::QQnxMediaIntegration()
{
    m_videoDevices = new QQnxVideoDevices(this);
}

QQnxMediaIntegration::~QQnxMediaIntegration()
{
    delete m_formatInfo;
}

QPlatformMediaFormatInfo *QQnxMediaIntegration::formatInfo()
{
    if (!m_formatInfo)
        m_formatInfo = new QQnxFormatInfo();
    return m_formatInfo;
}

QPlatformVideoSink *QQnxMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QQnxVideoSink(sink);
}

QPlatformMediaPlayer *QQnxMediaIntegration::createPlayer(QMediaPlayer *parent)
{
    return new QQnxMediaPlayer(parent);
}

QPlatformMediaCaptureSession *QQnxMediaIntegration::createCaptureSession()
{
    return new QQnxMediaCaptureSession();
}

QPlatformMediaRecorder *QQnxMediaIntegration::createRecorder(QMediaRecorder *parent)
{
    return new QQnxMediaRecorder(parent);
}

QPlatformCamera *QQnxMediaIntegration::createCamera(QCamera *parent)
{
    return new QQnxPlatformCamera(parent);
}

QPlatformImageCapture *QQnxMediaIntegration::createImageCapture(QImageCapture *parent)
{
    return new QQnxImageCapture(parent);
}

QT_END_NAMESPACE

#include "qqnxmediaintegration.moc"
