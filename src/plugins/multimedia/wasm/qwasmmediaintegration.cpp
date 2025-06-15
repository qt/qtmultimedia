// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmmediaintegration_p.h"
#include <QLoggingCategory>

#include <QCamera>
#include <QCameraDevice>

#include <private/qplatformmediaformatinfo_p.h>
#include <private/qplatformmediaplugin_p.h>
#include <private/qplatformaudiodevices_p.h>
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
        if (name == u"wasm")
            return new QWasmMediaIntegration;
        return nullptr;
    }
};

QWasmMediaIntegration::QWasmMediaIntegration()
    : QPlatformMediaIntegration(QLatin1String("wasm")) { }

q23::expected<QPlatformMediaPlayer *, QString> QWasmMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return new QWasmMediaPlayer(player);
}

q23::expected<QPlatformVideoSink *, QString> QWasmMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QWasmVideoSink(sink);
}

q23::expected<QPlatformAudioInput *, QString> QWasmMediaIntegration::createAudioInput(QAudioInput *audioInput)
{
    return new QWasmAudioInput(audioInput);
}

q23::expected<QPlatformAudioOutput *, QString> QWasmMediaIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QWasmAudioOutput(q);
}

QPlatformMediaFormatInfo *QWasmMediaIntegration::createFormatInfo()
{
    // TODO: create custom implementation
    return new QPlatformMediaFormatInfo;
}

QPlatformVideoDevices *QWasmMediaIntegration::createVideoDevices()
{
    return new QWasmCameraDevices(this);
}

q23::expected<QPlatformMediaCaptureSession *, QString> QWasmMediaIntegration::createCaptureSession()
{
    return new QWasmMediaCaptureSession();
}

q23::expected<QPlatformMediaRecorder *, QString> QWasmMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QWasmMediaRecorder(recorder);
}

q23::expected<QPlatformCamera *, QString> QWasmMediaIntegration::createCamera(QCamera *camera)
{
    return new QWasmCamera(camera);
}

q23::expected<QPlatformImageCapture *, QString>
QWasmMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QWasmImageCapture(imageCapture);
}

QT_END_NAMESPACE

#include "qwasmmediaintegration.moc"
