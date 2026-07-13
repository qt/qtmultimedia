// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qtmultimediaprivateqmlhelper.h"

#include <QtCore/qsettings.h>

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

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

} // anonymous namespace

bool QtMultimediaPrivateQmlHelper::s_environmentOverrideActive = false;

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

void QtMultimediaPrivateQmlHelper::applyPreferredBackendToEnvironment()
{
    s_environmentOverrideActive = qEnvironmentVariableIsSet("QT_MEDIA_BACKEND");
    if (s_environmentOverrideActive)
        return;

    const QString saved = savedPreferredBackend();
    if (!saved.isEmpty())
        QPlatformMediaIntegration::setPreferredBackend(saved);
}
