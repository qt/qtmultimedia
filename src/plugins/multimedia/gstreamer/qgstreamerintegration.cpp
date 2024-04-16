// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qgstreamerintegration_p.h>
#include <qgstreamerformatinfo_p.h>
#include <qgstreamervideodevices_p.h>
#include <audio/qgstreameraudiodecoder_p.h>
#include <common/qgstreameraudioinput_p.h>
#include <common/qgstreameraudiooutput_p.h>
#include <common/qgstreamermediaplayer_p.h>
#include <common/qgstreamervideosink_p.h>
#include <mediacapture/qgstreamercamera_p.h>
#include <mediacapture/qgstreamerimagecapture_p.h>
#include <mediacapture/qgstreamermediacapture_p.h>
#include <mediacapture/qgstreamermediaencoder_p.h>

#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcGstreamer, "qt.multimedia.gstreamer")

QGstreamerIntegration::QGstreamerIntegration()
    : QPlatformMediaIntegration(QLatin1String("gstreamer"))
{
    gst_init(nullptr, nullptr);
    qCDebug(lcGstreamer) << "Using gstreamer version: " << gst_version_string();

    if constexpr (!GST_CHECK_VERSION(1, 22, 0)) {
        GstRegistry* reg = gst_registry_get();
        const auto pluginNames = {
            "vaapidecodebin",
            "vaapih264dec",
            "vaapih264enc",
            "vaapih265dec",
            "vaapijpegdec",
            "vaapijpegenc",
            "vaapimpeg2dec",
            "vaapipostproc",
            "vaapisink",
            "vaapivp8dec",
            "vaapivp9dec"
        };

        for (auto name : pluginNames) {
            QGstPluginFeatureHandle pluginFeature {
                gst_registry_lookup_feature(reg, name),
                QGstPluginFeatureHandle::HasRef,
            };
            if (pluginFeature) {
                gst_plugin_feature_set_rank(pluginFeature.get(), GST_RANK_PRIMARY - 1);
            }
        }
    }
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
