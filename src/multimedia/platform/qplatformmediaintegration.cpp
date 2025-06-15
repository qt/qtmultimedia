// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformmediaintegration_p.h"

#include <QtCore/qapplicationstatic.h>
#include <QtCore/qatomic.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qfactoryloader_p.h>

#include <QtGui/qwindow.h>

#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qplatformaudiodevices_p.h>
#include <QtMultimedia/private/qplatformaudioinput_p.h>
#include <QtMultimedia/private/qplatformaudiooutput_p.h>
#include <QtMultimedia/private/qplatformaudioresampler_p.h>
#include <QtMultimedia/private/qplatformcapturablewindows_p.h>
#include <QtMultimedia/private/qplatformmediaformatinfo_p.h>
#include <QtMultimedia/private/qplatformmediaplugin_p.h>
#include <QtMultimedia/private/qplatformvideodevices_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

namespace {

class QFallbackIntegration : public QPlatformMediaIntegration
{
public:
    QFallbackIntegration() : QPlatformMediaIntegration(QLatin1String("fallback"))
    {
        qWarning("No QtMultimedia backends found. Only QMediaDevices, QAudioDevice, QSoundEffect, QAudioSink, and QAudioSource are available.");
    }
};

Q_STATIC_LOGGING_CATEGORY(qLcMediaPlugin, "qt.multimedia.plugin")

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
        init();
    }

    void init()
    {
        if (!QCoreApplication::instance())
            qCCritical(qLcMediaPlugin()) << "Qt Multimedia requires a QCoreApplication instance";

        const QStringList backends = QPlatformMediaIntegration::availableBackends();
        QString backend = QString::fromUtf8(qgetenv("QT_MEDIA_BACKEND")).toLower();
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

    std::unique_ptr<QPlatformMediaIntegration> instance;
};

Q_APPLICATION_STATIC(InstanceHolder, s_instanceHolder);

} // namespace

QT_BEGIN_NAMESPACE

QPlatformMediaIntegration *QPlatformMediaIntegration::instance()
{
    return s_instanceHolder->instance.get();
}

void QPlatformMediaIntegration::resetInstance()
{
    s_instanceHolder->init(); // tests only
}

q23::expected<std::unique_ptr<QPlatformAudioResampler>, QString>
QPlatformMediaIntegration::createAudioResampler(const QAudioFormat &, const QAudioFormat &)
{
    return q23::unexpected(notAvailable);
}

q23::expected<QPlatformAudioInput *, QString> QPlatformMediaIntegration::createAudioInput(QAudioInput *q)
{
    return new QPlatformAudioInput(q);
}

q23::expected<QPlatformAudioOutput *, QString> QPlatformMediaIntegration::createAudioOutput(QAudioOutput *q)
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

q23::expected<QCapturableWindow, QString> QPlatformMediaIntegration::capturableWindowFromQWindow(QWindow *window)
{
    const auto capturableWindows = this->capturableWindows();
    if (!capturableWindows)
        return q23::unexpected{ QStringLiteral("No windowcapture platform implementation") };
    if (window == nullptr)
        return q23::unexpected{ QStringLiteral("QWindow is nullptr") };
    if (!window->isTopLevel())
        return q23::unexpected{ QStringLiteral("QWindow is not top-level.") };
    return capturableWindows->fromQWindow(window);
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

std::unique_ptr<QPlatformAudioDevices> QPlatformMediaIntegration::createAudioDevices()
{
    return QPlatformAudioDevices::create();
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

QPlatformAudioDevices *QPlatformMediaIntegration::audioDevices()
{
    std::call_once(m_audioDevicesOnceFlag, [this] {
        m_audioDevices = createAudioDevices();
    });
    return m_audioDevices.get();
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

QLatin1String QPlatformMediaIntegration::audioBackendName()
{
    return QPlatformMediaIntegration::instance()->audioDevices()->backendName();
}

QPlatformMediaIntegration::QPlatformMediaIntegration(QLatin1String name) : m_backendName(name) { }

QPlatformMediaIntegration::~QPlatformMediaIntegration() = default;

QT_END_NAMESPACE

#include "moc_qplatformmediaintegration_p.cpp"
