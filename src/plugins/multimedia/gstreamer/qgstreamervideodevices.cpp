// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreamervideodevices_p.h"

#include <QtMultimedia/qmediadevices.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qset.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <common/qgst_p.h>
#include <common/qgst_debug_p.h>
#include <common/qgstutils_p.h>
#include <common/qglist_helper_p.h>

#if QT_CONFIG(linux_v4l)
#  include <linux/videodev2.h>
#  include <errno.h>
#endif

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(ltVideoDevices, "qt.multimedia.gstreamer.videodevices");

QGstreamerVideoDevices::QGstreamerVideoDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration),
      m_deviceMonitor{
          gst_device_monitor_new(),
          QGstDeviceMonitorHandle::HasRef,
      },
      m_busObserver{
          QGstBusHandle{
                  gst_device_monitor_get_bus(m_deviceMonitor.get()),
                  QGstBusHandle::HasRef,
          },
      }
{
    gst_device_monitor_add_filter(m_deviceMonitor.get(), "Video/Source", nullptr);

    m_busObserver.installMessageFilter(this);
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

QList<QCameraDevice> QGstreamerVideoDevices::findVideoInputs() const
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
        if (caps) {
            QList<QCameraFormat> formats;
            QSet<QSize> photoResolutions;

            int size = caps.size();
            for (int i = 0; i < size; ++i) {
                auto cap = caps.at(i);
                auto pixelFormat = cap.pixelFormat();
                auto frameRate = cap.frameRateRange();

                if (pixelFormat == QVideoFrameFormat::PixelFormat::Format_Invalid) {
                    qCDebug(ltVideoDevices) << "pixel format not supported:" << cap;
                    continue; // skip pixel formats that we don't support
                }

                auto addFormatForResolution = [&](QSize resolution) {
                    auto *f = new QCameraFormatPrivate{
                        QSharedData(), pixelFormat, resolution, frameRate.min, frameRate.max,
                    };
                    formats.append(f->create());
                    photoResolutions.insert(resolution);
                };

                std::optional<QGRange<QSize>> resolutionRange = cap.resolutionRange();
                if (resolutionRange) {
                    addFormatForResolution(resolutionRange->min);
                    addFormatForResolution(resolutionRange->max);
                } else {
                    QSize resolution = cap.resolution();
                    if (resolution.isValid())
                        addFormatForResolution(resolution);
                }
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
        QUniqueFileDescriptorHandle fd{
            qt_safe_open(p, O_RDONLY),
        };

        if (!fd) {
            qCDebug(ltVideoDevices) << "Cannot open v4l2 device:" << p;
            return;
        }

        struct v4l2_capability cap;
        if (::ioctl(fd.get(), VIDIOC_QUERYCAP, &cap) < 0) {
            qCWarning(ltVideoDevices)
                    << "ioctl failed: VIDIOC_QUERYCAP" << qt_error_string(errno) << p;
            return;
        }

        if (cap.device_caps & V4L2_CAP_META_CAPTURE) {
            qCDebug(ltVideoDevices) << "V4L2_CAP_META_CAPTURE device detected" << p;
            return;
        }

        constexpr uint32_t videoCaptureCapabilities =
                V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_VIDEO_CAPTURE_MPLANE;

        if (!(cap.capabilities & videoCaptureCapabilities)) {
            qCDebug(ltVideoDevices)
                    << "not a V4L2_CAP_VIDEO_CAPTURE or V4L2_CAP_VIDEO_CAPTURE_MPLANE device" << p;
            return;
        }
        if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
            qCDebug(ltVideoDevices) << "not a V4L2_CAP_STREAMING device" << p;
            return;
        }

        int index;
        if (::ioctl(fd.get(), VIDIOC_G_INPUT, &index) < 0) {
            switch (errno) {
            case ENOTTY:
                qCDebug(ltVideoDevices) << "device does not have video inputs" << p;
                return;

            default:
                qCWarning(ltVideoDevices)
                        << "ioctl failed: VIDIOC_G_INPUT" << qt_error_string(errno) << p;
                return;
            }
        }
    } else {
        qCDebug(ltVideoDevices) << "Video device not a v4l2 device:" << structureHandle;
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

    m_idGenerator++;

    onVideoInputsChanged();
}

void QGstreamerVideoDevices::removeDevice(QGstDeviceHandle device)
{
    auto it = std::find_if(m_videoSources.begin(), m_videoSources.end(),
                           [&](const QGstRecordDevice &a) { return a.gstDevice == device; });

    if (it != m_videoSources.end()) {
        m_videoSources.erase(it);
        onVideoInputsChanged();
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
