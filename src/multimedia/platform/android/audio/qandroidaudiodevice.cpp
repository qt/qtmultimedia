/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
