// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qx11capturablewindows_p.h"
#include "private/qcapturablewindow_p.h"
#include <qdebug.h>

#include <X11/Xlib.h>

QT_BEGIN_NAMESPACE

QX11CapturableWindows::~QX11CapturableWindows()
{
    if (m_display)
        XCloseDisplay(m_display);
}

QList<QCapturableWindow> QX11CapturableWindows::windows() const
{
    auto display = this->display();

    if (!display)
        return {};

    Atom atom = XInternAtom(display, "_NET_CLIENT_LIST", true);
    Atom actualType = 0;
    int format = 0;
    unsigned long windowsCount = 0;
    unsigned long bytesAfter = 0;
    unsigned char *data = nullptr;
    const int status = XGetWindowProperty(display, XDefaultRootWindow(display), atom, 0L, (~0L),
                                          false, AnyPropertyType, &actualType, &format,
                                          &windowsCount, &bytesAfter, &data);

    if (status < Success || !data)
        return {};

    QList<QCapturableWindow> result;

    auto freeDataGuard = qScopeGuard([data]() { XFree(data); });
    auto windows = reinterpret_cast<XID *>(data);
    for (unsigned long i = 0; i < windowsCount; i++) {
        auto windowData = std::make_unique<QCapturableWindowPrivate>();
        windowData->id = static_cast<QCapturableWindowPrivate::Id>(windows[i]);

        char *windowTitle = nullptr;
        if (XFetchName(display, windows[i], &windowTitle) && windowTitle) {
            windowData->description = QString::fromUtf8(windowTitle);
            XFree(windowTitle);
        }

        if (isWindowValid(*windowData))
            result.push_back(windowData.release()->create());
    }

    return result;
}

bool QX11CapturableWindows::isWindowValid(const QCapturableWindowPrivate &window) const
{
    auto display = this->display();
    XWindowAttributes windowAttributes = {};
    return display
            && XGetWindowAttributes(display, static_cast<Window>(window.id), &windowAttributes) != 0
            && windowAttributes.depth > 0;
}

Display *QX11CapturableWindows::display() const
{
    std::call_once(m_displayOnceFlag, [this]() { m_display = XOpenDisplay(nullptr); });
    return m_display;
}

QT_END_NAMESPACE
