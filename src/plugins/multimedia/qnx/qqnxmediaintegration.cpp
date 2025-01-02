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
    m_videoDevices = std::make_unique<QQnxVideoDevices>(this);
}

QPlatformMediaFormatInfo *QQnxMediaIntegration::createFormatInfo()
{
    return new QQnxFormatInfo;
}

QMaybe<QPlatformVideoSink *> QQnxMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QQnxVideoSink(sink);
}

QMaybe<QPlatformMediaPlayer *> QQnxMediaIntegration::createPlayer(QMediaPlayer *parent)
{
    return new QQnxMediaPlayer(parent);
}

QMaybe<QPlatformMediaCaptureSession *> QQnxMediaIntegration::createCaptureSession()
{
    return new QQnxMediaCaptureSession();
}

QMaybe<QPlatformMediaRecorder *> QQnxMediaIntegration::createRecorder(QMediaRecorder *parent)
{
    return new QQnxMediaRecorder(parent);
}

QMaybe<QPlatformCamera *> QQnxMediaIntegration::createCamera(QCamera *parent)
{
    return new QQnxPlatformCamera(parent);
}

QMaybe<QPlatformImageCapture *> QQnxMediaIntegration::createImageCapture(QImageCapture *parent)
{
    return new QQnxImageCapture(parent);
}

QT_END_NAMESPACE

#include "qqnxmediaintegration.moc"
