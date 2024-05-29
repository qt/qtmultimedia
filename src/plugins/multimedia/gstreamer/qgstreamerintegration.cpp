// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qgstreamerintegration_p.h>
#include <qgstreamerformatinfo_p.h>
#include <qgstreamervideodevices_p.h>
#include <audio/qgstreameraudiodevice_p.h>
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
#include <QtMultimedia/private/qmediaplayer_p.h>
#include <QtMultimedia/private/qmediacapturesession_p.h>
#include <QtMultimedia/private/qcameradevice_p.h>

QT_BEGIN_NAMESPACE

static thread_local bool inCustomCameraConstruction = false;
static thread_local QGstElement pendingCameraElement{};

QGStreamerPlatformSpecificInterfaceImplementation::
        ~QGStreamerPlatformSpecificInterfaceImplementation() = default;

QAudioDevice QGStreamerPlatformSpecificInterfaceImplementation::makeCustomGStreamerAudioInput(
        const QByteArray &gstreamerPipeline)
{
    return qMakeCustomGStreamerAudioInput(gstreamerPipeline);
}

QAudioDevice QGStreamerPlatformSpecificInterfaceImplementation::makeCustomGStreamerAudioOutput(
        const QByteArray &gstreamerPipeline)
{
    return qMakeCustomGStreamerAudioOutput(gstreamerPipeline);
}

QCamera *QGStreamerPlatformSpecificInterfaceImplementation::makeCustomGStreamerCamera(
        const QByteArray &gstreamerPipeline, QObject *parent)
{
    QCameraDevicePrivate *info = new QCameraDevicePrivate;
    info->id = gstreamerPipeline;
    QCameraDevice device = info->create();

    inCustomCameraConstruction = true;
    auto guard = qScopeGuard([] {
        inCustomCameraConstruction = false;
    });

    return new QCamera(device, parent);
}

QCamera *
QGStreamerPlatformSpecificInterfaceImplementation::makeCustomGStreamerCamera(GstElement *element,
                                                                             QObject *parent)
{
    QCameraDevicePrivate *info = new QCameraDevicePrivate;
    info->id = "Custom Camera from GstElement";
    QCameraDevice device = info->create();

    pendingCameraElement = QGstElement{
        element,
        QGstElement::NeedsRef,
    };

    inCustomCameraConstruction = true;
    auto guard = qScopeGuard([] {
        inCustomCameraConstruction = false;
        Q_ASSERT(!pendingCameraElement);
    });

    return new QCamera(device, parent);
}

GstPipeline *QGStreamerPlatformSpecificInterfaceImplementation::gstPipeline(QMediaPlayer *player)
{
    auto *priv = reinterpret_cast<QMediaPlayerPrivate *>(QMediaPlayerPrivate::get(player));
    if (!priv)
        return nullptr;

    QGstreamerMediaPlayer *gstreamerPlayer = dynamic_cast<QGstreamerMediaPlayer *>(priv->control);
    return gstreamerPlayer ? gstreamerPlayer->pipeline().pipeline() : nullptr;
}

GstPipeline *
QGStreamerPlatformSpecificInterfaceImplementation::gstPipeline(QMediaCaptureSession *session)
{
    auto *priv = QMediaCaptureSessionPrivate::get(session);
    if (!priv)
        return nullptr;

    QGstreamerMediaCapture *gstreamerCapture =
            dynamic_cast<QGstreamerMediaCapture *>(priv->captureSession.get());
    return gstreamerCapture ? gstreamerCapture->pipeline().pipeline() : nullptr;
}

Q_LOGGING_CATEGORY(lcGstreamer, "qt.multimedia.gstreamer")

namespace {

void rankDownPlugin(GstRegistry *reg, const char *name)
{
    QGstPluginFeatureHandle pluginFeature{
        gst_registry_lookup_feature(reg, name),
        QGstPluginFeatureHandle::HasRef,
    };
    if (pluginFeature)
        gst_plugin_feature_set_rank(pluginFeature.get(), GST_RANK_PRIMARY - 1);
}

// https://gstreamer.freedesktop.org/documentation/vaapi/index.html
constexpr auto vaapiPluginNames = {
    "vaapidecodebin", "vaapih264dec", "vaapih264enc",  "vaapih265dec",
    "vaapijpegdec",   "vaapijpegenc", "vaapimpeg2dec", "vaapipostproc",
    "vaapisink",      "vaapivp8dec",  "vaapivp9dec",
};

// https://gstreamer.freedesktop.org/documentation/va/index.html
constexpr auto vaPluginNames = {
    "vaav1dec",  "vacompositor", "vadeinterlace", "vah264dec", "vah264enc", "vah265dec",
    "vajpegdec", "vampeg2dec",   "vapostproc",    "vavp8dec",  "vavp9dec",
};

// https://gstreamer.freedesktop.org/documentation/nvcodec/index.html
constexpr auto nvcodecPluginNames = {
    "cudaconvert",     "cudaconvertscale", "cudadownload",     "cudaipcsink",      "cudaipcsrc",
    "cudascale",       "cudaupload",       "nvautogpuh264enc", "nvautogpuh265enc", "nvav1dec",
    "nvcudah264enc",   "nvcudah265enc",    "nvd3d11h264enc",   "nvd3d11h265enc",   "nvh264dec",
    "nvh264enc",       "nvh265dec",        "nvh265enc",        "nvjpegdec",        "nvjpegenc",
    "nvmpeg2videodec", "nvmpeg4videodec",  "nvmpegvideodec",   "nvvp8dec",         "nvvp9dec",
};

} // namespace

QGstreamerIntegration::QGstreamerIntegration()
    : QPlatformMediaIntegration(QLatin1String("gstreamer"))
{
    gst_init(nullptr, nullptr);
    qCDebug(lcGstreamer) << "Using gstreamer version: " << gst_version_string();

    GstRegistry *reg = gst_registry_get();

    if constexpr (!GST_CHECK_VERSION(1, 22, 0)) {
        GstRegistry* reg = gst_registry_get();
        for (const char *name : vaapiPluginNames)
            rankDownPlugin(reg, name);
    }

    if (qEnvironmentVariableIsSet("QT_GSTREAMER_DISABLE_VA")) {
        for (const char *name : vaPluginNames)
            rankDownPlugin(reg, name);
    }

    if (qEnvironmentVariableIsSet("QT_GSTREAMER_DISABLE_NVCODEC")) {
        for (const char *name : nvcodecPluginNames)
            rankDownPlugin(reg, name);
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
    if (inCustomCameraConstruction) {
        QGstElement element = std::exchange(pendingCameraElement, {});
        return element ? new QGstreamerCustomCamera{ camera, std::move(element) }
                       : new QGstreamerCustomCamera{ camera };
    }

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

QAbstractPlatformSpecificInterface *QGstreamerIntegration::platformSpecificInterface()
{
    return &m_platformSpecificImplementation;
}

QT_END_NAMESPACE
