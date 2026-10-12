// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlmediaplayeradvancedhelper.h"

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
const QString onlineSamplesPath =
    u":/qt/qml/QmlMediaPlayerAdvanced/qml-mediaplayer-advanced-onlinesamples.json"_s;

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

// Lower case with spaces removed, so that "direct3d 11", "Direct3D11" and "d3d11" compare equal.
[[nodiscard]] QString normalized(QString name)
{
    name.remove(QChar::Space);
    return name.toLower();
}

[[nodiscard]] QString savedSetting(const QString &key)
{
    QSettings settings;
    return settings.value(key).toString();
}

} // namespace

bool QmlMediaPlayerAdvancedHelper::s_mediaBackendOverriddenByEnvironment = false;
bool QmlMediaPlayerAdvancedHelper::s_graphicsApiOverriddenByEnvironment = false;
QUrl QmlMediaPlayerAdvancedHelper::s_startupSource;
int QmlMediaPlayerAdvancedHelper::s_startupLoops = 0;
bool QmlMediaPlayerAdvancedHelper::s_exitOnEnd = false;
bool QmlMediaPlayerAdvancedHelper::s_noGui = false;
QString QmlMediaPlayerAdvancedHelper::s_requestedMediaBackend;
QString QmlMediaPlayerAdvancedHelper::s_requestedGraphicsApi;

QString QmlMediaPlayerAdvancedHelper::mediaBackendName() const
{
    return QString(QPlatformMediaIntegration::instance()->name());
}

QStringList QmlMediaPlayerAdvancedHelper::availableMediaBackends() const
{
    return QPlatformMediaIntegration::availableBackends();
}

QString QmlMediaPlayerAdvancedHelper::preferredMediaBackend() const
{
    return savedSetting(mediaBackendKey);
}

bool QmlMediaPlayerAdvancedHelper::mediaBackendOverriddenByEnvironment() const
{
    return s_mediaBackendOverriddenByEnvironment;
}

void QmlMediaPlayerAdvancedHelper::setPreferredMediaBackend(const QString &backend)
{
    QSettings settings;
    settings.setValue(mediaBackendKey, backend);
    emit preferredMediaBackendChanged();
}

QStringList QmlMediaPlayerAdvancedHelper::availableGraphicsApis() const
{
    return graphicsApiNames();
}

QStringList QmlMediaPlayerAdvancedHelper::graphicsApiNames()
{
    QStringList names;
    for (const GraphicsApiEntry &entry : graphicsApis) {
        if (graphicsApiAvailable(entry.api))
            names.append(QString(entry.name));
    }
    return names;
}

QString QmlMediaPlayerAdvancedHelper::canonicalGraphicsApi(const QString &input)
{
    const QString wanted = normalized(input);
    for (const GraphicsApiEntry &entry : graphicsApis) {
        if (!graphicsApiAvailable(entry.api))
            continue;
        const QString name = QString(entry.name);
        // Also accept the short forms "d3d11" and "d3d12".
        QString shortName = normalized(name);
        shortName.replace(u"direct3d"_s, u"d3d"_s);
        if (wanted == normalized(name) || wanted == shortName)
            return name;
    }
    return {};
}

QString QmlMediaPlayerAdvancedHelper::canonicalMediaBackend(const QString &input)
{
    const QStringList backends = QPlatformMediaIntegration::availableBackends();
    for (const QString &backend : backends) {
        if (backend.compare(input.trimmed(), Qt::CaseInsensitive) == 0)
            return backend;
    }
    return {};
}

void QmlMediaPlayerAdvancedHelper::setRequestedMediaBackend(const QString &backend)
{
    s_requestedMediaBackend = backend;
}

void QmlMediaPlayerAdvancedHelper::setRequestedGraphicsApi(const QString &api)
{
    s_requestedGraphicsApi = api;
}

QString QmlMediaPlayerAdvancedHelper::requestedMediaBackend()
{
    return s_requestedMediaBackend;
}

QString QmlMediaPlayerAdvancedHelper::requestedGraphicsApi()
{
    return s_requestedGraphicsApi;
}

QString QmlMediaPlayerAdvancedHelper::verifyRequestedMediaBackend()
{
    if (s_requestedMediaBackend.isEmpty())
        return {};
    const QString actual = QString(QPlatformMediaIntegration::instance()->name());
    if (actual.compare(s_requestedMediaBackend, Qt::CaseInsensitive) == 0)
        return {};
    return u"Requested media backend \"%1\" but got \"%2\"."_s.arg(
        s_requestedMediaBackend,
        actual);
}

QString QmlMediaPlayerAdvancedHelper::preferredGraphicsApi() const
{
    return savedSetting(graphicsApiKey);
}

bool QmlMediaPlayerAdvancedHelper::graphicsApiOverriddenByEnvironment() const
{
    return s_graphicsApiOverriddenByEnvironment;
}

void QmlMediaPlayerAdvancedHelper::setPreferredGraphicsApi(const QString &api)
{
    QSettings settings;
    settings.setValue(graphicsApiKey, api);
    emit preferredGraphicsApiChanged();
}

QString QmlMediaPlayerAdvancedHelper::graphicsApiName(int api) const
{
    return graphicsApiDisplayName(api);
}

QString QmlMediaPlayerAdvancedHelper::graphicsApiDisplayName(int api)
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

QUrl QmlMediaPlayerAdvancedHelper::startupSource() const
{
    return s_startupSource;
}

void QmlMediaPlayerAdvancedHelper::setStartupSource(const QUrl &source)
{
    s_startupSource = source;
}

int QmlMediaPlayerAdvancedHelper::startupLoops() const
{
    return s_startupLoops;
}

void QmlMediaPlayerAdvancedHelper::setStartupLoops(int loops)
{
    s_startupLoops = loops;
}

bool QmlMediaPlayerAdvancedHelper::exitOnEnd() const
{
    return s_exitOnEnd;
}

void QmlMediaPlayerAdvancedHelper::setExitOnEnd(bool exitOnEnd)
{
    s_exitOnEnd = exitOnEnd;
}

bool QmlMediaPlayerAdvancedHelper::noGui() const
{
    return s_noGui;
}

void QmlMediaPlayerAdvancedHelper::setNoGui(bool noGui)
{
    s_noGui = noGui;
}

QVariantList QmlMediaPlayerAdvancedHelper::onlineSamples() const
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

void QmlMediaPlayerAdvancedHelper::applyPreferredBackends()
{
    s_mediaBackendOverriddenByEnvironment = qEnvironmentVariableIsSet("QT_MEDIA_BACKEND")
        || !s_requestedMediaBackend.isEmpty();
    if (!s_requestedMediaBackend.isEmpty()) {
        QPlatformMediaIntegration::setBackend(s_requestedMediaBackend);
    } else if (!s_mediaBackendOverriddenByEnvironment) {
        const QString backend = savedSetting(mediaBackendKey);
        if (!backend.isEmpty())
            QPlatformMediaIntegration::setBackend(backend);
    }

    s_graphicsApiOverriddenByEnvironment = qEnvironmentVariableIsSet("QSG_RHI_BACKEND")
        || qEnvironmentVariableIsSet("QT_QUICK_BACKEND")
        || qEnvironmentVariableIsSet("QMLSCENE_DEVICE") || !s_requestedGraphicsApi.isEmpty();
    if (!s_requestedGraphicsApi.isEmpty() || !s_graphicsApiOverriddenByEnvironment) {
        const QString name = !s_requestedGraphicsApi.isEmpty() ? s_requestedGraphicsApi
                                                               : savedSetting(graphicsApiKey);
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
