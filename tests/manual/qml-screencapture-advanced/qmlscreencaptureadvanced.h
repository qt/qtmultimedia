// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMLSCREENCAPTUREADVANCED_H
#define QMLSCREENCAPTUREADVANCED_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtMultimedia/qscreencapture.h>
#include <QtMultimedia/qwindowcapture.h>
#include <QtQml/qqmlregistration.h>

// Helpers for this test that need C++, such as platform APIs.
class QmlScreencaptureAdvanced : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // Screen and window capturing on macOS requires the Screen Recording permission,
    // which is not covered by QPermission. On other platforms, the permission is
    // always considered granted.
    //
    // macOS only applies a newly granted permission after the application restarts,
    // so the permission cannot change while the application is running.
    Q_PROPERTY(bool screenCapturePermissionGranted READ screenCapturePermissionGranted CONSTANT)

    // Media backend and Qt Quick graphics API. The preferred values are saved and only
    // applied on the next launch, see applyPreferredBackends(). Empty means default.
    Q_PROPERTY(QString mediaBackendName READ mediaBackendName CONSTANT)
    Q_PROPERTY(QStringList availableMediaBackends READ availableMediaBackends CONSTANT)
    Q_PROPERTY(QString preferredMediaBackend READ preferredMediaBackend
               NOTIFY preferredMediaBackendChanged)
    // True if QT_MEDIA_BACKEND is set, in which case the preference is ignored.
    Q_PROPERTY(bool mediaBackendOverriddenByEnvironment
               READ mediaBackendOverriddenByEnvironment CONSTANT)

    Q_PROPERTY(QStringList availableGraphicsApis READ availableGraphicsApis CONSTANT)
    Q_PROPERTY(QString preferredGraphicsApi READ preferredGraphicsApi
               NOTIFY preferredGraphicsApiChanged)
    // True if QSG_RHI_BACKEND, QT_QUICK_BACKEND or QMLSCENE_DEVICE is set, in which
    // case the preference is ignored.
    Q_PROPERTY(bool graphicsApiOverriddenByEnvironment
               READ graphicsApiOverriddenByEnvironment CONSTANT)

public:
    explicit QmlScreencaptureAdvanced(QObject *parent = nullptr);

    bool screenCapturePermissionGranted() const;

    // Shows the system dialog asking for the Screen Recording permission. macOS only
    // shows the dialog the first time; afterwards the permission must be granted in
    // System Settings.
    Q_INVOKABLE void requestScreenCapturePermission();

    Q_INVOKABLE void setScreenCaptureCursorEnabled(QScreenCapture *capture, bool enabled);
    Q_INVOKABLE void setWindowCaptureCursorEnabled(QWindowCapture *capture, bool enabled);
    Q_INVOKABLE bool screenCaptureCursorEnabled(QScreenCapture *capture) const;
    Q_INVOKABLE bool windowCaptureCursorEnabled(QWindowCapture *capture) const;

    [[nodiscard]] QString mediaBackendName() const;
    [[nodiscard]] QStringList availableMediaBackends() const;
    [[nodiscard]] QString preferredMediaBackend() const;
    [[nodiscard]] bool mediaBackendOverriddenByEnvironment() const;
    // Saves the media backend for the next launch. An empty string clears the preference.
    Q_INVOKABLE void setPreferredMediaBackend(const QString &backend);

    [[nodiscard]] QStringList availableGraphicsApis() const;
    [[nodiscard]] QString preferredGraphicsApi() const;
    [[nodiscard]] bool graphicsApiOverriddenByEnvironment() const;
    // Saves the graphics API for the next launch. An empty string clears the preference.
    Q_INVOKABLE void setPreferredGraphicsApi(const QString &api);
    // Takes a GraphicsInfo.api value.
    Q_INVOKABLE QString graphicsApiName(int api) const;

    // Applies the saved preferences. Must be called from main() before QtMultimedia is
    // used and before the first QQuickWindow is created. Environment variables take
    // precedence and are never overridden.
    static void applyPreferredBackends();

signals:
    void preferredMediaBackendChanged();
    void preferredGraphicsApiChanged();

private:
    bool m_screenCapturePermissionGranted = true;

    static bool s_mediaBackendOverriddenByEnvironment;
    static bool s_graphicsApiOverriddenByEnvironment;
};

#endif // QMLSCREENCAPTUREADVANCED_H
