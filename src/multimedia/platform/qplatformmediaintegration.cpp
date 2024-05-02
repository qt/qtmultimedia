// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qtmultimediaglobal_p.h>
#include "qplatformmediaintegration_p.h"
#include <qatomic.h>
#include <qmutex.h>
#include <qplatformaudioinput_p.h>
#include <qplatformaudiooutput_p.h>
#include <qplatformaudioresampler_p.h>
#include <qplatformvideodevices_p.h>
#include <qmediadevices.h>
#include <qcameradevice.h>
#include <qloggingcategory.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qapplicationstatic.h>

#include "qplatformcapturablewindows_p.h"
#include "qplatformmediadevices_p.h"
#include <QtCore/private/qfactoryloader_p.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <private/qplatformmediaformatinfo_p.h>
#include "qplatformmediaplugin_p.h"

namespace {

class QFallbackIntegration : public QPlatformMediaIntegration
{
public:
    QFallbackIntegration() : QPlatformMediaIntegration(QLatin1String("fallback"))
    {
        qWarning("No QtMultimedia backends found. Only QMediaDevices, QAudioDevice, QSoundEffect, QAudioSink, and QAudioSource are available.");
    }
};

static Q_LOGGING_CATEGORY(qLcMediaPlugin, "qt.multimedia.plugin")

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
                          (QPlatformMediaPlugin_iid,
                           QLatin1String("/multimedia")))

static const auto FFmpegBackend = QStringLiteral("ffmpeg");

static QString defaultBackend(const QStringList &backends)
{
#ifdef QT_DEFAULT_MEDIA_BACKEND
    auto backend = QString::fromUtf8(QT_DEFAULT_MEDIA_BACKEND);
    if (backends.contains(backend))
        return backend;
#endif

#if defined(Q_OS_DARWIN) || defined(Q_OS_LINUX) || defined(Q_OS_WINDOWS) || defined(Q_OS_ANDROID)
    // Return ffmpeg backend by default.
    // Platform backends for the OS list are optionally available but have limited support.
    if (backends.contains(FFmpegBackend))
        return FFmpegBackend;
#else
    // Return platform backend (non-ffmpeg) by default.
    if (backends.size() > 1 && backends[0] == FFmpegBackend)
        return backends[1];
#endif

    return backends[0];
}

struct InstanceHolder
{
    InstanceHolder()
    {
        if (!QCoreApplication::instance())
            qCCritical(qLcMediaPlugin()) << "Qt Multimedia requires a QCoreApplication instance";

        const QStringList backends = QPlatformMediaIntegration::availableBackends();
        QString backend = QString::fromUtf8(qgetenv("QT_MEDIA_BACKEND"));
        if (backend.isEmpty() && !backends.isEmpty())
            backend = defaultBackend(backends);

        qCDebug(qLcMediaPlugin) << "Loading media backend" << backend;
        instance.reset(
                qLoadPlugin<QPlatformMediaIntegration, QPlatformMediaPlugin>(loader(), backend));

        if (!instance) {
            // No backends found. Use fallback to support basic functionality
            instance = std::make_unique<QFallbackIntegration>();
        }
    }

    ~InstanceHolder()
    {
        instance.reset();
        qCDebug(qLcMediaPlugin) << "Released media backend";
    }

    // Play nice with QtGlobalStatic::ApplicationHolder
    using QAS_Type = InstanceHolder;
    static void innerFunction(void *pointer)
    {
        new (pointer) InstanceHolder();
    }

    std::unique_ptr<QPlatformMediaIntegration> instance;
};

// Specialized implementation of Q_APPLICATION_STATIC which behaves as
// an application static if a Qt application is present, otherwise as a Q_GLOBAL_STATIC.
// By doing this, and we have a Qt application, all system resources allocated by the
// backend is released when application lifetime ends. This is important on Windows,
// where Windows Media Foundation instances should not be released during static destruction.
//
// If we don't have a Qt application available when instantiating the instance holder,
// it will be created once, and not destroyed until static destruction. This can cause
// abrupt termination of Windows applications during static destruction. This is not a
// supported use case, but we keep this as a fallback to keep old applications functional.
// See also QTBUG-120198
struct ApplicationHolder : QtGlobalStatic::ApplicationHolder<InstanceHolder>
{
    // Replace QtGlobalStatic::ApplicationHolder::pointer to prevent crash if
    // no application is present
    static InstanceHolder* pointer()
    {
        if (guard.loadAcquire() == QtGlobalStatic::Initialized)
            return realPointer();

        QMutexLocker locker(&mutex);
        if (guard.loadRelaxed() == QtGlobalStatic::Uninitialized) {
            InstanceHolder::innerFunction(&storage);

            if (const QCoreApplication *app = QCoreApplication::instance())
                QObject::connect(app, &QObject::destroyed, app, reset, Qt::DirectConnection);

            guard.storeRelease(QtGlobalStatic::Initialized);
        }
        return realPointer();
    }
};

} // namespace

QT_BEGIN_NAMESPACE

QPlatformMediaIntegration *QPlatformMediaIntegration::instance()
{
    static QGlobalStatic<ApplicationHolder> s_instanceHolder;
    return s_instanceHolder->instance.get();
}

QList<QCameraDevice> QPlatformMediaIntegration::videoInputs()
{
    auto devices = videoDevices();
    return devices ? devices->videoDevices() : QList<QCameraDevice>{};
}

QMaybe<std::unique_ptr<QPlatformAudioResampler>>
QPlatformMediaIntegration::createAudioResampler(const QAudioFormat &, const QAudioFormat &)
{
    return notAvailable;
}

QMaybe<QPlatformAudioInput *> QPlatformMediaIntegration::createAudioInput(QAudioInput *q)
{
    return new QPlatformAudioInput(q);
}

QMaybe<QPlatformAudioOutput *> QPlatformMediaIntegration::createAudioOutput(QAudioOutput *q)
{
    return new QPlatformAudioOutput(q);
}

QList<QCapturableWindow> QPlatformMediaIntegration::capturableWindowsList()
{
    const auto capturableWindows = this->capturableWindows();
    return capturableWindows ? capturableWindows->windows() : QList<QCapturableWindow>{};
}

bool QPlatformMediaIntegration::isCapturableWindowValid(const QCapturableWindowPrivate &window)
{
    const auto capturableWindows = this->capturableWindows();
    return capturableWindows && capturableWindows->isWindowValid(window);
}

const QPlatformMediaFormatInfo *QPlatformMediaIntegration::formatInfo()
{
    std::call_once(m_formatInfoOnceFlg, [this]() {
        m_formatInfo.reset(createFormatInfo());
        Q_ASSERT(m_formatInfo);
    });
    return m_formatInfo.get();
}

QPlatformMediaFormatInfo *QPlatformMediaIntegration::createFormatInfo()
{
    return new QPlatformMediaFormatInfo;
}

std::unique_ptr<QPlatformMediaDevices> QPlatformMediaIntegration::createMediaDevices()
{
    return QPlatformMediaDevices::create();
}

// clang-format off
QPlatformVideoDevices *QPlatformMediaIntegration::videoDevices()
{
    std::call_once(m_videoDevicesOnceFlag,
                   [this]() {
                       m_videoDevices.reset(createVideoDevices());
                   });
    return m_videoDevices.get();
}

QPlatformCapturableWindows *QPlatformMediaIntegration::capturableWindows()
{
    std::call_once(m_capturableWindowsOnceFlag,
                   [this]() {
                       m_capturableWindows.reset(createCapturableWindows());
                   });
    return m_capturableWindows.get();
}

QPlatformMediaDevices *QPlatformMediaIntegration::mediaDevices()
{
    std::call_once(m_mediaDevicesOnceFlag, [this] {
        m_mediaDevices = createMediaDevices();
    });
    return m_mediaDevices.get();
}

// clang-format on

QStringList QPlatformMediaIntegration::availableBackends()
{
    QStringList list;

    if (QFactoryLoader *fl = loader()) {
        const auto keyMap = fl->keyMap();
        for (auto it = keyMap.constBegin(); it != keyMap.constEnd(); ++it)
            if (!list.contains(it.value()))
                list << it.value();
    }

    qCDebug(qLcMediaPlugin) << "Available backends" << list;
    return list;
}

QLatin1String QPlatformMediaIntegration::name()
{
    return m_backendName;
}

QVideoFrame QPlatformMediaIntegration::convertVideoFrame(QVideoFrame &,
                                                         const QVideoFrameFormat &)
{
    return {};
}

QPlatformMediaIntegration::QPlatformMediaIntegration(QLatin1String name) : m_backendName(name) { }

QPlatformMediaIntegration::~QPlatformMediaIntegration() = default;

QT_END_NAMESPACE

#include "moc_qplatformmediaintegration_p.cpp"
