// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwincapturablewindows_p.h"
#include "private/qcapturablewindow_p.h"

#include <qt_windows.h>

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

QList<QCapturableWindow> QWinCapturableWindows::windows() const
{
    QList<QCapturableWindow> result;

    auto windowHandler = [](HWND hwnd, LPARAM lParam) {
        if (!canCaptureWindow(hwnd))
            return TRUE; // Ignore window and continue enumerating

        auto& windows = *reinterpret_cast<QList<QCapturableWindow>*>(lParam);

        auto windowData = std::make_unique<QCapturableWindowPrivate>();
        windowData->id = reinterpret_cast<QCapturableWindowPrivate::Id>(hwnd);
        windowData->description = windowTitle(hwnd);
        windows.push_back(windowData.release()->create());

        return TRUE;
    };

    ::EnumWindows(windowHandler, reinterpret_cast<LPARAM>(&result));

    return result;
}

bool QWinCapturableWindows::isWindowValid(const QCapturableWindowPrivate &window) const
{
    const auto hwnd = reinterpret_cast<HWND>(window.id);
    return isTopLevelWindow(hwnd) && canCaptureWindow(hwnd);
}

QT_END_NAMESPACE
