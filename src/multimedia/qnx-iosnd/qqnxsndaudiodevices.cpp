// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxsndaudiodevices_p.h"
#include "qmediadevices.h"
#include "qqnxsndhelpers_p.h"

#include "private/qqnxsndaudiosource_p.h"
#include "private/qqnxsndaudiosink_p.h"
#include "private/qqnxsndaudiodevice_p.h"

#include <alsa/asoundlib.h>

#include <QtCore/qdir.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

// Declared in qqnxsndaudiodevice_p.h; shared with the device-info TU.
Q_LOGGING_CATEGORY(lcQnxSndDevices, "qt.multimedia.qnxsnd.devices");

namespace {

struct free_char
{
    void operator()(char *c) const { ::free(c); }
};

using unique_str = std::unique_ptr<char, free_char>;

bool operator==(const unique_str &str, std::string_view sv)
{
    return str && std::string_view{ str.get() } == sv;
}
bool operator!=(const unique_str &str, std::string_view sv)
{
    return !(str == sv);
}

QList<QAudioDevice> availableDevicesViaHints(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;

    void **rawHints;
    if (snd_device_name_hint(-1, "pcm", &rawHints) < 0) {
        qCWarning(lcQnxSndDevices) << "snd_device_name_hint failed";
        return devices;
    }
    QnxSndHelpers::HintsGuard hints(rawHints);

    std::string_view filter = (mode == QAudioDevice::Input) ? "Input" : "Output";

    auto makeDeviceInfo = [filter, mode](void *entry) -> std::unique_ptr<QQnxSndAudioDeviceInfo> {
        unique_str name(snd_device_name_get_hint(entry, "NAME"));
        if (name && name != "null") {
            unique_str descr(snd_device_name_get_hint(entry, "DESC"));
            unique_str io(snd_device_name_get_hint(entry, "IOID"));

            if (descr && (!io || (io == filter))) {
                auto info = std::make_unique<QQnxSndAudioDeviceInfo>(
                        name.get(), QString::fromUtf8(descr.get()), mode);
                return info;
            }
        }
        return nullptr;
    };

    // QList cannot host move-only unique_ptr<T> because detach() copy-appends.
    // Stage in std::vector, then move into the QList<QAudioDevice>.
    std::vector<std::unique_ptr<QQnxSndAudioDeviceInfo>> pending;

    for (void **n = hints.get(); *n != nullptr; ++n) {
        if (auto info = makeDeviceInfo(*n))
            pending.push_back(std::move(info));
    }

    // snd_device_name_hint enumeration order is not contractually stable
    // across calls, so sort by id before picking a default. That way the
    // "first match" semantics (and the position-0 fallback) are reproducible
    // and QMediaDevices::defaultAudioOutput() returns the same device on
    // repeated queries.
    std::sort(pending.begin(), pending.end(),
              [](const auto &a, const auto &b) { return a->id < b->id; });

    qsizetype defaultIdx = -1;
    qsizetype preferredIdx = -1;
    for (qsizetype i = 0; i < static_cast<qsizetype>(pending.size()); ++i) {
        if (defaultIdx < 0 && pending[i]->id.startsWith("default"))
            defaultIdx = i;
        if (preferredIdx < 0 && pending[i]->id.contains("pcmPreferred"))
            preferredIdx = i;
    }

    const qsizetype chosen = defaultIdx >= 0 ? defaultIdx
            : preferredIdx >= 0              ? preferredIdx
            : pending.empty()                ? -1
                                             : 0;
    if (chosen >= 0)
        pending[chosen]->isDefault = true;

    devices.reserve(pending.size());
    // Explicitly move each unique_ptr out of the staging vector into the QList.
    for (auto &&info : pending)
        devices.append(QAudioDevicePrivate::createQAudioDevice(std::move(info)));

    return devices;
}

bool hasDeviceFilesForMode(QAudioDevice::Mode mode)
{
    QDir dir(QStringLiteral("/dev/snd"));
    const char16_t suffix = mode == QAudioDevice::Input ? u'c' : u'p';
    for (const QString &entry : dir.entryList(QDir::Files)) {
        if (entry.startsWith(QStringLiteral("pcm")) && entry.back() == suffix)
            return true;
    }
    return false;
}

} // namespace

QQnxSndAudioDevices::QQnxSndAudioDevices() = default;

static QList<QAudioDevice> availableDevices(QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices = availableDevicesViaHints(mode);

    // snd_device_name_hint on QNX returns devices without IOID, causing them
    // to pass the filter for both Input and Output modes. Validate against
    // /dev/snd/pcm*{c,p} entries to discard phantom devices on systems that
    // lack the hardware for the requested mode.
    if (!devices.isEmpty() && !hasDeviceFilesForMode(mode)) {
        qCDebug(lcQnxSndDevices) << "No /dev/snd/ device files for"
                                 << (mode == QAudioDevice::Input ? "capture" : "playback")
                                 << "- discarding" << devices.size() << "hint-reported device(s)";
        devices.clear();
    }

    if (devices.isEmpty())
        qCWarning(lcQnxSndDevices) << "No audio devices found for"
                                   << (mode == QAudioDevice::Input ? "input" : "output");
    return devices;
}

QList<QAudioDevice> QQnxSndAudioDevices::findAudioInputs() const
{
    return availableDevices(QAudioDevice::Input);
}

QList<QAudioDevice> QQnxSndAudioDevices::findAudioOutputs() const
{
    return availableDevices(QAudioDevice::Output);
}

QPlatformAudioSource *QQnxSndAudioDevices::createAudioSource(const QAudioDevice &deviceInfo,
                                                             const QAudioFormat &fmt,
                                                             QObject *parent)
{
    return new QQnxSndAudioSource(deviceInfo, fmt, parent);
}

QPlatformAudioSink *QQnxSndAudioDevices::createAudioSink(const QAudioDevice &deviceInfo,
                                                          const QAudioFormat &fmt,
                                                          QObject *parent)
{
    return new QQnxSndAudioSink(deviceInfo, fmt, parent);
}

QT_END_NAMESPACE
