/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaudiodeviceinfo_pulse_p.h"
#include "qaudioengine_pulse_p.h"
#include "qpulsehelpers_p.h"

QT_BEGIN_NAMESPACE

QPulseAudioDeviceInfo::QPulseAudioDeviceInfo(const char *device, const char *description, bool isDef, QAudio::Mode mode)
    : QAudioDeviceInfoPrivate(device, mode),
      m_description(QString::fromUtf8(description))
{
    isDefault = isDef;
}

bool QPulseAudioDeviceInfo::isFormatSupported(const QAudioFormat &format) const
{
    pa_sample_spec spec = QPulseAudioInternal::audioFormatToSampleSpec(format);
    return pa_sample_spec_valid(&spec) != 0;
}

QAudioFormat QPulseAudioDeviceInfo::preferredFormat() const
{
    QPulseAudioEngine *pulseEngine = QPulseAudioEngine::instance();
    QAudioFormat format = pulseEngine->m_preferredFormats.value(id);
    return format;
}

QString QPulseAudioDeviceInfo::description() const
{
    return m_description;
}

QList<int> QPulseAudioDeviceInfo::supportedSampleRates() const
{
    return QList<int>() << 8000 << 11025 << 22050 << 44100 << 48000;
}

QList<int> QPulseAudioDeviceInfo::supportedChannelCounts() const
{
    return QList<int>() << 1 << 2 << 4 << 6 << 8;
}

QList<int> QPulseAudioDeviceInfo::supportedSampleSizes() const
{
    return QList<int>() << 8 << 16 << 24 << 32;
}

QList<QAudioFormat::Endian> QPulseAudioDeviceInfo::supportedByteOrders() const
{
    return QList<QAudioFormat::Endian>() << QAudioFormat::BigEndian << QAudioFormat::LittleEndian;
}

QList<QAudioFormat::SampleType> QPulseAudioDeviceInfo::supportedSampleTypes() const
{
    return QList<QAudioFormat::SampleType>() << QAudioFormat::SignedInt << QAudioFormat::UnSignedInt << QAudioFormat::Float;
}

QT_END_NAMESPACE
