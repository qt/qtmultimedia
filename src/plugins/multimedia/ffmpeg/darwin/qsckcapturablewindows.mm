// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsckcapturablewindows_p.h"

#include <QtCore/private/qcore_mac_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/qlist.h>

#include <QtFFmpegMediaPluginImpl/private/qmacscreencapturekit_p.h>

#include <QtGui/qwindow.h>

#include <QtMultimedia/private/qcapturablewindow_p.h>

#import <AppKit/NSWindow.h>

#include <algorithm>
#include <vector>

using namespace Qt::Literals::StringLiterals;

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

// ScreenCaptureKit reports several windows that cannot be captured, or
// are otherwise just black. Such windows include invidividual icons on
// the taskbar. Filtering out some of these based on bundle identifier
// tends to clean up the list.
constexpr std::array windowBundleIdentifierBlacklist = {
    "com.apple.controlcenter",
    "com.apple.dock",
    "com.apple.notificationcenterui", };

// Can be called from any thread.
QList<QCapturableWindow> QSckCapturableWindows::windows() const
{
    QMacAutoReleasePool autoReleasePool;

    // Do a blocking query for ScreenCaptureKit capturable items.
    q23::expected<QMacScreenCaptureKit::CapturableItems, QString> enumerateResult =
        QMacScreenCaptureKit::enumerateCapturableItems()
        .get();
    if (!enumerateResult) {
        qCWarning(qLcMacScreenCapture)
            << "Failed to enumerate capturable windows/displays: "
            << enumerateResult.error();
        return {};
    }

    QMacScreenCaptureKit::CapturableItems const &capturableItems = *enumerateResult;

    QList<QCapturableWindow> result;

    for (AVFScopedPointer<SCWindow> const &window : capturableItems.windows) {
        // SCWindows with onScreen set to false usually indicates we cannot
        // capture it at this time, but it might be capturable in the future.
        if (!window.data().onScreen)
            continue;

        QString bundleIdentifier = QString::fromNSString(window.data().owningApplication.bundleIdentifier);
        // An empty bundle-identifier commonly implies this window cannot be captured.
        if (bundleIdentifier.isEmpty())
            continue;

        bool bundleIdentifierBlacklisted = std::any_of(
            windowBundleIdentifierBlacklist.begin(),
            windowBundleIdentifierBlacklist.end(),
            [&](const char *identifier) {
                return QString::fromLatin1(identifier, -1) == bundleIdentifier;
            });
        if (bundleIdentifierBlacklisted)
            continue;

        // No title commonly implies this is not a user-facing window.
        QString title = QString::fromNSString(window.data().title);
        if (title.isEmpty())
            continue;

        result.push_back(QCapturableWindowPrivate::create(
            static_cast<QCapturableWindowPrivate::Id>(window.data().windowID),
            std::move(title)));
    }

    return result;
}

// Can be called from any thread.
bool QSckCapturableWindows::isWindowValid(const QCapturableWindowPrivate &window) const
{
    QMacAutoReleasePool autoReleasePool;

    if (window.id == 0)
        return false;

    // Do a blocking query for ScreenCaptureKit capturable items.
    q23::expected<QMacScreenCaptureKit::CapturableItems, QString> enumerateResult =
        QMacScreenCaptureKit::enumerateCapturableItems()
        .get();
    if (!enumerateResult) {
        qCWarning(qLcMacScreenCapture)
            << "Failed to enumerate capturable windows/displays during QSckCapturableWindows::isWindowValid: "
            << enumerateResult.error();
        return false;
    }

    std::vector<AVFScopedPointer<SCWindow>> const &windows = enumerateResult->windows;

    return std::any_of(
        windows.begin(),
        windows.end(),
        [&](AVFScopedPointer<SCWindow> const &item) {
            return item.data().windowID == static_cast<CGWindowID>(window.id);
        });
}

// Can be called from any thread.
q23::expected<QCapturableWindow, QString> QSckCapturableWindows::fromQWindow(QWindow *window) const
{
    QMacAutoReleasePool autoReleasePool;

    auto *nsView = reinterpret_cast<NSView*>(window->winId());

    NSWindow *nsWindow = [nsView window];
    if (nsWindow == nullptr)
        return q23::unexpected{ QStringLiteral("NSView had no associated NSWindow") };

    auto cgWindowId = (CGWindowID)[nsWindow windowNumber];
    if (cgWindowId == kCGNullWindowID)
        return q23::unexpected{ QStringLiteral("NSWindow has no CGWindowID") };

    return QCapturableWindowPrivate::create(
        static_cast<QCapturableWindowPrivate::Id>(cgWindowId),
        window->title());
}

std::unique_ptr<QPlatformCapturableWindows> makeQSckCapturableWindows()
{
    return std::make_unique<QSckCapturableWindows>();
}

} // namespace QFFmpeg

QT_END_NAMESPACE
