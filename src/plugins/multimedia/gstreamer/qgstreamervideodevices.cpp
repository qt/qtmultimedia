// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideodevices_p.h"
#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qcameradevice_p.h>

#include <common/qgst_p.h>
#include <common/qgstutils_p.h>
#include <common/qglist_helper_p.h>

#if QT_CONFIG(linux_v4l)
#  include <linux/videodev2.h>
#  include <errno.h>
#endif

QT_BEGIN_NAMESPACE

QGstreamerVideoDevices::QGstreamerVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration),
      m_deviceMonitor{
          gst_device_monitor_new(),
      },
      m_bus{
          QGstBusHandle{
                  gst_device_monitor_get_bus(m_deviceMonitor.get()),
                  QGstBusHandle::HasRef,
          },
      }
{
    gst_device_monitor_add_filter(m_deviceMonitor.get(), "Video/Source", nullptr);

    m_bus.installMessageFilter(this);
    gst_device_monitor_start(m_deviceMonitor.get());

    GList *devices = gst_device_monitor_get_devices(m_deviceMonitor.get());

    for (GstDevice *device : QGstUtils::GListRangeAdaptor<GstDevice *>(devices)) {
        addDevice(QGstDeviceHandle{
                device,
                QGstDeviceHandle::HasRef,
        });
    }

    g_list_free(devices);
}

QGstreamerVideoDevices::~QGstreamerVideoDevices()
{
    gst_device_monitor_stop(m_deviceMonitor.get());
}

QList<QCameraDevice> QGstreamerVideoDevices::videoDevices() const
{
    QList<QCameraDevice> devices;

    for (const auto &device : m_videoSources) {
        QCameraDevicePrivate *info = new QCameraDevicePrivate;

        QGString desc{
            gst_device_get_display_name(device.gstDevice.get()),
        };
        info->description = desc.toQString();
        info->id = device.id;

        QUniqueGstStructureHandle properties{
            gst_device_get_properties(device.gstDevice.get()),
        };
        if (properties) {
            QGstStructureView view{ properties };
            auto def = view["is-default"].toBool();
            info->isDefault = def && *def;
        }

        if (info->isDefault)
            devices.prepend(info->create());
        else
            devices.append(info->create());

        auto caps = QGstCaps(gst_device_get_caps(device.gstDevice.get()), QGstCaps::HasRef);
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

                auto *f = new QCameraFormatPrivate{ QSharedData(), pixelFormat, resolution,
                                                    frameRate.min, frameRate.max };
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

void QGstreamerVideoDevices::addDevice(QGstDeviceHandle device)
{
    Q_ASSERT(gst_device_has_classes(device.get(), "Video/Source"));

#if QT_CONFIG(linux_v4l)
    QUniqueGstStructureHandle structureHandle{
        gst_device_get_properties(device.get()),
    };

    const auto *p = QGstStructureView(structureHandle.get())["device.path"].toString();
    if (p) {
        QFileDescriptorHandle fd{
            qt_safe_open(p, O_RDONLY),
        };
        int index;
        if (::ioctl(fd.get(), VIDIOC_G_INPUT, &index) < 0) {
            switch (errno) {
            case ENOTTY: // no video inputs
            case EINVAL: // ioctl is not supported. E.g. the Broadcom Image Signal Processor
                         // available on Raspberry Pi
                return;

            default:
                qWarning() << "ioctl failed: VIDIOC_G_INPUT" << qt_error_string(errno) << p;
                return;
            }
        }
    }
#endif

    auto it = std::find_if(m_videoSources.begin(), m_videoSources.end(),
                           [&](const QGstRecordDevice &a) { return a.gstDevice == device; });

    if (it != m_videoSources.end())
        return;

    m_videoSources.push_back(QGstRecordDevice{
            std::move(device),
            QByteArray::number(m_idGenerator),
    });
    emit videoInputsChanged();
    m_idGenerator++;
}

void QGstreamerVideoDevices::removeDevice(QGstDeviceHandle device)
{
    auto it = std::find_if(m_videoSources.begin(), m_videoSources.end(),
                           [&](const QGstRecordDevice &a) { return a.gstDevice == device; });

    if (it != m_videoSources.end()) {
        m_videoSources.erase(it);
        emit videoInputsChanged();
    }
}

bool QGstreamerVideoDevices::processBusMessage(const QGstreamerMessage &message)
{
    QGstDeviceHandle device;

    switch (message.type()) {
    case GST_MESSAGE_DEVICE_ADDED:
        gst_message_parse_device_added(message.message(), &device);
        addDevice(std::move(device));
        break;
    case GST_MESSAGE_DEVICE_REMOVED:
        gst_message_parse_device_removed(message.message(), &device);
        removeDevice(std::move(device));
        break;
    default:
        break;
    }

    return false;
}

GstDevice *QGstreamerVideoDevices::videoDevice(const QByteArray &id) const
{
    auto it = std::find_if(m_videoSources.begin(), m_videoSources.end(),
                           [&](const QGstRecordDevice &a) { return a.id == id; });
    return it != m_videoSources.end() ? it->gstDevice.get() : nullptr;
}

QT_END_NAMESPACE
