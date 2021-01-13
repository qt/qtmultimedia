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

#include "qaudiodeviceinfo_gstreamer_p.h"
#include "qaudioengine_gstreamer_p.h"

#include <private/qgstutils_p.h>
#include <private/qmediaplatformintegration_p.h>
#include <private/qgstreamerplatformdevicemanager_p.h>

QT_BEGIN_NAMESPACE

QGStreamerAudioDeviceInfo::QGStreamerAudioDeviceInfo(const QByteArray &device, QAudio::Mode mode)
    : QAudioDeviceInfoPrivate(device, mode)
{
    auto *deviceManager = static_cast<QGstreamerPlatformDeviceManager *>(QMediaPlatformIntegration::instance()->deviceManager());
    gstDevice = deviceManager->audioDevice(device, mode);
    if (gstDevice) {
        gst_object_ref(gstDevice);
        auto *n = gst_device_get_display_name(gstDevice);
        m_description = QString::fromUtf8(n);
        g_free(n);
    }
}

QGStreamerAudioDeviceInfo::~QGStreamerAudioDeviceInfo()
{
    if (gstDevice)
        gst_object_unref(gstDevice);
}

bool QGStreamerAudioDeviceInfo::isFormatSupported(const QAudioFormat &) const
{
    // ####
    return true;
}

QAudioFormat QGStreamerAudioDeviceInfo::preferredFormat() const
{
    QAudioFormat format;
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setChannelCount(2);
    format.setSampleRate(48000);
    format.setSampleSize(16);
    return format;
}

QString QGStreamerAudioDeviceInfo::description() const
{
    return m_description;
}

QStringList QGStreamerAudioDeviceInfo::supportedCodecs() const
{
    return QStringList() << QString::fromLatin1("audio/x-raw");
}

QList<int> QGStreamerAudioDeviceInfo::supportedSampleRates() const
{
    return QList<int>() << 8000 << 11025 << 22050 << 44100 << 48000;
}

QList<int> QGStreamerAudioDeviceInfo::supportedChannelCounts() const
{
    return QList<int>() << 1 << 2 << 4 << 6 << 8;
}

QList<int> QGStreamerAudioDeviceInfo::supportedSampleSizes() const
{
    return QList<int>() << 8 << 16 << 24 << 32;
}

QList<QAudioFormat::Endian> QGStreamerAudioDeviceInfo::supportedByteOrders() const
{
    return QList<QAudioFormat::Endian>() << QAudioFormat::BigEndian << QAudioFormat::LittleEndian;
}

QList<QAudioFormat::SampleType> QGStreamerAudioDeviceInfo::supportedSampleTypes() const
{
    return QList<QAudioFormat::SampleType>() << QAudioFormat::SignedInt << QAudioFormat::UnSignedInt << QAudioFormat::Float;
}

QT_END_NAMESPACE
