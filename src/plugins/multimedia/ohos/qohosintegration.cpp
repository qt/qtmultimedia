// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosintegration_p.h"

#include "qohosglobal_p.h"
#include "common/qohosvideosink_p.h"
#include "qohosformatsinfo_p.h"
#include "mediacapture/qohoscamera_p.h"
#include "mediacapture/qohosimagecapture_p.h"
#include "mediacapture/qohosmediacapturesession_p.h"
#include "mediacapture/qohosmediarecorder_p.h"
#include "mediacapture/qohosvideodevices_p.h"
#include "mediaplayer/qohosmediaplayer_p.h"

#include <private/qplatformmediaformatinfo_p.h>
#include <private/qplatformmediaplugin_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcOhosMediaPlugin, "qt.multimedia.ohos")

class QOhosMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "ohos.json")

public:
    QOhosMediaPlugin() : QPlatformMediaPlugin() { }

    QPlatformMediaIntegration *create(const QString &name) override
    {
        if (name == u"ohos")
            return new QOhosIntegration;
        return nullptr;
    }
};

QOhosIntegration::QOhosIntegration() : QPlatformMediaIntegration(QLatin1String("ohos"))
{
}

QOhosIntegration::~QOhosIntegration() = default;

QPlatformMediaFormatInfo *QOhosIntegration::createFormatInfo()
{
    return new QOhosFormatsInfo;
}

q23::expected<QPlatformMediaPlayer *, QString>
QOhosIntegration::createPlayer(QMediaPlayer *player)
{
    return new QOhosMediaPlayer(player);
}

q23::expected<QPlatformVideoSink *, QString>
QOhosIntegration::createVideoSink(QVideoSink *sink)
{
    return new QOhosVideoSink(sink);
}

q23::expected<QPlatformCamera *, QString> QOhosIntegration::createCamera(QCamera *camera)
{
    return new QOhosCamera(camera);
}

q23::expected<QPlatformImageCapture *, QString>
QOhosIntegration::createImageCapture(QImageCapture *imageCapture)
{
    return new QOhosImageCapture(imageCapture);
}

q23::expected<QPlatformMediaRecorder *, QString>
QOhosIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QOhosMediaRecorder(recorder);
}

q23::expected<QPlatformMediaCaptureSession *, QString> QOhosIntegration::createCaptureSession()
{
    return new QOhosMediaCaptureSession;
}

QPlatformVideoDevices *QOhosIntegration::createVideoDevices()
{
    return new QOhosVideoDevices(this);
}

QT_END_NAMESPACE

#include "qohosintegration.moc"
