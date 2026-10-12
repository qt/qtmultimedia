// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QMLMEDIAPLAYERADVANCEDHELPER_H
#define QMLMEDIAPLAYERADVANCEDHELPER_H

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariant.h>
#include <QtQml/qqmlregistration.h>

// Helpers for this test that need C++, such as private APIs.
class QmlMediaPlayerAdvancedHelper : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

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

    Q_PROPERTY(QVariantList onlineSamples READ onlineSamples CONSTANT)

    // The source given on the command line, which is played right away. Empty if none.
    Q_PROPERTY(QUrl startupSource READ startupSource CONSTANT)
    // Number of times to play the source given on the command line, or -1 for infinite.
    // 0 if --loops was not given.
    Q_PROPERTY(int startupLoops READ startupLoops CONSTANT)
    // True if the app should exit when playback reaches the end of the media.
    Q_PROPERTY(bool exitOnEnd READ exitOnEnd CONSTANT)
    // True if the controls should be hidden, leaving only the video.
    Q_PROPERTY(bool noGui READ noGui CONSTANT)

public:
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

    [[nodiscard]] QVariantList onlineSamples() const;

    [[nodiscard]] QUrl startupSource() const;
    // Must be called from main() before the QML engine loads.
    static void setStartupSource(const QUrl &source);

    [[nodiscard]] int startupLoops() const;
    static void setStartupLoops(int loops);

    [[nodiscard]] bool exitOnEnd() const;
    static void setExitOnEnd(bool exitOnEnd);

    [[nodiscard]] bool noGui() const;
    static void setNoGui(bool noGui);

    // Command-line requests for this run only, which are not saved. They take precedence
    // over the saved preferences and the environment, and the app is expected to exit if
    // it does not get exactly what was requested. The canonical functions map a
    // user-supplied name to the name used by the app, or return an empty string if it is
    // unknown.
    [[nodiscard]] static QStringList graphicsApiNames();
    [[nodiscard]] static QString canonicalGraphicsApi(const QString &input);
    [[nodiscard]] static QString canonicalMediaBackend(const QString &input);
    [[nodiscard]] static QString graphicsApiDisplayName(int api);
    static void setRequestedMediaBackend(const QString &backend);
    static void setRequestedGraphicsApi(const QString &api);
    [[nodiscard]] static QString requestedMediaBackend();
    [[nodiscard]] static QString requestedGraphicsApi();
    // Returns an error message if the loaded media backend is not the requested one, or an
    // empty string if it is or none was requested. Loads the media backend.
    [[nodiscard]] static QString verifyRequestedMediaBackend();

    // Applies the saved preferences. Must be called from main() before QtMultimedia is
    // used and before the first QQuickWindow is created. Environment variables take
    // precedence and are never overridden.
    static void applyPreferredBackends();

signals:
    void preferredMediaBackendChanged();
    void preferredGraphicsApiChanged();

private:
    static bool s_mediaBackendOverriddenByEnvironment;
    static bool s_graphicsApiOverriddenByEnvironment;
    static QUrl s_startupSource;
    static int s_startupLoops;
    static bool s_exitOnEnd;
    static bool s_noGui;
    static QString s_requestedMediaBackend;
    static QString s_requestedGraphicsApi;
};

#endif // QMLMEDIAPLAYERADVANCEDHELPER_H
