// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSMEDIADEVICES_H
#define QWINDOWSMEDIADEVICES_H

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

#include <private/qplatformmediadevices_p.h>
#include <private/qwindowsiupointer_p.h>

#include <qaudiodevice.h>

struct IMMDeviceEnumerator;

QT_BEGIN_NAMESPACE

class QWindowsEngine;
class CMMNotificationClient;

class QWindowsMediaDevices : public QPlatformMediaDevices
{
public:
    QWindowsMediaDevices();
    virtual ~QWindowsMediaDevices();

    QList<QAudioDevice> audioInputs() const override;
    QList<QAudioDevice> audioOutputs() const override;
    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo) override;

private:
    QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode) const;

    QWindowsIUPointer<IMMDeviceEnumerator> m_deviceEnumerator;
    QWindowsIUPointer<CMMNotificationClient> m_notificationClient;

    friend CMMNotificationClient;
};

QT_END_NAMESPACE

#endif
