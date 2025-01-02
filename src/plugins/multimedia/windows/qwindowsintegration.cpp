// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsintegration_p.h"
#include <private/qwindowsmediadevices_p.h>
#include <qwindowsformatinfo_p.h>
#include <qwindowsmediacapture_p.h>
#include <qwindowsimagecapture_p.h>
#include <qwindowscamera_p.h>
#include <qwindowsmediaencoder_p.h>
#include <mfplayercontrol_p.h>
#include <mfaudiodecodercontrol_p.h>
#include <mfevrvideowindowcontrol_p.h>
#include <private/qplatformmediaplugin_p.h>

QT_BEGIN_NAMESPACE

class QWindowsMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "windows.json")

public:
    QWindowsMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("windows"))
            return new QWindowsMediaIntegration;
        return nullptr;
    }
};

QWindowsMediaIntegration::QWindowsMediaIntegration()
{
    CoInitialize(NULL);
    MFStartup(MF_VERSION);

    m_videoDevices = std::make_unique<QWindowsVideoDevices>(this);
}

QWindowsMediaIntegration::~QWindowsMediaIntegration()
{
    MFShutdown();
    CoUninitialize();
}

QPlatformMediaFormatInfo *QWindowsMediaIntegration::createFormatInfo()
{
    return new QWindowsFormatInfo();
}

QMaybe<QPlatformMediaCaptureSession *> QWindowsMediaIntegration::createCaptureSession()
{
    return new QWindowsMediaCaptureService();
}

QMaybe<QPlatformAudioDecoder *> QWindowsMediaIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new MFAudioDecoderControl(decoder);
}

QMaybe<QPlatformMediaPlayer *> QWindowsMediaIntegration::createPlayer(QMediaPlayer *parent)
{
    return new MFPlayerControl(parent);
}

QMaybe<QPlatformCamera *> QWindowsMediaIntegration::createCamera(QCamera *camera)
{
    return new QWindowsCamera(camera);
}

QMaybe<QPlatformMediaRecorder *> QWindowsMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QWindowsMediaEncoder(recorder);
}

QMaybe<QPlatformImageCapture *> QWindowsMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QWindowsImageCapture(imageCapture);
}

QMaybe<QPlatformVideoSink *> QWindowsMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new MFEvrVideoWindowControl(sink);
}

QT_END_NAMESPACE

#include "qwindowsintegration.moc"
