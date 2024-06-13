// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameraudiodevice_p.h"

#include <common/qgst_p.h>
#include <common/qgstutils_p.h>
#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

QGStreamerAudioDeviceInfo::QGStreamerAudioDeviceInfo(GstDevice *d, const QByteArray &device,
                                                     QAudioDevice::Mode mode)
    : QAudioDevicePrivate(device, mode),
      gstDevice{
          d,
          QGstDeviceHandle::NeedsRef,
      }
{
    QGString name{
        gst_device_get_display_name(gstDevice.get()),
    };
    description = name.toQString();

    auto caps = QGstCaps(gst_device_get_caps(gstDevice.get()), QGstCaps::HasRef);
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

QGStreamerCustomAudioDeviceInfo::QGStreamerCustomAudioDeviceInfo(
        const QByteArray &gstreamerPipeline, QAudioDevice::Mode mode)
    : QAudioDevicePrivate{
          gstreamerPipeline,
          mode,
      }
{
}

QAudioDevice qMakeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline)
{
    auto deviceInfo = std::make_unique<QGStreamerCustomAudioDeviceInfo>(gstreamerPipeline,
                                                                        QAudioDevice::Mode::Input);

    return deviceInfo.release()->create();
}

QAudioDevice qMakeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline)
{
    auto deviceInfo = std::make_unique<QGStreamerCustomAudioDeviceInfo>(gstreamerPipeline,
                                                                        QAudioDevice::Mode::Output);

    return deviceInfo.release()->create();
}

bool isCustomAudioDevice(const QAudioDevicePrivate *device)
{
    return dynamic_cast<const QGStreamerCustomAudioDeviceInfo *>(device);
}

bool isCustomAudioDevice(const QAudioDevice &device)
{
    return isCustomAudioDevice(device.handle());
}

QT_END_NAMESPACE
