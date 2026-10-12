// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qmlmediaplayeradvancedhelper.h"

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qdir.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qsettings.h>

#include <QtGui/qguiapplication.h>

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

#include <QtQml/qqmlapplicationengine.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/qsgrendererinterface.h>

#include <optional>

struct CLIArgs
{
    std::optional<QUrl> source;
    std::optional<int> loops; // -1 for infinite
    bool exitOnEnd = false;
    bool noGui = false;
    QString graphicsApi;
    QString mediaBackend;
};

std::optional<CLIArgs> parseArgs(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "Advanced media player test. If a source is given, it is played right away, and the app "
        "exits with code 1 if playback fails.");
    parser.addHelpOption();
    parser.addPositionalArgument(
        "source",
        "Media file or URL to play. Relative paths are resolved against the current directory. "
        "The word \"help\" prints this help, like --help.",
        "[source]");
    const QCommandLineOption graphicsApiOption(
        "graphics-api",
        "Qt Quick graphics API to use, one of: "
            + QmlMediaPlayerAdvancedHelper::graphicsApiNames().join(", ")
            + ". Takes precedence over the saved setting and the environment. The app exits "
              "with an error if it does not get exactly this API.",
        "api");
    const QCommandLineOption mediaBackendOption(
        "media-backend",
        "Media backend to use, one of: "
            + QPlatformMediaIntegration::availableBackends().join(", ")
            + ". Takes precedence over the saved setting and the environment. The app exits "
              "with an error if it does not get exactly this backend.",
        "backend");
    const QCommandLineOption loopsOption(
        "loops",
        "Number of times to play the source, or \"infinite\".",
        "count");
    const QCommandLineOption exitOnEndOption(
        "exit-on-end",
        "Exit with code 0 when playback reaches the end of the media.");
    const QCommandLineOption noGuiOption(
        "no-gui",
        "Hide the controls and settings, leaving only the video. Useful for screenshots.");
    parser.addOption(graphicsApiOption);
    parser.addOption(mediaBackendOption);
    parser.addOption(loopsOption);
    parser.addOption(exitOnEndOption);
    parser.addOption(noGuiOption);
    parser.process(app);

    CLIArgs args;
    const auto fail = [](const QString &message) -> std::optional<CLIArgs> {
        qCritical().noquote() << message;
        return std::nullopt;
    };

    const QStringList positionalArguments = parser.positionalArguments();
    if (positionalArguments.size() > 1)
        return fail("Too many arguments, expected at most one source.");
    if (positionalArguments.size() == 1) {
        if (positionalArguments.first() == "help")
            parser.showHelp();
        args.source = QUrl::fromUserInput(
            positionalArguments.first(),
            QDir::currentPath(),
            QUrl::AssumeLocalFile);
    }

    if (parser.isSet(graphicsApiOption)) {
        const QString api =
            QmlMediaPlayerAdvancedHelper::canonicalGraphicsApi(parser.value(graphicsApiOption));
        if (api.isEmpty())
            return fail(
                "Unknown or unavailable graphics API \"" + parser.value(graphicsApiOption)
                + "\". Available: " + QmlMediaPlayerAdvancedHelper::graphicsApiNames().join(", ")
                + ".");
        args.graphicsApi = api;
    }
    if (parser.isSet(mediaBackendOption)) {
        const QString backend =
            QmlMediaPlayerAdvancedHelper::canonicalMediaBackend(parser.value(mediaBackendOption));
        if (backend.isEmpty())
            return fail(
                "Unknown or unavailable media backend \"" + parser.value(mediaBackendOption)
                + "\". Available: " + QPlatformMediaIntegration::availableBackends().join(", ")
                + ".");
        args.mediaBackend = backend;
    }

    if (parser.isSet(loopsOption)) {
        const QString value = parser.value(loopsOption);
        if (value.compare("infinite", Qt::CaseInsensitive) == 0) {
            args.loops = -1;
        } else {
            bool ok = false;
            const int count = value.toInt(&ok);
            if (!ok || count < 1)
                return fail(
                    "Invalid loop count \"" + value
                    + "\", expected a positive integer or \"infinite\".");
            args.loops = count;
        }
    }
    args.exitOnEnd = parser.isSet(exitOnEndOption);
    args.noGui = parser.isSet(noGuiOption);

    return args;
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCoreApplication::setOrganizationName("QtProject");
    QCoreApplication::setApplicationName("qml-mediaplayer-advanced");
    // Store settings in an INI file on every platform, rather than the registry on
    // Windows or a plist on macOS, so that they are easy to inspect, edit and reset.
    // Must be set before any settings are read.
    QSettings::setDefaultFormat(QSettings::IniFormat);

    const std::optional<CLIArgs> args = parseArgs(app);
    if (!args)
        return 1;

    if (args->source)
        QmlMediaPlayerAdvancedHelper::setStartupSource(*args->source);
    if (args->loops)
        QmlMediaPlayerAdvancedHelper::setStartupLoops(*args->loops);
    QmlMediaPlayerAdvancedHelper::setExitOnEnd(args->exitOnEnd);
    QmlMediaPlayerAdvancedHelper::setNoGui(args->noGui);
    if (!args->graphicsApi.isEmpty())
        QmlMediaPlayerAdvancedHelper::setRequestedGraphicsApi(args->graphicsApi);
    if (!args->mediaBackend.isEmpty())
        QmlMediaPlayerAdvancedHelper::setRequestedMediaBackend(args->mediaBackend);

    QmlMediaPlayerAdvancedHelper::applyPreferredBackends();

    // The media backend is loaded on first use, so force it now to fail before any window or
    // playback exists.
    if (const QString error = QmlMediaPlayerAdvancedHelper::verifyRequestedMediaBackend();
        !error.isEmpty()) {
        qCritical().noquote() << error;
        return 1;
    }

    qDebug() << QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("QmlMediaPlayerAdvanced", "QmlMediaPlayerAdvanced");

    // The graphics API is only known once the scene graph has been initialized, which can fail
    // or silently end up on another API. Exit right away if it is not the requested one.
    const QString requestedGraphicsApi = QmlMediaPlayerAdvancedHelper::requestedGraphicsApi();
    if (!requestedGraphicsApi.isEmpty()) {
        // Queued, since exit() has no effect before the event loop has started.
        const auto exitWithFailure = []() {
            QMetaObject::invokeMethod(
                qApp,
                []() {
                    QCoreApplication::exit(1);
                },
                Qt::QueuedConnection);
        };
        const auto checkGraphicsApi =
            [requestedGraphicsApi, exitWithFailure](QQuickWindow *window) {
                const QSGRendererInterface *rendererInterface = window->rendererInterface();
                if (!rendererInterface)
                    return;
                const QString actual = QmlMediaPlayerAdvancedHelper::graphicsApiDisplayName(
                    rendererInterface->graphicsApi());
                if (actual == requestedGraphicsApi)
                    return;
                qCritical().noquote() << "Requested graphics API" << requestedGraphicsApi
                                      << "but got" << actual;
                exitWithFailure();
            };
        const auto rootObjects = engine.rootObjects();
        for (QObject *object : rootObjects) {
            auto *window = qobject_cast<QQuickWindow *>(object);
            if (!window)
                continue;
            QObject::connect(
                window,
                &QQuickWindow::sceneGraphInitialized,
                &app,
                [window, checkGraphicsApi]() {
                    checkGraphicsApi(window);
                },
                Qt::QueuedConnection);
            QObject::connect(
                window,
                &QQuickWindow::sceneGraphError,
                &app,
                [requestedGraphicsApi, exitWithFailure](
                    QQuickWindow::SceneGraphError,
                    const QString &message) {
                    qCritical().noquote() << "Requested graphics API" << requestedGraphicsApi
                                          << "failed:" << message;
                    exitWithFailure();
                },
                Qt::QueuedConnection);
            checkGraphicsApi(window);
        }
    }

    return app.exec();
}
