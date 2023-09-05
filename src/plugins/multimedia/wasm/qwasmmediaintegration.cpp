// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediaintegration_p.h"
#include <QLoggingCategory>

#include <QCamera>
#include <QCameraDevice>

#include <private/qplatformmediaformatinfo_p.h>
#include <private/qplatformmediaplugin_p.h>
#include <private/qplatformmediadevices_p.h>
#include <private/qplatformvideodevices_p.h>

#include "mediaplayer/qwasmmediaplayer_p.h"
#include "mediaplayer/qwasmvideosink_p.h"
#include "qwasmaudioinput_p.h"
#include "common/qwasmaudiooutput_p.h"

#include "mediacapture/qwasmmediacapturesession_p.h"
#include "mediacapture/qwasmmediarecorder_p.h"
#include "mediacapture/qwasmcamera_p.h"
#include "mediacapture/qwasmmediacapturesession_p.h"
#include "mediacapture/qwasmimagecapture_p.h"

QT_BEGIN_NAMESPACE


static Q_LOGGING_CATEGORY(qtWasmMediaPlugin, "qt.multimedia.wasm")

class QWasmMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "wasm.json")

public:
    QWasmMediaPlugin()
        : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration *create(const QString &name) override
    {
        if (name == QLatin1String("wasm"))
            return new QWasmMediaIntegration;
        return nullptr;
    }
};

QWasmMediaIntegration::QWasmMediaIntegration()
{
   m_videoDevices = std::make_unique<QWasmCameraDevices>(this);
}

QMaybe<QPlatformMediaPlayer *> QWasmMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return new QWasmMediaPlayer(player);
}

QMaybe<QPlatformVideoSink *> QWasmMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QWasmVideoSink(sink);
}

QMaybe<QPlatformAudioInput *> QWasmMediaIntegration::createAudioInput(QAudioInput *audioInput)
{
    return new QWasmAudioInput(audioInput);
}

QMaybe<QPlatformAudioOutput *> QWasmMediaIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QWasmAudioOutput(q);
}

QPlatformMediaFormatInfo *QWasmMediaIntegration::createFormatInfo()
{
    // TODO: create custom implementation
    return new QPlatformMediaFormatInfo;
}

QMaybe<QPlatformMediaCaptureSession *> QWasmMediaIntegration::createCaptureSession()
{
    return new QWasmMediaCaptureSession();
}

QMaybe<QPlatformMediaRecorder *> QWasmMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QWasmMediaRecorder(recorder);
}

QMaybe<QPlatformCamera *> QWasmMediaIntegration::createCamera(QCamera *camera)
{
    return new QWasmCamera(camera);
}

QMaybe<QPlatformImageCapture *>
QWasmMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QWasmImageCapture(imageCapture);
}

QList<QCameraDevice> QWasmMediaIntegration::videoInputs()
{
    return m_videoDevices->videoDevices();
}

QT_END_NAMESPACE

#include "qwasmmediaintegration.moc"
