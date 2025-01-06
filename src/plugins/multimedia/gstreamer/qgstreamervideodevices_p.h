// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERMEDIADEVICES_H
#define QGSTREAMERMEDIADEVICES_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformvideodevices_p.h>
#include <gst/gst.h>
#include <qaudiodevice.h>
#include <vector>

#include <common/qgst_handle_types_p.h>
#include <common/qgst_bus_observer_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerVideoDevices final : public QPlatformVideoDevices,
                                     private QGstreamerBusMessageFilter
{
public:
    explicit QGstreamerVideoDevices(QPlatformMediaIntegration *integration);
    ~QGstreamerVideoDevices() override;

    GstDevice *videoDevice(const QByteArray &id) const;

    void addDevice(QGstDeviceHandle);
    void removeDevice(QGstDeviceHandle);

protected:
    QList<QCameraDevice> findVideoInputs() const override;

private:
    bool processBusMessage(const QGstreamerMessage &message) override;

    struct QGstRecordDevice
    {
        QGstDeviceHandle gstDevice;
        QByteArray id;
    };

    quint64 m_idGenerator = 0;
    std::vector<QGstRecordDevice> m_videoSources;

    QGstDeviceMonitorHandle m_deviceMonitor;
    QGstBusObserver m_busObserver;
};

QT_END_NAMESPACE

#endif
