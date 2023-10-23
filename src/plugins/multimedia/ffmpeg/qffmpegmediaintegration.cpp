// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#include <qcameradevice.h>
#include "qffmpegmediaintegration_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediaplayer_p.h"
#include "qffmpegvideosink_p.h"
#include "qffmpegmediacapturesession_p.h"
#include "qffmpegmediarecorder_p.h"
#include "qffmpegimagecapture_p.h"
#include "qffmpegaudioinput_p.h"
#include "qffmpegaudiodecoder_p.h"
#include "qffmpegsymbolsresolve_p.h"
#include "qgrabwindowsurfacecapture_p.h"

#ifdef Q_OS_MACOS
#include <VideoToolbox/VideoToolbox.h>

#include "qcgcapturablewindows_p.h"
#include "qcgwindowcapture_p.h"
#include "qavfscreencapture_p.h"
#endif

#ifdef Q_OS_DARWIN
#include "qavfcamera_p.h"

#elif defined(Q_OS_WINDOWS)
#include "qwindowscamera_p.h"
#include "qwindowsvideodevices_p.h"
#include "qffmpegscreencapture_dxgi_p.h"
#include "qwincapturablewindows_p.h"
#include "qgdiwindowcapture_p.h"
#endif

#ifdef Q_OS_ANDROID
#    include "jni.h"
#    include "qandroidvideodevices_p.h"
#    include "qandroidcamera_p.h"
#    include "qandroidimagecapture_p.h"
extern "C" {
#  include <libavutil/log.h>
#  include <libavcodec/jni.h>
}
#endif

#if QT_CONFIG(linux_v4l)
#include "qv4l2camera_p.h"
#include "qv4l2cameradevices_p.h"
#endif

#if QT_CONFIG(cpp_winrt)
#include "qffmpegwindowcapture_uwp_p.h"
#endif

#if QT_CONFIG(xlib)
#include "qx11surfacecapture_p.h"
#include "qx11capturablewindows_p.h"
#endif

QT_BEGIN_NAMESPACE

class QFFmpegMediaPlugin : public QPlatformMediaPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformMediaPlugin_iid FILE "ffmpeg.json")

public:
    QFFmpegMediaPlugin()
      : QPlatformMediaPlugin()
    {}

    QPlatformMediaIntegration* create(const QString &name) override
    {
        if (name == QLatin1String("ffmpeg"))
            return new QFFmpegMediaIntegration;
        return nullptr;
    }
};

bool thread_local FFmpegLogsEnabledInThread = true;
static bool UseCustomFFmpegLogger = false;

static void qffmpegLogCallback(void *ptr, int level, const char *fmt, va_list vl)
{
    if (!FFmpegLogsEnabledInThread)
        return;

    if (!UseCustomFFmpegLogger)
        return av_log_default_callback(ptr, level, fmt, vl);

    // filter logs above the chosen level and AV_LOG_QUIET (negative level)
    if (level < 0 || level > av_log_get_level())
        return;

    QString message = QString("FFmpeg log: %1").arg(QString::vasprintf(fmt, vl));
    if (message.endsWith("\n"))
        message.removeLast();

    if (level == AV_LOG_DEBUG || level == AV_LOG_TRACE)
        qDebug() << message;
    else if (level == AV_LOG_VERBOSE || level == AV_LOG_INFO)
        qInfo() << message;
    else if (level == AV_LOG_WARNING)
        qWarning() << message;
    else if (level == AV_LOG_ERROR || level == AV_LOG_FATAL || level == AV_LOG_PANIC)
        qCritical() << message;
}

static void setupFFmpegLogger()
{
    if (qEnvironmentVariableIsSet("QT_FFMPEG_DEBUG")) {
        av_log_set_level(AV_LOG_DEBUG);
        UseCustomFFmpegLogger = true;
    }

    av_log_set_callback(&qffmpegLogCallback);
}

static QPlatformSurfaceCapture *createWindowCaptureByBackend(QString backend) {
    if (backend == QLatin1String("grabwindow"))
        return new QGrabWindowSurfaceCapture(QPlatformSurfaceCapture::WindowSource{});

#if QT_CONFIG(xlib)
    if (backend == QLatin1String("x11"))
        return new QX11SurfaceCapture(QPlatformSurfaceCapture::WindowSource{});
#elif defined(Q_OS_WINDOWS)
    if (backend == QLatin1String("gdi"))
        return new QGdiWindowCapture;
#if QT_CONFIG(cpp_winrt)
    if (backend == QLatin1String("uwp"))
        return new QFFmpegWindowCaptureUwp;
#endif
#elif defined(Q_OS_MACOS)
    if (backend == QLatin1String("cg"))
        return new QCGWindowCapture;
#endif
    return nullptr;
}

QFFmpegMediaIntegration::QFFmpegMediaIntegration()
{
    resolveSymbols();

    setupFFmpegLogger();

#if defined(Q_OS_ANDROID)
    m_videoDevices = std::make_unique<QAndroidVideoDevices>(this);
#elif QT_CONFIG(linux_v4l)
    m_videoDevices = std::make_unique<QV4L2CameraDevices>(this);
#elif defined Q_OS_DARWIN
    m_videoDevices = std::make_unique<QAVFVideoDevices>(this);
#elif defined(Q_OS_WINDOWS)
    m_videoDevices = std::make_unique<QWindowsVideoDevices>(this);
#endif

#if QT_CONFIG(xlib)
    if (QX11SurfaceCapture::isSupported())
        m_capturableWindows = std::make_unique<QX11CapturableWindows>();
#elif defined Q_OS_MACOS
    m_capturableWindows = std::make_unique<QCGCapturableWindows>();
#elif defined(Q_OS_WINDOWS)
    m_capturableWindows = std::make_unique<QWinCapturableWindows>();
#endif

#ifndef QT_NO_DEBUG
    qDebug() << "Available HW decoding frameworks:";
    for (auto type : QFFmpeg::HWAccel::decodingDeviceTypes())
        qDebug() << "    " << av_hwdevice_get_type_name(type);

    qDebug() << "Available HW encoding frameworks:";
    for (auto type : QFFmpeg::HWAccel::encodingDeviceTypes())
        qDebug() << "    " << av_hwdevice_get_type_name(type);
#endif
}

QMaybe<QPlatformAudioDecoder *> QFFmpegMediaIntegration::createAudioDecoder(QAudioDecoder *decoder)
{
    return new QFFmpegAudioDecoder(decoder);
}

QMaybe<QPlatformMediaCaptureSession *> QFFmpegMediaIntegration::createCaptureSession()
{
    return new QFFmpegMediaCaptureSession();
}

QMaybe<QPlatformMediaPlayer *> QFFmpegMediaIntegration::createPlayer(QMediaPlayer *player)
{
    return new QFFmpegMediaPlayer(player);
}

QMaybe<QPlatformCamera *> QFFmpegMediaIntegration::createCamera(QCamera *camera)
{
#ifdef Q_OS_DARWIN
    return new QAVFCamera(camera);
#elif defined(Q_OS_ANDROID)
    return new QAndroidCamera(camera);
#elif QT_CONFIG(linux_v4l)
    return new QV4L2Camera(camera);
#elif defined(Q_OS_WINDOWS)
    return new QWindowsCamera(camera);
#else
    Q_UNUSED(camera);
    return nullptr;//new QFFmpegCamera(camera);
#endif
}

QPlatformSurfaceCapture *QFFmpegMediaIntegration::createScreenCapture(QScreenCapture *)
{
#if QT_CONFIG(xlib)
    if (QX11SurfaceCapture::isSupported())
        return new QX11SurfaceCapture(QPlatformSurfaceCapture::ScreenSource{});
#endif

#if defined(Q_OS_WINDOWS)
    return new QFFmpegScreenCaptureDxgi;
#elif defined(Q_OS_MACOS) // TODO: probably use it for iOS as well
    return new QAVFScreenCapture;
#else
    return new QGrabWindowSurfaceCapture(QPlatformSurfaceCapture::ScreenSource{});
#endif
}

QPlatformSurfaceCapture *QFFmpegMediaIntegration::createWindowCapture(QWindowCapture *)
{
    static const QString windowCaptureBackend = qgetenv("QT_WINDOW_CAPTURE_BACKEND").toLower();

    if (!windowCaptureBackend.isEmpty()) {
        if (auto windowCapture = createWindowCaptureByBackend(windowCaptureBackend))
            return windowCapture;

        qWarning() << "Not supported QT_WINDOW_CAPTURE_BACKEND:" << windowCaptureBackend;
    }

#if QT_CONFIG(xlib)
    if (QX11SurfaceCapture::isSupported())
        return new QX11SurfaceCapture(QPlatformSurfaceCapture::WindowSource{});
#endif

#if defined(Q_OS_WINDOWS)
#  if QT_CONFIG(cpp_winrt)
    if (QFFmpegWindowCaptureUwp::isSupported())
        return new QFFmpegWindowCaptureUwp;
#  endif

    return new QGdiWindowCapture;
#elif defined(Q_OS_MACOS) // TODO: probably use it for iOS as well
    return new QCGWindowCapture;
#else
    return new QGrabWindowSurfaceCapture(QPlatformSurfaceCapture::WindowSource{});
#endif
}

QMaybe<QPlatformMediaRecorder *> QFFmpegMediaIntegration::createRecorder(QMediaRecorder *recorder)
{
    return new QFFmpegMediaRecorder(recorder);
}

QMaybe<QPlatformImageCapture *> QFFmpegMediaIntegration::createImageCapture(QImageCapture *imageCapture)
{
#if defined(Q_OS_ANDROID)
    return new QAndroidImageCapture(imageCapture);
#else
    return new QFFmpegImageCapture(imageCapture);
#endif
}

QMaybe<QPlatformVideoSink *> QFFmpegMediaIntegration::createVideoSink(QVideoSink *sink)
{
    return new QFFmpegVideoSink(sink);
}

QMaybe<QPlatformAudioInput *> QFFmpegMediaIntegration::createAudioInput(QAudioInput *input)
{
    return new QFFmpegAudioInput(input);
}

QPlatformMediaFormatInfo *QFFmpegMediaIntegration::createFormatInfo()
{
    return new QFFmpegMediaFormatInfo;
}

#ifdef Q_OS_ANDROID

Q_DECL_EXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/)
{
    static bool initialized = false;
    if (initialized)
        return JNI_VERSION_1_6;
    initialized = true;

    QT_USE_NAMESPACE
    void *environment;
    if (vm->GetEnv(&environment, JNI_VERSION_1_6))
        return JNI_ERR;

    // setting our javavm into ffmpeg.
    if (av_jni_set_java_vm(vm, nullptr))
        return JNI_ERR;

    if (!QAndroidCamera::registerNativeMethods())
        return JNI_ERR;

    return JNI_VERSION_1_6;
}
#endif

QT_END_NAMESPACE

#include "qffmpegmediaintegration.moc"
