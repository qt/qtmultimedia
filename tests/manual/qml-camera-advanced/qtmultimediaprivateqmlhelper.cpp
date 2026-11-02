// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtmultimediaprivateqmlhelper.h"

#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtCore/qsettings.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qsgrendererinterface.h>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace {

[[nodiscard]] QString preferredBackendKey()
{
    return u"preferredMediaBackend"_s;
}

[[nodiscard]] QString savedPreferredBackend()
{
    QSettings settings;
    return settings.value(preferredBackendKey()).toString();
}

[[nodiscard]] QString preferredGraphicsApiKey()
{
    return u"preferredGraphicsApi"_s;
}

[[nodiscard]] QString savedPreferredGraphicsApi()
{
    QSettings settings;
    return settings.value(preferredGraphicsApiKey()).toString();
}

struct GraphicsApiEntry
{
    QSGRendererInterface::GraphicsApi api;
    const char16_t *name;
};

constexpr std::array graphicsApis = {
    GraphicsApiEntry{ QSGRendererInterface::GraphicsApi::Software, u"Software" },
    GraphicsApiEntry{ QSGRendererInterface::GraphicsApi::OpenGL, u"OpenGL" },
    GraphicsApiEntry{ QSGRendererInterface::GraphicsApi::Vulkan, u"Vulkan" },
    GraphicsApiEntry{ QSGRendererInterface::GraphicsApi::Metal, u"Metal" },
    GraphicsApiEntry{ QSGRendererInterface::GraphicsApi::Direct3D11, u"Direct3D 11" },
    GraphicsApiEntry{ QSGRendererInterface::GraphicsApi::Direct3D12, u"Direct3D 12" },
};

// Can still fail at runtime, e.g. if the driver lacks support.
[[nodiscard]] bool graphicsApiAvailable(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::GraphicsApi::Software:
        return true;
    case QSGRendererInterface::GraphicsApi::OpenGL:
        return QT_CONFIG(opengl);
    case QSGRendererInterface::GraphicsApi::Vulkan:
        return QT_CONFIG(vulkan);
    case QSGRendererInterface::GraphicsApi::Metal:
        return QT_CONFIG(metal);
    case QSGRendererInterface::GraphicsApi::Direct3D11:
    case QSGRendererInterface::GraphicsApi::Direct3D12:
#ifdef Q_OS_WIN
        return true;
#else
        return false;
#endif
    default:
        return false;
    }
}

} // anonymous namespace

bool QtMultimediaPrivateQmlHelper::s_environmentOverrideActive = false;
bool QtMultimediaPrivateQmlHelper::s_graphicsApiOverriddenByEnvironment = false;

QString QtMultimediaPrivateQmlHelper::mediaBackendName() const
{
    return QString(QPlatformMediaIntegration::instance()->name());
}

QStringList QtMultimediaPrivateQmlHelper::availableBackends() const
{
    return QPlatformMediaIntegration::availableBackends();
}

QString QtMultimediaPrivateQmlHelper::preferredBackend() const
{
    return savedPreferredBackend();
}

bool QtMultimediaPrivateQmlHelper::backendOverriddenByEnvironment() const
{
    return s_environmentOverrideActive;
}

void QtMultimediaPrivateQmlHelper::setPreferredBackend(const QString &backend)
{
    QSettings settings;
    settings.setValue(preferredBackendKey(), backend);
    emit preferredBackendChanged();
}

QStringList QtMultimediaPrivateQmlHelper::availableGraphicsApis() const
{
    QStringList names;
    for (const GraphicsApiEntry &entry : graphicsApis) {
        if (graphicsApiAvailable(entry.api))
            names.append(QString(entry.name));
    }
    return names;
}

QString QtMultimediaPrivateQmlHelper::preferredGraphicsApi() const
{
    return savedPreferredGraphicsApi();
}

bool QtMultimediaPrivateQmlHelper::graphicsApiOverriddenByEnvironment() const
{
    return s_graphicsApiOverriddenByEnvironment;
}

void QtMultimediaPrivateQmlHelper::setPreferredGraphicsApi(const QString &api)
{
    QSettings settings;
    settings.setValue(preferredGraphicsApiKey(), api);
    emit preferredGraphicsApiChanged();
}

QString QtMultimediaPrivateQmlHelper::graphicsApiName(int api) const
{
    const auto it = std::find_if(
        graphicsApis.begin(),
        graphicsApis.end(),
        [api](const GraphicsApiEntry &entry) {
            return entry.api == api;
        });
    if (it == graphicsApis.end())
        return u"Unknown"_s;
    return QString(it->name);
}

void QtMultimediaPrivateQmlHelper::applyPreferredBackends()
{
    s_environmentOverrideActive = qEnvironmentVariableIsSet("QT_MEDIA_BACKEND");
    if (!s_environmentOverrideActive) {
        const QString saved = savedPreferredBackend();
        if (!saved.isEmpty())
            QPlatformMediaIntegration::setBackend(saved);
    }

    s_graphicsApiOverriddenByEnvironment = qEnvironmentVariableIsSet("QSG_RHI_BACKEND")
        || qEnvironmentVariableIsSet("QT_QUICK_BACKEND")
        || qEnvironmentVariableIsSet("QMLSCENE_DEVICE");
    if (!s_graphicsApiOverriddenByEnvironment) {
        const QString name = savedPreferredGraphicsApi();
        const auto it = std::find_if(
            graphicsApis.begin(),
            graphicsApis.end(),
            [&name](const GraphicsApiEntry &entry) {
                return name == QStringView(entry.name);
            });
        if (it != graphicsApis.end() && graphicsApiAvailable(it->api))
            QQuickWindow::setGraphicsApi(it->api);
    }
}
