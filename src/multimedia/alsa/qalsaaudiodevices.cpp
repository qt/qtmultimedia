// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qalsaaudiodevices_p.h"
#include "qmediadevices.h"

#include "private/qalsaaudiosource_p.h"
#include "private/qalsaaudiosink_p.h"
#include "private/qalsaaudiodevice_p.h"

#include <alsa/asoundlib.h>

QT_BEGIN_NAMESPACE

namespace {

struct free_char
{
    void operator()(char *c) const { ::free(c); }
};

using unique_str = std::unique_ptr<char, free_char>;

bool operator==(const unique_str &str, std::string_view sv)
{
    return std::string_view{ str.get() } == sv;
}
bool operator!=(const unique_str &str, std::string_view sv)
{
    return !(str == sv);
}

} // namespace

QAlsaAudioDevices::QAlsaAudioDevices()
    : QPlatformAudioDevices()
{
}

static QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;

    // Create a list of all current audio devices that support mode
    void **hints;
    if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
        qWarning() << "no alsa devices available";
        return devices;
    }

    std::string_view filter = (mode == QAudioDevice::Input) ? "Input" : "Output";

    QAlsaAudioDeviceInfo *sysdefault = nullptr;

    auto makeDeviceInfo = [&filter, mode](void *entry) -> std::unique_ptr<QAlsaAudioDeviceInfo> {
        unique_str name{ snd_device_name_get_hint(entry, "NAME") };
        if (name && name != "null") {
            unique_str descr{ snd_device_name_get_hint(entry, "DESC") };
            unique_str io{ snd_device_name_get_hint(entry, "IOID") };

            if (descr && (!io || (io == filter))) {
                auto info = std::make_unique<QAlsaAudioDeviceInfo>(
                        name.get(), QString::fromUtf8(descr.get()), mode);
                return info;
            }
        }
        return nullptr;
    };

    bool hasDefault = false;
    void **n = hints;
    while (*n != NULL) {
        std::unique_ptr<QAlsaAudioDeviceInfo> info = makeDeviceInfo(*n++);

        if (info) {
            if (!hasDefault && info->id.startsWith("default")) {
                info->isDefault = true;
                hasDefault = true;
            }
            if (!sysdefault && info->id.startsWith("sysdefault"))
                sysdefault = info.get();
            devices.append(QAudioDevicePrivate::createQAudioDevice(std::move(info)));
        }
    }

    if (!hasDefault && sysdefault) {
        // Make "sysdefault" the default device if there is no "default" device exists
        sysdefault->isDefault = true;
        hasDefault = true;
    }
    if (!hasDefault && devices.size() > 0) {
        // forcefully declare the first device as "default"
        std::unique_ptr<QAlsaAudioDeviceInfo> info = makeDeviceInfo(hints[0]);
        if (info) {
            info->isDefault = true;
            devices.prepend(QAudioDevicePrivate::createQAudioDevice(std::move(info)));
        }
    }

    snd_device_name_free_hint(hints);
    return devices;
}

QList<QAudioDevice> QAlsaAudioDevices::findAudioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QAlsaAudioDevices::findAudioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QAlsaAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                           const QAudioFormat &fmt,
                                                           QObject *parent)
{
    return new QAlsaAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QAlsaAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                       const QAudioFormat &fmt,
                                                       QObject *parent)
{
    return new QAlsaAudioSink(deviceInfo, fmt, parent);
}

QT_END_NAMESPACE
