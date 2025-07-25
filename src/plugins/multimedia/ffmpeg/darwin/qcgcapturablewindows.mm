// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qcgcapturablewindows_p.h>

#include <QtCore/private/qcore_mac_p.h>

#include <QtMultimedia/private/qcapturablewindow_p.h>

#import <AppKit/NSWindow.h>

QT_BEGIN_NAMESPACE

QList<QCapturableWindow> QCGCapturableWindows::windows() const
{
    QList<QCapturableWindow> result;
    QCFType<CFArrayRef> windowList(
            CGWindowListCopyWindowInfo(kCGWindowListOptionOnScreenOnly, kCGNullWindowID));

    // Iterate through the window dictionaries
    CFIndex count = CFArrayGetCount(windowList);
    for (CFIndex i = 0; i < count; ++i) {
        auto windowInfo = (CFDictionaryRef)CFArrayGetValueAtIndex(windowList, i);
        auto windowNumber = (CFNumberRef)CFDictionaryGetValue(windowInfo, kCGWindowNumber);
        auto windowName = (CFStringRef)CFDictionaryGetValue(windowInfo, kCGWindowName);

        CGWindowID windowId = 0;
        static_assert(sizeof(windowId) == 4,
                      "CGWindowID size is not compatible with kCFNumberSInt32Type");
        CFNumberGetValue(windowNumber, kCFNumberSInt32Type, &windowId);

        QString windowDescription;
        if (windowName)
            windowDescription = QString::fromCFString(windowName);

        result.push_back(QCapturableWindowPrivate::create(
            static_cast<QCapturableWindowPrivate::Id>(windowId),
            std::move(windowDescription)));
    }

    return result;
}

bool QCGCapturableWindows::isWindowValid(const QCapturableWindowPrivate &window) const
{
    QCFType<CFArrayRef> windowList(
            CGWindowListCreate(kCGWindowListOptionIncludingWindow, window.id));
    return CFArrayGetCount(windowList) > 0;
}

QT_END_NAMESPACE
