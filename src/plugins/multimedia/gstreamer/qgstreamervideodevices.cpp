// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideodevices_p.h"
#include "qmediadevices.h"
#include "private/qcameradevice_p.h"

#include "qgstreameraudiosource_p.h"
#include "qgstreameraudiosink_p.h"
#include "qgstreameraudiodevice_p.h"
#include "qgstutils_p.h"

QT_BEGIN_NAMESPACE

static gboolean deviceMonitor(GstBus *, GstMessage *message, gpointer m)
{
    auto *manager = static_cast<QGstreamerVideoDevices *>(m);
    GstDevice *device = nullptr;

    switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_DEVICE_ADDED:
        gst_message_parse_device_added(message, &device);
        manager->addDevice(device);
        break;
    case GST_MESSAGE_DEVICE_REMOVED:
        gst_message_parse_device_removed(message, &device);
        manager->removeDevice(device);
        break;
    default:
        break;
    }
    if (device)
        gst_object_unref (device);

    return G_SOURCE_CONTINUE;
}

QGstreamerVideoDevices::QGstreamerVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    GstDeviceMonitor *monitor;
    GstBus *bus;

    monitor = gst_device_monitor_new();

    gst_device_monitor_add_filter (monitor, nullptr, nullptr);

    bus = gst_device_monitor_get_bus(monitor);
    gst_bus_add_watch(bus, deviceMonitor, this);
    gst_object_unref(bus);

    gst_device_monitor_start(monitor);

    auto devices = gst_device_monitor_get_devices(monitor);

    while (devices) {
        GstDevice *device = static_cast<GstDevice *>(devices->data);
        addDevice(device);
        gst_object_unref(device);
        devices = g_list_delete_link(devices, devices);
    }
}

QList<QCameraDevice> QGstreamerVideoDevices::videoDevices() const
{
    QList<QCameraDevice> devices;

    for (auto device : m_videoSources) {
        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        auto *desc = gst_device_get_display_name(device.gstDevice);
        info->description = QString::fromUtf8(desc);
        g_free(desc);
        info->id = device.id;

        if (QGstStructure properties = gst_device_get_properties(device.gstDevice); !properties.isNull()) {
            auto def = properties["is-default"].toBool();
            info->isDefault = def && *def;
            properties.free();
        }

        if (info->isDefault)
            devices.prepend(info->create());
        else
            devices.append(info->create());

        auto caps = QGstCaps(gst_device_get_caps(device.gstDevice), QGstCaps::HasRef);
        if (!caps.isNull()) {
            QList<QCameraFormat> formats;
            QSet<QSize> photoResolutions;

            int size = caps.size();
            for (int i = 0; i < size; ++i) {
                auto cap = caps.at(i);

                QSize resolution = cap.resolution();
                if (!resolution.isValid())
                    continue;

                auto pixelFormat = cap.pixelFormat();
                auto frameRate = cap.frameRateRange();

                auto *f = new QCameraFormatPrivate{
                    QSharedData(),
                    pixelFormat,
                    resolution,
                    frameRate.min,
                    frameRate.max
                };
                formats << f->create();
                photoResolutions.insert(resolution);
            }
            info->videoFormats = formats;
            // ### sort resolutions?
            info->photoResolutions = photoResolutions.values();
        }
    }
    return devices;
}

void QGstreamerVideoDevices::addDevice(GstDevice *device)
{
    if (gst_device_has_classes(device, "Video/Source")) {
        gst_object_ref(device);
        m_videoSources.push_back({device, QByteArray::number(m_idGenerator)});
        emit videoInputsChanged();
        m_idGenerator++;
    }
}

void QGstreamerVideoDevices::removeDevice(GstDevice *device)
{
    auto it = std::find_if(m_videoSources.begin(), m_videoSources.end(),
                           [=](const QGstDevice &a) { return a.gstDevice == device; });

    if (it != m_videoSources.end()) {
        m_videoSources.erase(it);
        emit videoInputsChanged();
    }

    gst_object_unref(device);
}

GstDevice *QGstreamerVideoDevices::videoDevice(const QByteArray &id) const
{
    auto it = std::find_if(m_videoSources.begin(), m_videoSources.end(),
                           [=](const QGstDevice &a) { return a.id == id; });
    return it != m_videoSources.end() ? it->gstDevice : nullptr;
}

QT_END_NAMESPACE
