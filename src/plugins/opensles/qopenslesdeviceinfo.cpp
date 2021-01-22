/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
****************************************************************************/

#include "qopenslesdeviceinfo.h"

#include "qopenslesengine.h"

QT_BEGIN_NAMESPACE

QOpenSLESDeviceInfo::QOpenSLESDeviceInfo(const QByteArray &device, QAudio::Mode mode)
    : m_engine(QOpenSLESEngine::instance())
    , m_device(device)
    , m_mode(mode)
{
}

bool QOpenSLESDeviceInfo::isFormatSupported(const QAudioFormat &format) const
{
    QOpenSLESDeviceInfo *that = const_cast<QOpenSLESDeviceInfo*>(this);
    return that->supportedCodecs().contains(format.codec())
            && that->supportedSampleRates().contains(format.sampleRate())
            && that->supportedChannelCounts().contains(format.channelCount())
            && that->supportedSampleSizes().contains(format.sampleSize())
            && that->supportedByteOrders().contains(format.byteOrder())
            && that->supportedSampleTypes().contains(format.sampleType());
}

QAudioFormat QOpenSLESDeviceInfo::preferredFormat() const
{
    QAudioFormat format;
    format.setCodec(QStringLiteral("audio/pcm"));
    format.setSampleSize(16);
    format.setSampleType(QAudioFormat::SignedInt);
    format.setSampleRate(QOpenSLESEngine::getOutputValue(QOpenSLESEngine::SampleRate, 48000));
    format.setChannelCount(m_mode == QAudio::AudioInput ? 1 : 2);
    return format;
}

QString QOpenSLESDeviceInfo::deviceName() const
{
    return m_device;
}

QStringList QOpenSLESDeviceInfo::supportedCodecs()
{
    return QStringList() << QStringLiteral("audio/pcm");
}

QList<int> QOpenSLESDeviceInfo::supportedSampleRates()
{
    return m_engine->supportedSampleRates(m_mode);
}

QList<int> QOpenSLESDeviceInfo::supportedChannelCounts()
{
    return m_engine->supportedChannelCounts(m_mode);
}

QList<int> QOpenSLESDeviceInfo::supportedSampleSizes()
{
    if (m_mode == QAudio::AudioInput)
        return QList<int>() << 16;
    else
        return QList<int>() << 8 << 16;
}

QList<QAudioFormat::Endian> QOpenSLESDeviceInfo::supportedByteOrders()
{
    return QList<QAudioFormat::Endian>() << QAudioFormat::LittleEndian;
}

QList<QAudioFormat::SampleType> QOpenSLESDeviceInfo::supportedSampleTypes()
{
    return QList<QAudioFormat::SampleType>() << QAudioFormat::SignedInt;
}

QT_END_NAMESPACE
