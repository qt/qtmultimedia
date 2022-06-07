// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidaudiodevice_p.h"

#include "qopenslesengine_p.h"

QT_BEGIN_NAMESPACE

QOpenSLESDeviceInfo::QOpenSLESDeviceInfo(const QByteArray &device, const QString &desc, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode),
      m_engine(QOpenSLESEngine::instance())
{
    description = desc;

    auto channels = m_engine->supportedChannelCounts(mode);
    if (channels.size()) {
        minimumChannelCount = channels.first();
        maximumChannelCount = channels.last();
    }

    auto sampleRates = m_engine->supportedSampleRates(mode);
    if (sampleRates.size()) {
        minimumSampleRate = sampleRates.first();
        maximumSampleRate = sampleRates.last();
    }
    if (mode == QAudioDevice::Input)
        supportedSampleFormats.append(QAudioFormat::UInt8);
    supportedSampleFormats.append(QAudioFormat::Int16);

    preferredFormat.setChannelCount(2);
    preferredFormat.setSampleRate(48000);
    QAudioFormat::SampleFormat f = QAudioFormat::Int16;
    if (!supportedSampleFormats.contains(f))
        f = supportedSampleFormats.value(0, QAudioFormat::Unknown);
    preferredFormat.setSampleFormat(f);
}

QT_END_NAMESPACE
