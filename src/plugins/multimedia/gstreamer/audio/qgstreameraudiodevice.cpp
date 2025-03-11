// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameraudiodevice_p.h"

#include <common/qgst_p.h>
#include <common/qgstutils_p.h>
#include <private/qplatformmediaintegration_p.h>

QT_BEGIN_NAMESPACE

QGStreamerCustomAudioDeviceInfo::QGStreamerCustomAudioDeviceInfo(const QByteArray &gstreamerPipeline,
                                                                 QAudioDevice::Mode mode)
    : QAudioDevicePrivate{
          gstreamerPipeline,
          mode,
          QString::fromUtf8(gstreamerPipeline),
      }
{
}

QAudioDevice qMakeCustomGStreamerAudioInput(const QByteArray &gstreamerPipeline)
{
    auto deviceInfo = std::make_unique<QGStreamerCustomAudioDeviceInfo>(gstreamerPipeline,
                                                                        QAudioDevice::Mode::Input);

    return QAudioDevicePrivate::createQAudioDevice(std::move(deviceInfo));
}

QAudioDevice qMakeCustomGStreamerAudioOutput(const QByteArray &gstreamerPipeline)
{
    auto deviceInfo = std::make_unique<QGStreamerCustomAudioDeviceInfo>(gstreamerPipeline,
                                                                        QAudioDevice::Mode::Output);

    return QAudioDevicePrivate::createQAudioDevice(std::move(deviceInfo));
}

bool isCustomAudioDevice(const QAudioDevicePrivate *device)
{
    return dynamic_cast<const QGStreamerCustomAudioDeviceInfo *>(device);
}

bool isCustomAudioDevice(const QAudioDevice &device)
{
    return isCustomAudioDevice(QAudioDevicePrivate::handle(device));
}

QT_END_NAMESPACE
