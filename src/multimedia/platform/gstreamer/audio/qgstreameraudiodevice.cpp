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

#include "qgstreameraudiodevice_p.h"

#include <private/qgstutils_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qgstreamermediadevices_p.h>

QT_BEGIN_NAMESPACE

QGStreamerAudioDeviceInfo::QGStreamerAudioDeviceInfo(GstDevice *d, const QByteArray &device, QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode),
      gstDevice(d)
{
    Q_ASSERT(gstDevice);
    gst_object_ref(gstDevice);

    auto *n = gst_device_get_display_name(gstDevice);
    description = QString::fromUtf8(n);
    g_free(n);

    QGstCaps caps = gst_device_get_caps(gstDevice);
    int size = caps.size();
    for (int i = 0; i < size; ++i) {
        auto c = caps.at(i);
        if (c.name() == "audio/x-raw") {
            auto rate = c["rate"].toIntRange();
            if (rate) {
                minimumSampleRate = rate->min;
                maximumSampleRate = rate->max;
            }
            auto channels = c["channels"].toIntRange();
            if (channels) {
                minimumChannelCount = channels->min;
                maximumChannelCount = channels->max;
            }
            supportedSampleFormats = c["format"].getSampleFormats();
        }
    }

    preferredFormat.setChannelCount(qBound(minimumChannelCount, 2, maximumChannelCount));
    preferredFormat.setSampleRate(qBound(minimumSampleRate, 48000, maximumSampleRate));
    QAudioFormat::SampleFormat f = QAudioFormat::Int16;
    if (!supportedSampleFormats.contains(f))
        f = supportedSampleFormats.value(0, QAudioFormat::Unknown);
    preferredFormat.setSampleFormat(f);
}

QGStreamerAudioDeviceInfo::~QGStreamerAudioDeviceInfo()
{
    if (gstDevice)
        gst_object_unref(gstDevice);
}

QT_END_NAMESPACE
