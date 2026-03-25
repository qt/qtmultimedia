// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqnxsndaudiodevice_p.h"
#include "qqnxsndhelpers_p.h"

#include <QtMultimedia/private/qaudioformat_p.h>

#include <alsa/asoundlib.h>

#include <QtCore/qloggingcategory.h>

#include <algorithm>

QT_BEGIN_NAMESPACE

// lcQnxSndDevices is declared in qqnxsndaudiodevice_p.h and defined in
// qqnxsndaudiodevices.cpp; the device-info and enumeration TUs share it.

namespace {

// Conservative format returned when the device's parameter space can't be probed.
// io-snd is non-exclusive, so any openable device is probed for its real caps; this
// is reached only for a hint-reported phantom with no backing hardware (whose device
// is then discarded by the /dev/snd cross-check in qqnxsndaudiodevices.cpp) and for
// the offline unit test, which uses a non-openable id to exercise this path.
QAudioDevicePrivate::AudioDeviceFormat defaultDeviceFormat(QAudioDevice::Mode mode)
{
    QAudioDevicePrivate::AudioDeviceFormat format;

    format.minimumChannelCount = 1;
    format.maximumChannelCount = 2;

    format.minimumSampleRate = 8000;
    format.maximumSampleRate = 48000;

    format.supportedSampleFormats = {
        QAudioFormat::UInt8,
        QAudioFormat::Int16,
        QAudioFormat::Int32,
        QAudioFormat::Float,
    };

    format.preferredFormat.setChannelCount(mode == QAudioDevice::Input ? 1 : 2);
    format.preferredFormat.setSampleFormat(QAudioFormat::Float);
    format.preferredFormat.setSampleRate(48000);

    return format;
}

// Query the device's real capabilities by opening it and inspecting the ALSA
// hw_params space, in the spirit of the WASAPI backend's IsFormatSupported probe.
// SALSA on QNX has no plugin/resampling layer, so the hint-reported format must
// reflect what the hardware actually accepts. On any failure we fall back to the
// conservative defaults rather than dropping the device.
QAudioDevicePrivate::AudioDeviceFormat probeDeviceFormat(const QByteArray &dev,
                                                         QAudioDevice::Mode mode)
{
    QAudioDevicePrivate::AudioDeviceFormat format = defaultDeviceFormat(mode);

    const snd_pcm_stream_t direction =
            mode == QAudioDevice::Input ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;

    // Non-blocking open: probing only reads the parameter space and must never
    // stall enumeration if the device is busy.
    snd_pcm_t *handle = nullptr;
    if (int err = snd_pcm_open(&handle, dev.constData(), direction, SND_PCM_NONBLOCK); err < 0) {
        qCDebug(lcQnxSndDevices) << "probe: cannot open" << dev << ":" << snd_strerror(err)
                                 << "- using default format";
        return format;
    }

    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_alloca(&hwparams);
    if (int err = snd_pcm_hw_params_any(handle, hwparams); err < 0) {
        qCDebug(lcQnxSndDevices) << "probe: hw_params_any failed for" << dev << ":"
                                 << snd_strerror(err) << "- using default format";
        snd_pcm_close(handle);
        return format;
    }

    // Sample rate range: test the standard rates against the device and keep the
    // span of supported ones. (get_rate_max reports an unbounded value on io-snd.)
    QList<int> supportedRates;
    for (int rate : QtMultimediaPrivate::allSupportedSampleRates) {
        if (snd_pcm_hw_params_test_rate(handle, hwparams, static_cast<unsigned>(rate), 0) == 0)
            supportedRates.append(rate);
    }
    if (!supportedRates.isEmpty()) {
        // allSupportedSampleRates is ascending, so first/last bound the range.
        format.minimumSampleRate = supportedRates.first();
        format.maximumSampleRate = supportedRates.last();
    }

    // io-snd converts sample formats internally; combined with the backend's
    // "always best-native + convert" open strategy (see openConfiguredPcm), every
    // QAudioFormat sample format is usable regardless of the device's native
    // format, so advertise them all. The probe below is kept only to report the
    // device-native formats in the debug log.
    QList<QAudioFormat::SampleFormat> nativeFormats;
    for (QAudioFormat::SampleFormat sf : { QAudioFormat::UInt8, QAudioFormat::Int16,
                                           QAudioFormat::Int32, QAudioFormat::Float }) {
        const snd_pcm_format_t pcmFormat = QnxSndHelpers::mapSampleFormat(sf);
        if (pcmFormat != SND_PCM_FORMAT_UNKNOWN
            && snd_pcm_hw_params_test_format(handle, hwparams, pcmFormat) == 0) {
            nativeFormats.append(sf);
        }
    }
    format.supportedSampleFormats = qAllSupportedSampleFormats();

    // Channel count range.
    unsigned int minChannels = 0;
    unsigned int maxChannels = 0;
    if (snd_pcm_hw_params_get_channels_min(hwparams, &minChannels) == 0
        && snd_pcm_hw_params_get_channels_max(hwparams, &maxChannels) == 0 && maxChannels > 0) {
        format.minimumChannelCount = static_cast<int>(minChannels);
        format.maximumChannelCount = static_cast<int>(maxChannels);
    }

    // Float is always usable (converted to the device-native format on open), so
    // it is the preferred application format.
    format.preferredFormat.setSampleFormat(QAudioFormat::Float);

    // Prefer 48 kHz, else the supported rate closest to it (logarithmic distance).
    const int preferredRate = supportedRates.isEmpty()
            ? ((48000 >= format.minimumSampleRate && 48000 <= format.maximumSampleRate)
                       ? 48000
                       : format.maximumSampleRate)
            : QtMultimediaPrivate::findClosestSamplingRate(48000, QSpan<const int>{ supportedRates });
    format.preferredFormat.setSampleRate(preferredRate);

    const int preferredChannels = mode == QAudioDevice::Input ? 1 : 2;
    format.preferredFormat.setChannelCount(
            std::clamp(preferredChannels, format.minimumChannelCount, format.maximumChannelCount));

    if (const int err = snd_pcm_close(handle); err < 0)
        qCDebug(lcQnxSndDevices) << "probe: snd_pcm_close failed:" << snd_strerror(err);

    qCDebug(lcQnxSndDevices) << "probe:" << dev << "rate" << format.minimumSampleRate << "-"
                             << format.maximumSampleRate << "ch" << format.minimumChannelCount << "-"
                             << format.maximumChannelCount << "native formats" << nativeFormats
                             << "(advertising all)";
    return format;
}

} // namespace

QQnxSndAudioDeviceInfo::QQnxSndAudioDeviceInfo(const QByteArray &dev, const QString &desc,
                                               QAudioDevice::Mode mode)
    : QAudioDevicePrivate(dev, mode, desc, false, probeDeviceFormat(dev, mode))
{
}

QQnxSndAudioDeviceInfo::~QQnxSndAudioDeviceInfo() = default;

QT_END_NAMESPACE
