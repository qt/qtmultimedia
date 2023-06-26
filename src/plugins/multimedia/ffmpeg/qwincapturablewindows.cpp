// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwincapturablewindows_p.h"
#include "private/qcapturablewindow_p.h"

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

static QString windowTitle(HWND hwnd) {
    // QTBUG-114890
    // TODO: investigate the case when hwnd is inner and belows to another thread.
    // It might causes deadlocks in specific cases.
    auto titleLength = ::GetWindowTextLengthW(hwnd);
    std::wstring buffer(titleLength, L'\0');
    titleLength = ::GetWindowTextW(hwnd, buffer.data(), titleLength);
    buffer.resize(titleLength);

    return QString::fromStdWString(buffer);
}

QList<QCapturableWindow> QWinCapturableWindows::windows() const
{
    QList<QCapturableWindow> result;

    auto windowHandler = [](HWND hwnd, LPARAM lParam) {
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
    return hwnd && ::GetAncestor(hwnd, GA_ROOT) == hwnd;
}

QT_END_NAMESPACE
