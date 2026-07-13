// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QTMULTIMEDIAPRIVATEQMLHELPER_H
#define QTMULTIMEDIAPRIVATEQMLHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

#include <QtQml/qqmlregistration.h>

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

public:
    [[nodiscard]] QString mediaBackendName() const;
    [[nodiscard]] QStringList availableBackends() const;
    [[nodiscard]] QString preferredBackend() const;
    [[nodiscard]] bool backendOverriddenByEnvironment() const;

    // Persists the chosen backend for the next launch. An empty string clears the preference.
    Q_INVOKABLE void setPreferredBackend(const QString &backend);

    // Applies the saved backend preference to the environment. Must be called from main() before
    // the media integration is first used. An explicitly set QT_MEDIA_BACKEND takes precedence and
    // is never overridden.
    static void applyPreferredBackendToEnvironment();

signals:
    void preferredBackendChanged();

private:
    static bool s_environmentOverrideActive;
};

#endif // QTMULTIMEDIAPRIVATEQMLHELPER_H
