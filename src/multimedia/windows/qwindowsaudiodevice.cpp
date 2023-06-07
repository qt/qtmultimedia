// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of other Qt classes.  This header file may change from version to
// version without notice, or even be removed.
//
// INTERNAL USE ONLY: Do NOT use for any other purpose.
//

#include "qwindowsaudiodevice_p.h"
#include "qwindowsaudioutils_p.h"

#include <QtCore/qt_windows.h>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>

#include <audioclient.h>
#include <mmsystem.h>

#include <initguid.h>
#include <wtypes.h>
#include <propkeydef.h>
#include <mmdeviceapi.h>

QT_BEGIN_NAMESPACE

QWindowsAudioDeviceInfo::QWindowsAudioDeviceInfo(QByteArray dev, ComPtr<IMMDevice> immDev, int waveID, const QString &description, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(dev, mode),
      m_devId(waveID),
      m_immDev(std::move(immDev))
{
    Q_ASSERT(m_immDev);

    this->description = description;

    ComPtr<IAudioClient> audioClient;
    HRESULT hr = m_immDev->Activate(__uuidof(IAudioClient), CLSCTX_INPROC_SERVER, nullptr,
                                    (void **)audioClient.GetAddressOf());
    if (SUCCEEDED(hr)) {
        WAVEFORMATEX *pwfx = nullptr;
        hr = audioClient->GetMixFormat(&pwfx);
        if (SUCCEEDED(hr))
            preferredFormat = QWindowsAudioUtils::waveFormatExToFormat(*pwfx);
    }

    if (!preferredFormat.isValid()) {
        preferredFormat.setSampleRate(44100);
        preferredFormat.setChannelCount(2);
        preferredFormat.setSampleFormat(QAudioFormat::Int16);
    }

    DWORD fmt = 0;

    if(mode == QAudioDevice::Output) {
        WAVEOUTCAPS woc;
        if (waveOutGetDevCaps(m_devId, &woc, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR)
            fmt = woc.dwFormats;
    } else {
        WAVEINCAPS woc;
        if (waveInGetDevCaps(m_devId, &woc, sizeof(WAVEINCAPS)) == MMSYSERR_NOERROR)
            fmt = woc.dwFormats;
    }

    if (!fmt)
        return;

    // Check sample size
    if ((fmt & WAVE_FORMAT_1M08)
        || (fmt & WAVE_FORMAT_1S08)
        || (fmt & WAVE_FORMAT_2M08)
        || (fmt & WAVE_FORMAT_2S08)
        || (fmt & WAVE_FORMAT_4M08)
        || (fmt & WAVE_FORMAT_4S08)
        || (fmt & WAVE_FORMAT_48M08)
        || (fmt & WAVE_FORMAT_48S08)
        || (fmt & WAVE_FORMAT_96M08)
        || (fmt & WAVE_FORMAT_96S08)) {
        supportedSampleFormats.append(QAudioFormat::UInt8);
    }
    if ((fmt & WAVE_FORMAT_1M16)
        || (fmt & WAVE_FORMAT_1S16)
        || (fmt & WAVE_FORMAT_2M16)
        || (fmt & WAVE_FORMAT_2S16)
        || (fmt & WAVE_FORMAT_4M16)
        || (fmt & WAVE_FORMAT_4S16)
        || (fmt & WAVE_FORMAT_48M16)
        || (fmt & WAVE_FORMAT_48S16)
        || (fmt & WAVE_FORMAT_96M16)
        || (fmt & WAVE_FORMAT_96S16)) {
        supportedSampleFormats.append(QAudioFormat::Int16);
    }

    minimumSampleRate = std::numeric_limits<int>::max();
    maximumSampleRate = 0;
    // Check sample rate
    if ((fmt & WAVE_FORMAT_1M08)
       || (fmt & WAVE_FORMAT_1S08)
       || (fmt & WAVE_FORMAT_1M16)
       || (fmt & WAVE_FORMAT_1S16)) {
        minimumSampleRate = qMin(minimumSampleRate, 11025);
        maximumSampleRate = qMax(maximumSampleRate, 11025);
    }
    if ((fmt & WAVE_FORMAT_2M08)
       || (fmt & WAVE_FORMAT_2S08)
       || (fmt & WAVE_FORMAT_2M16)
       || (fmt & WAVE_FORMAT_2S16)) {
        minimumSampleRate = qMin(minimumSampleRate, 22050);
        maximumSampleRate = qMax(maximumSampleRate, 22050);
    }
    if ((fmt & WAVE_FORMAT_4M08)
       || (fmt & WAVE_FORMAT_4S08)
       || (fmt & WAVE_FORMAT_4M16)
       || (fmt & WAVE_FORMAT_4S16)) {
        minimumSampleRate = qMin(minimumSampleRate, 44100);
        maximumSampleRate = qMax(maximumSampleRate, 44100);
    }
    if ((fmt & WAVE_FORMAT_48M08)
        || (fmt & WAVE_FORMAT_48S08)
        || (fmt & WAVE_FORMAT_48M16)
        || (fmt & WAVE_FORMAT_48S16)) {
        minimumSampleRate = qMin(minimumSampleRate, 48000);
        maximumSampleRate = qMax(maximumSampleRate, 48000);
    }
    if ((fmt & WAVE_FORMAT_96M08)
       || (fmt & WAVE_FORMAT_96S08)
       || (fmt & WAVE_FORMAT_96M16)
       || (fmt & WAVE_FORMAT_96S16)) {
        minimumSampleRate = qMin(minimumSampleRate, 96000);
        maximumSampleRate = qMax(maximumSampleRate, 96000);
    }
    if (minimumSampleRate == std::numeric_limits<int>::max())
        minimumSampleRate = 0;

    minimumChannelCount = std::numeric_limits<int>::max();
    maximumChannelCount = 0;
    // Check channel count
    if (fmt & WAVE_FORMAT_1M08
            || fmt & WAVE_FORMAT_1M16
            || fmt & WAVE_FORMAT_2M08
            || fmt & WAVE_FORMAT_2M16
            || fmt & WAVE_FORMAT_4M08
            || fmt & WAVE_FORMAT_4M16
            || fmt & WAVE_FORMAT_48M08
            || fmt & WAVE_FORMAT_48M16
            || fmt & WAVE_FORMAT_96M08
            || fmt & WAVE_FORMAT_96M16) {
        minimumChannelCount = 1;
        maximumChannelCount = 1;
    }
    if (fmt & WAVE_FORMAT_1S08
            || fmt & WAVE_FORMAT_1S16
            || fmt & WAVE_FORMAT_2S08
            || fmt & WAVE_FORMAT_2S16
            || fmt & WAVE_FORMAT_4S08
            || fmt & WAVE_FORMAT_4S16
            || fmt & WAVE_FORMAT_48S08
            || fmt & WAVE_FORMAT_48S16
            || fmt & WAVE_FORMAT_96S08
            || fmt & WAVE_FORMAT_96S16) {
        minimumChannelCount = qMin(minimumChannelCount, 2);
        maximumChannelCount = qMax(maximumChannelCount, 2);
    }

    if (minimumChannelCount == std::numeric_limits<int>::max())
        minimumChannelCount = 0;

    // WAVEOUTCAPS and WAVEINCAPS contains information only for the previously tested parameters.
    // WaveOut and WaveInt might actually support more formats, the only way to know is to try
    // opening the device with it.
    QAudioFormat testFormat;
    testFormat.setChannelCount(maximumChannelCount);
    testFormat.setSampleRate(maximumSampleRate);
    const QAudioFormat defaultTestFormat(testFormat);

    // Check if float samples are supported
    testFormat.setSampleFormat(QAudioFormat::Float);
    if (testSettings(testFormat))
        supportedSampleFormats.append(QAudioFormat::Float);

    // Check channel counts > 2
    testFormat = defaultTestFormat;
    for (int i = 18; i > 2; --i) { // <mmreg.h> defines 18 different channels
        testFormat.setChannelCount(i);
        if (testSettings(testFormat)) {
            maximumChannelCount = i;
            break;
        }
    }

    channelConfiguration = QAudioFormat::defaultChannelConfigForChannelCount(maximumChannelCount);

    ComPtr<IPropertyStore> props;
    hr = m_immDev->OpenPropertyStore(STGM_READ, props.GetAddressOf());
    if (SUCCEEDED(hr)) {
        PROPVARIANT var;
        PropVariantInit(&var);
        hr = props->GetValue(PKEY_AudioEndpoint_PhysicalSpeakers, &var);
        if (SUCCEEDED(hr) && var.uintVal != 0)
            channelConfiguration = QWindowsAudioUtils::maskToChannelConfig(var.uintVal, maximumChannelCount);
    }
}

QWindowsAudioDeviceInfo::~QWindowsAudioDeviceInfo()
{
}

bool QWindowsAudioDeviceInfo::testSettings(const QAudioFormat& format) const
{
    WAVEFORMATEXTENSIBLE wfx;
    if (QWindowsAudioUtils::formatToWaveFormatExtensible(format, wfx)) {
        // query only, do not open device
        if (mode == QAudioDevice::Output) {
            return (waveOutOpen(NULL, UINT_PTR(m_devId), &wfx.Format, 0, 0,
                                WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR);
        } else { // AudioInput
            return (waveInOpen(NULL, UINT_PTR(m_devId), &wfx.Format, 0, 0,
                                WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR);
        }
    }

    return false;
}

QT_END_NAMESPACE
