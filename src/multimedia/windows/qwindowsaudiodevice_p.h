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
// We mean it.
//

#ifndef QWINDOWSAUDIODEVICE_H
#define QWINDOWSAUDIODEVICE_H

#include <QtCore/qbytearray.h>
#include <QtCore/qstring.h>
#include <QtCore/quuid.h>
#include <QtCore/private/qcomptr_p.h>

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/private/qaudioformat_p.h>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudiodevice_p.h>

#include <mmdeviceapi.h>

struct IMMDevice;

QT_BEGIN_NAMESPACE

namespace QtWASAPI {

struct WindowsProbeData {
    std::pair<int, int> channelCountRange;
    std::pair<int, int> sampleRateRange;
};

struct WindowsFormatResultFutures
{
    std::future<QAudioDevicePrivate::AudioDeviceFormat> formatFuture;
    std::future<WindowsProbeData> probeDataFuture;
};

} // namespace QtWASAPI

class QWindowsAudioDevice final : public QAudioDevicePrivate
{
public:
    QWindowsAudioDevice(QByteArray deviceId, ComPtr<IMMDevice>, QString description,
                        QUuid containerId, EndpointFormFactor, QAudioDevice::Mode);
    QWindowsAudioDevice(QByteArray deviceId, QString description, QUuid containerId,
                        EndpointFormFactor, QAudioDevice::Mode,
                        QtWASAPI::WindowsFormatResultFutures);
    ~QWindowsAudioDevice();

    std::unique_ptr<QAudioDevicePrivate> clone() const;

    ComPtr<IMMDevice> open() const;

    const QUuid m_device_ContainerId;
    const EndpointFormFactor m_formFactor = EndpointFormFactor::UnknownFormFactor;
    std::shared_future<QtWASAPI::WindowsProbeData> m_probeDataFuture;
};

QT_END_NAMESPACE

#endif // QWINDOWSAUDIODEVICE_H
