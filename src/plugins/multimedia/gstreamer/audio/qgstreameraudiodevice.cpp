// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameraudiodevice_p.h"

#include <qgstutils_p.h>
#include <private/qplatformmediaintegration_p.h>

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

    auto caps = QGstCaps(gst_device_get_caps(gstDevice),QGstCaps::HasRef);
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
