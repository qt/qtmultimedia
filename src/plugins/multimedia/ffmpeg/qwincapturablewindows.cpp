// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwincapturablewindows_p.h"
#include "private/qcapturablewindow_p.h"

#include <QtGui/qwindow.h>

#include <qt_windows.h>
#include <dwmapi.h>

QT_BEGIN_NAMESPACE

static bool isTopLevelWindow(HWND hwnd)
{
    return hwnd && ::GetAncestor(hwnd, GA_ROOT) == hwnd;
}

static bool canCaptureWindow(HWND hwnd)
{
    Q_ASSERT(hwnd);

    if (!::IsWindowVisible(hwnd))
        return false;

    // Minimized windows are not rendered, so capturing them never delivers a frame
    if (::IsIconic(hwnd))
        return false;

    // The desktop is not a window that can be captured
    if (hwnd == ::GetShellWindow())
        return false;

    // Windows without a class name are not real windows
    wchar_t className[2] = {};
    if (::GetClassNameW(hwnd, className, 2) == 0)
        return false;

    if (::GetWindowLongPtrW(hwnd, GWL_STYLE) & WS_DISABLED)
        return false;

    // Tool windows are helpers like the taskbar, tooltips and message windows
    if (::GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return false;

    // Cloaked windows are visible according to IsWindowVisible, but are not shown on screen
    // (e.g. suspended UWP apps, windows on other virtual desktops, or system helper windows)
    DWORD cloaked = 0;
    if (SUCCEEDED(::DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked)))
        && cloaked != 0)
        return false;

    RECT rect{};
    if (!::GetWindowRect(hwnd, &rect))
        return false;

    if (rect.left >= rect.right || rect.top >= rect.bottom)
        return false;

    return true;
}

static QString windowTitle(HWND hwnd) {
    // QTBUG-114890
    // TODO: investigate the case when hwnd is inner and belows to another thread.
    // It might causes deadlocks in specific cases.
    auto titleLength = ::GetWindowTextLengthW(hwnd);
    std::wstring buffer(titleLength + 1, L'\0');
    titleLength = ::GetWindowTextW(hwnd, buffer.data(), titleLength + 1);
    buffer.resize(titleLength);

    return QString::fromStdWString(buffer);
}

static BOOL CALLBACK windowHandler(HWND hwnd, LPARAM lParam) {
    if (!canCaptureWindow(hwnd))
        return TRUE; // Ignore window and continue enumerating

    auto &windows = *reinterpret_cast<QList<QCapturableWindow> *>(lParam);

    windows.push_back(QCapturableWindowPrivate::create(
        reinterpret_cast<QCapturableWindowPrivate::Id>(hwnd),
        windowTitle(hwnd)));

    return TRUE;
}

QList<QCapturableWindow> QWinCapturableWindows::windows() const
{
    QList<QCapturableWindow> result;

    ::EnumWindows(windowHandler, reinterpret_cast<LPARAM>(&result));

    return result;
}

bool QWinCapturableWindows::isWindowValid(const QCapturableWindowPrivate &window) const
{
    const auto hwnd = reinterpret_cast<HWND>(window.id);
    return isTopLevelWindow(hwnd) && canCaptureWindow(hwnd);
}

q23::expected<QCapturableWindow, QString> QWinCapturableWindows::fromQWindow(QWindow *window) const
{
    const auto hwnd = reinterpret_cast<HWND>(window->winId());
    return QCapturableWindowPrivate::create(
        reinterpret_cast<QCapturableWindowPrivate::Id>(hwnd),
        windowTitle(hwnd));
}
QT_END_NAMESPACE
