// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlmediaplayeradvanced.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsondocument.h>
#include <QtCore/qsettings.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuick/qsgrendererinterface.h>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace {

const QString mediaBackendKey = u"preferredMediaBackend"_s;
const QString graphicsApiKey = u"preferredGraphicsApi"_s;
const QString onlineSamplesPath = u":/qt/qml/QmlMediaPlayerAdvanced/onlinesamples.json"_s;

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

// Whether the graphics API can be requested in this build on this platform. It might
// still fail at runtime, for example if the driver lacks support.
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

[[nodiscard]] QString savedSetting(const QString &key)
{
    QSettings settings;
    return settings.value(key).toString();
}

} // namespace

bool QmlMediaPlayerAdvanced::s_mediaBackendOverriddenByEnvironment = false;
bool QmlMediaPlayerAdvanced::s_graphicsApiOverriddenByEnvironment = false;

QString QmlMediaPlayerAdvanced::mediaBackendName() const
{
    return QString(QPlatformMediaIntegration::instance()->name());
}

QStringList QmlMediaPlayerAdvanced::availableMediaBackends() const
{
    return QPlatformMediaIntegration::availableBackends();
}

QString QmlMediaPlayerAdvanced::preferredMediaBackend() const
{
    return savedSetting(mediaBackendKey);
}

bool QmlMediaPlayerAdvanced::mediaBackendOverriddenByEnvironment() const
{
    return s_mediaBackendOverriddenByEnvironment;
}

void QmlMediaPlayerAdvanced::setPreferredMediaBackend(const QString &backend)
{
    QSettings settings;
    settings.setValue(mediaBackendKey, backend);
    emit preferredMediaBackendChanged();
}

QStringList QmlMediaPlayerAdvanced::availableGraphicsApis() const
{
    QStringList names;
    for (const GraphicsApiEntry &entry : graphicsApis) {
        if (graphicsApiAvailable(entry.api))
            names.append(QString(entry.name));
    }
    return names;
}

QString QmlMediaPlayerAdvanced::preferredGraphicsApi() const
{
    return savedSetting(graphicsApiKey);
}

bool QmlMediaPlayerAdvanced::graphicsApiOverriddenByEnvironment() const
{
    return s_graphicsApiOverriddenByEnvironment;
}

void QmlMediaPlayerAdvanced::setPreferredGraphicsApi(const QString &api)
{
    QSettings settings;
    settings.setValue(graphicsApiKey, api);
    emit preferredGraphicsApiChanged();
}

QString QmlMediaPlayerAdvanced::graphicsApiName(int api) const
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

QVariantList QmlMediaPlayerAdvanced::onlineSamples() const
{
    QFile file(onlineSamplesPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open" << onlineSamplesPath << file.errorString();
        return {};
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "Failed to parse" << onlineSamplesPath << error.errorString();
        return {};
    }
    return document.array().toVariantList();
}

void QmlMediaPlayerAdvanced::applyPreferredBackends()
{
    s_mediaBackendOverriddenByEnvironment = qEnvironmentVariableIsSet("QT_MEDIA_BACKEND");
    if (!s_mediaBackendOverriddenByEnvironment) {
        const QString backend = savedSetting(mediaBackendKey);
        if (!backend.isEmpty())
            QPlatformMediaIntegration::setBackend(backend);
    }

    s_graphicsApiOverriddenByEnvironment = qEnvironmentVariableIsSet("QSG_RHI_BACKEND")
        || qEnvironmentVariableIsSet("QT_QUICK_BACKEND")
        || qEnvironmentVariableIsSet("QMLSCENE_DEVICE");
    if (!s_graphicsApiOverriddenByEnvironment) {
        const QString name = savedSetting(graphicsApiKey);
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
