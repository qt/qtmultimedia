// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSVIDEODEVICES_H
#define QWINDOWSVIDEODEVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformvideodevices_p.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

LRESULT QT_WIN_CALLBACK deviceNotificationWndProc(HWND, UINT, WPARAM, LPARAM);

class QWindowsVideoDevices : public QPlatformVideoDevices
{
public:
    QWindowsVideoDevices(QPlatformMediaIntegration *integration);
    ~QWindowsVideoDevices();

    QList<QCameraDevice> videoDevices() const override;

private:
    HWND m_videoDeviceMsgWindow = nullptr;
    HDEVNOTIFY m_videoDeviceNotification = nullptr;

    friend LRESULT QT_WIN_CALLBACK deviceNotificationWndProc(HWND, UINT, WPARAM, LPARAM);
};



QT_END_NAMESPACE

#endif
