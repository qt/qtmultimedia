// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qalsamediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"

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

QAlsaMediaDevices::QAlsaMediaDevices()
    : QPlatformMediaDevices()
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

    auto makeDeviceInfo = [&filter, mode](void *entry) -> QAlsaAudioDeviceInfo * {
        unique_str name{ snd_device_name_get_hint(entry, "NAME") };
        if (name && name != "null") {
            unique_str descr{ snd_device_name_get_hint(entry, "DESC") };
            unique_str io{ snd_device_name_get_hint(entry, "IOID") };

            if (descr && (!io || (io == filter))) {
                auto *infop = new QAlsaAudioDeviceInfo{
                    name.get(),
                    QString::fromUtf8(descr.get()),
                    mode,
                };
                return infop;
            }
        }
        return nullptr;
    };

    bool hasDefault = false;
    void **n = hints;
    while (*n != NULL) {
        QAlsaAudioDeviceInfo *infop = makeDeviceInfo(*n++);

        if (infop) {
            devices.append(infop->create());
            if (!hasDefault && infop->id.startsWith("default")) {
                infop->isDefault = true;
                hasDefault = true;
            }
            if (!sysdefault && infop->id.startsWith("sysdefault"))
                sysdefault = infop;
        }
    }

    if (!hasDefault && sysdefault) {
        // Make "sysdefault" the default device if there is no "default" device exists
        sysdefault->isDefault = true;
        hasDefault = true;
    }
    if (!hasDefault && devices.size() > 0) {
        // forcefully declare the first device as "default"
        QAlsaAudioDeviceInfo *infop = makeDeviceInfo(hints[0]);
        if (infop) {
            infop->isDefault = true;
            devices.prepend(infop->create());
        }
    }

    snd_device_name_free_hint(hints);
    return devices;
}

QList<QAudioDevice> QAlsaMediaDevices::audioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QAlsaMediaDevices::audioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QAlsaMediaDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                           QObject *parent)
{
    return new QAlsaAudioSource(deviceInfo.id(), parent);
}

QPlatformAudioSink *QAlsaMediaDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                       QObject *parent)
{
    return new QAlsaAudioSink(deviceInfo.id(), parent);
}

QT_END_NAMESPACE
