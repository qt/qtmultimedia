// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qx11capturablewindows_p.h"

#include <QtCore/qdebug.h>
#include <QtGui/qwindow.h>
#include <QtMultimedia/private/qcapturablewindow_p.h>

#include <X11/Xlib.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace { // Anonymous namespace start

[[nodiscard]] bool qIsX11WindowValid(Display *display, Window window)
{
    XWindowAttributes windowAttributes = {};
    return display
        && XGetWindowAttributes(display, window, &windowAttributes) != 0
        && windowAttributes.depth > 0;
}

[[nodiscard]] std::optional<QString> qGetX11WindowTitle(Display *display, Window window)
{
    if (!display)
        return std::nullopt;

    char *windowTitle = nullptr;
    if (XFetchName(display, window, &windowTitle) && windowTitle) {
        QString out = QString::fromUtf8(windowTitle);
        XFree(windowTitle);
        return out;
    }

    return std::nullopt;
}

} // Anonymous namespace end

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
        XID windowId = windows[i];
        if (!qIsX11WindowValid(display, windowId))
            continue;

        result.push_back(QCapturableWindowPrivate::create(
            static_cast<QCapturableWindowPrivate::Id>(windowId),
            qGetX11WindowTitle(display, windowId).value_or(QString())));
    }

    return result;
}

bool QX11CapturableWindows::isWindowValid(const QCapturableWindowPrivate &window) const
{
    return qIsX11WindowValid(this->display(), static_cast<Window>(window.id));
}

Display *QX11CapturableWindows::display() const
{
    std::call_once(m_displayOnceFlag, [this]() { m_display = XOpenDisplay(nullptr); });
    return m_display;
}

q23::expected<QCapturableWindow, QString> QX11CapturableWindows::fromQWindow(QWindow *window) const
{
    const auto xId = static_cast<XID>(window->winId());
    return QCapturableWindowPrivate::create(
        static_cast<QCapturableWindowPrivate::Id>(xId),
        window->title());
}

QT_END_NAMESPACE
