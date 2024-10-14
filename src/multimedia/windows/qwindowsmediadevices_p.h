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
#include <private/qcomptr_p.h>
#include <private/qcominitializer_p.h>
#include <private/qwindowsmediafoundation_p.h>

#include <qaudiodevice.h>

struct IAudioClient3;
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
    QPlatformAudioSource *createAudioSource(const QAudioDevice &deviceInfo,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &deviceInfo,
                                        QObject *parent) override;

    void prepareAudio() override;

private:
    QComInitializer m_comInitializer;
    QMFRuntimeInit m_wmfRuntime{ QWindowsMediaFoundation::instance() };
    QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode) const;

    ComPtr<IMMDeviceEnumerator> m_deviceEnumerator;
    ComPtr<CMMNotificationClient> m_notificationClient;
    // The "warm-up" audio client is required to run in the background in order to keep audio engine
    // ready for audio output immediately after creating any other subsequent audio client.
    ComPtr<IAudioClient3> m_warmUpAudioClient;
    std::atomic_bool m_isAudioClientWarmedUp = false;

    friend CMMNotificationClient;
};

QT_END_NAMESPACE

#endif
