// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSAUDIODEVICES_P_H
#define QOHOSAUDIODEVICES_P_H

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

#include <private/qplatformaudiodevices_p.h>

#include <ohaudio/native_audio_device_base.h>

QT_BEGIN_NAMESPACE

class QOhosAudioDevices : public QPlatformAudioDevices
{
public:
    QOhosAudioDevices();
    ~QOhosAudioDevices() override;

    QPlatformAudioSource *createAudioSource(const QAudioDevice &, const QAudioFormat &,
                                            QObject *parent) override;
    QPlatformAudioSink *createAudioSink(const QAudioDevice &, const QAudioFormat &,
                                        QObject *parent) override;

    QLatin1String backendName() const override { return QLatin1String{ "OHAudio" }; }
    bool hasCallbackApi() const override { return true; }

protected:
    QList<QAudioDevice> findAudioInputs() const override;
    QList<QAudioDevice> findAudioOutputs() const override;

private:
    void registerDeviceChangeCallbacks();
    void unregisterDeviceChangeCallbacks();

    static int32_t onInputDevicesChanged(OH_AudioDevice_ChangeType type,
                                         OH_AudioDeviceDescriptorArray *devices);
    static int32_t onOutputDevicesChanged(OH_AudioDevice_ChangeType type,
                                          OH_AudioDeviceDescriptorArray *devices);

    bool m_deviceChangeCallbacksRegistered = false;
};

QT_END_NAMESPACE

#endif // QOHOSAUDIODEVICES_P_H
