/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qgstreamermediadevices_p.h"
#include "qmediadevices.h"
#include "qcameradevice_p.h"

#include "private/qgstreameraudiosource_p.h"
#include "private/qgstreameraudiosink_p.h"
#include "private/qgstreameraudiodevice_p.h"
#include "private/qgstutils_p.h"

QT_BEGIN_NAMESPACE

static gboolean deviceMonitor(GstBus *, GstMessage *message, gpointer m)
{
    QGstreamerMediaDevices *manager = static_cast<QGstreamerMediaDevices *>(m);
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

QGstreamerMediaDevices::QGstreamerMediaDevices()
    : QPlatformMediaDevices()
{
    GstDeviceMonitor *monitor;
    GstBus *bus;

    monitor = gst_device_monitor_new();

    gst_device_monitor_add_filter (monitor, "Video/Source", nullptr);
    gst_device_monitor_add_filter (monitor, "Audio/Source", nullptr);
    gst_device_monitor_add_filter (monitor, "Audio/Sink", nullptr);

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

static QList<QAudioDevice> devicesFromSet(const QSet<GstDevice *> &deviceSet, QAudioDevice::Mode mode)
{
    QList<QAudioDevice> devices;
    for (auto *d : deviceSet) {
        auto *properties = gst_device_get_properties(d);
        if (properties) {
            auto *klass = gst_structure_get_string(properties, "device.class");
            if (qstrcmp(klass, "monitor")) {
                auto *name = gst_structure_get_string(properties, "device.bus_path");
                gboolean def;
                auto *info = new QGStreamerAudioDeviceInfo(d, name, mode);
                if (gst_structure_get_boolean(properties, "is-default", &def) && def)
                    devices.prepend(info->create());
                else
                    devices.append(info->create());
            }

            gst_structure_free(properties);
        }
    }
    return devices;
};

QList<QAudioDevice> QGstreamerMediaDevices::audioInputs() const
{
    return devicesFromSet(m_audioSources, QAudioDevice::Input);
}

QList<QAudioDevice> QGstreamerMediaDevices::audioOutputs() const
{
    return devicesFromSet(m_audioSinks, QAudioDevice::Output);
}

QList<QCameraDevice> QGstreamerMediaDevices::videoInputs() const
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

QPlatformAudioSource *QGstreamerMediaDevices::createAudioSource(const QAudioDevice &deviceInfo)
{
    return new QGStreamerAudioSource(deviceInfo);
}

QPlatformAudioSink *QGstreamerMediaDevices::createAudioSink(const QAudioDevice &deviceInfo)
{
    return new QGStreamerAudioSink(deviceInfo);
}

void QGstreamerMediaDevices::addDevice(GstDevice *device)
{
    gchar *type = gst_device_get_device_class(device);
//    qDebug() << "adding device:" << device << type << gst_device_get_display_name(device) << gst_structure_to_string(gst_device_get_properties(device));
    gst_object_ref(device);
    if (!strcmp(type, "Video/Source")) {
        m_videoSources.insert(device);
        videoInputsChanged();
    } else if (!strcmp(type, "Audio/Source")) {
        m_audioSources.insert(device);
        audioInputsChanged();
    } else if (!strcmp(type, "Audio/Sink")) {
        m_audioSinks.insert(device);
        audioOutputsChanged();
    } else {
        gst_object_unref(device);
    }
    g_free(type);
}

void QGstreamerMediaDevices::removeDevice(GstDevice *device)
{
//    qDebug() << "removing device:" << device << gst_device_get_display_name(device);
    if (m_videoSources.remove(device)) {
        videoInputsChanged();
    } else if (m_audioSources.remove(device)) {
        audioInputsChanged();
    } else if (m_audioSinks.remove(device)) {
        audioOutputsChanged();
    }

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

GstDevice *QGstreamerMediaDevices::audioDevice(const QByteArray &id, QAudioDevice::Mode mode) const
{
    const auto devices = (mode == QAudioDevice::Output) ? m_audioSinks : m_audioSources;

    return getDevice(devices, "device.bus_path", id);
}

GstDevice *QGstreamerMediaDevices::videoDevice(const QByteArray &id) const
{
    return getDevice(m_videoSources, "device.path", id);
}

QT_END_NAMESPACE
