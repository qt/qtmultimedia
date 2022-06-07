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

    for (auto *d : qAsConst(m_videoSources)) {
        QGstStructure properties = gst_device_get_properties(d);
        if (!properties.isNull()) {
            QCameraDevicePrivate *info = new QCameraDevicePrivate;
            auto *desc = gst_device_get_display_name(d);
            info->description = QString::fromUtf8(desc);
            g_free(desc);

            info->id = properties["device.path"].toString();
            auto def = properties["is-default"].toBool();
            info->isDefault = def && *def;
            if (def)
                devices.prepend(info->create());
            else
                devices.append(info->create());
            properties.free();
            QGstCaps caps = gst_device_get_caps(d);
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
    }
    return devices;
}

void QGstreamerVideoDevices::addDevice(GstDevice *device)
{
    gchar *type = gst_device_get_device_class(device);
//    qDebug() << "adding device:" << device << type << gst_device_get_display_name(device) << gst_structure_to_string(gst_device_get_properties(device));
    gst_object_ref(device);
    if (!strcmp(type, "Video/Source") || !strcmp(type, "Source/Video")) {
        m_videoSources.insert(device);
        videoInputsChanged();
    } else {
        gst_object_unref(device);
    }
    g_free(type);
}

void QGstreamerVideoDevices::removeDevice(GstDevice *device)
{
//    qDebug() << "removing device:" << device << gst_device_get_display_name(device);
    if (m_videoSources.remove(device))
        videoInputsChanged();

    gst_object_unref(device);
}

static GstDevice *getDevice(const QSet<GstDevice *> &devices, const char *key, const QByteArray &id)
{
    GstDevice *gstDevice = nullptr;
    for (auto *d : devices) {
        QGstStructure properties = gst_device_get_properties(d);
        if (!properties.isNull()) {
            auto *name = properties[key].toString();
            if (id == name) {
                gstDevice = d;
            }
        }
        properties.free();
        if (gstDevice)
            break;
    }
    return gstDevice;
}

GstDevice *QGstreamerVideoDevices::videoDevice(const QByteArray &id) const
{
    return getDevice(m_videoSources, "device.path", id);
}

QT_END_NAMESPACE
