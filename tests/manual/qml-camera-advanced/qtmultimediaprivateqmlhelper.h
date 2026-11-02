// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTMULTIMEDIAPRIVATEQMLHELPER_H
#define QTMULTIMEDIAPRIVATEQMLHELPER_H

#include <QtQml/qqmlregistration.h>
#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

class QtMultimediaPrivateQmlHelper : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(QtMultimediaPrivate)
    QML_SINGLETON

    Q_PROPERTY(QString mediaBackendName READ mediaBackendName CONSTANT)

    Q_PROPERTY(QStringList availableBackends READ availableBackends CONSTANT)

    // The backend to load on the next launch. Empty means default.
    Q_PROPERTY(QString preferredBackend READ preferredBackend NOTIFY preferredBackendChanged)

    // True if QT_MEDIA_BACKEND was set, in which case the saved preference
    // should be ignored.
    Q_PROPERTY(bool backendOverriddenByEnvironment READ backendOverriddenByEnvironment CONSTANT)

    // Applied on the next launch. Empty means default.
    Q_PROPERTY(QStringList availableGraphicsApis READ availableGraphicsApis CONSTANT)
    Q_PROPERTY(QString preferredGraphicsApi READ preferredGraphicsApi NOTIFY preferredGraphicsApiChanged)
    // True if QSG_RHI_BACKEND, QT_QUICK_BACKEND or QMLSCENE_DEVICE is set.
    Q_PROPERTY(bool graphicsApiOverriddenByEnvironment READ graphicsApiOverriddenByEnvironment CONSTANT)

public:
    [[nodiscard]] QString mediaBackendName() const;
    [[nodiscard]] QStringList availableBackends() const;
    [[nodiscard]] QString preferredBackend() const;
    [[nodiscard]] bool backendOverriddenByEnvironment() const;

    // Persists the chosen backend for the next launch. An empty string clears the preference.
    Q_INVOKABLE void setPreferredBackend(const QString &backend);

    [[nodiscard]] QStringList availableGraphicsApis() const;
    [[nodiscard]] QString preferredGraphicsApi() const;
    [[nodiscard]] bool graphicsApiOverriddenByEnvironment() const;

    // An empty string clears the preference.
    Q_INVOKABLE void setPreferredGraphicsApi(const QString &api);

    // Takes a GraphicsInfo.api value.
    [[nodiscard]] Q_INVOKABLE QString graphicsApiName(int api) const;

    // Must be called from main() before the media integration is first used and before the first
    // window is created. Environment variables take precedence over the saved preferences.
    static void applyPreferredBackends();

signals:
    void preferredBackendChanged();
    void preferredGraphicsApiChanged();

private:
    static bool s_environmentOverrideActive;
    static bool s_graphicsApiOverriddenByEnvironment;
};

#endif // QTMULTIMEDIAPRIVATEQMLHELPER_H
