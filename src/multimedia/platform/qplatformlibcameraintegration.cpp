// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformlibcameraintegration_p.h"

#include <QtMultimedia/private/qplatformlibcameraplugin_p.h>

#include <QtCore/qapplicationstatic.h>
#include <QtCore/private/qfactoryloader_p.h>

QT_BEGIN_NAMESPACE

namespace QLibCamera {

Q_STATIC_LOGGING_CATEGORY(qLcMediaPlugin, "qt.multimedia.plugin")

QPlatformLibCameraIntegration::QPlatformLibCameraIntegration(QObject *parent) : QObject(parent) { }
QPlatformLibCameraIntegration::~QPlatformLibCameraIntegration() = default;

namespace {

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, libCameraLoader,
                          (QPlatformLibCameraPlugin_iid, QLatin1String("/libcamera")))

struct LibCameraInstanceHolder
{
    LibCameraInstanceHolder() { init(); }

    void init()
    {
        qCDebug(qLcMediaPlugin) << "Loading libcamera plugin";
        QObject *factoryObject = libCameraLoader()->instance(0);
        if (auto *factory = qobject_cast<QPlatformLibCameraPlugin *>(factoryObject))
            instance = factory->create();

        if (!instance)
            qCWarning(qLcMediaPlugin)
                    << "libcamera plugin failed to load; falling back to the platform's default "
                       "camera support";
    }

    std::unique_ptr<QPlatformLibCameraIntegration> instance;
};

Q_APPLICATION_STATIC(LibCameraInstanceHolder, s_libCameraInstanceHolder);

} // namespace

QPlatformLibCameraIntegration *libCameraIntegration()
{
    return s_libCameraInstanceHolder->instance.get();
}

} // namespace QLibCamera

QT_END_NAMESPACE

#include "moc_qplatformlibcameraintegration_p.cpp"
