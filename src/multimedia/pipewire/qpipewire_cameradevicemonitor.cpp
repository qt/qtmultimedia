// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_cameradevicemonitor_p.h"

#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qpipewire_async_support_p.h>
#include <QtMultimedia/private/qpipewire_cameracontextmanager_p.h>
#include <QtMultimedia/private/qpipewire_cameranode_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>

#include <spa/param/param.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_STATIC_LOGGING_CATEGORY(lcPipewireCameraMonitor, "qt.multimedia.pipewire.camera.devicemonitor")

std::vector<QCameraDeviceMonitor::CameraNode> QCameraDeviceMonitor::cameraNodes() const
{
    using namespace QtMultimediaPrivate;

    std::vector<CameraNode> result = [&] {
        std::lock_guard guard{ m_mutex };
        return m_nodes | views::values | ranges::to<std::vector>();
    }();

    ranges::sort(result, [](const CameraNode &lhs, const CameraNode &rhs) {
        return cameraDeviceId(lhs.properties) < cameraDeviceId(rhs.properties);
    });

    return result;
}

QCameraDeviceMonitor::QCameraDeviceMonitor()
{
    if (!QThread::isMainThread())
        // ensure that device monitor runs on application thread
        moveToThread(qApp->thread());
}

std::optional<ObjectId> QCameraDeviceMonitor::findCameraNode(QByteArrayView deviceId) const
{
    std::lock_guard guard{ m_mutex };

    for (const auto &[id, node] : m_nodes) {
        if (cameraDeviceId(node.properties) == deviceId)
            return id;
    }

    return std::nullopt;
}

std::optional<QByteArray> QCameraDeviceMonitor::findTargetObject(QByteArrayView deviceId) const
{
    std::lock_guard guard{ m_mutex };

    for (const auto &[id, node] : m_nodes) {
        if (cameraDeviceId(node.properties) != deviceId)
            continue;

        // The serial is unambiguous and survives a node name being reused.
        if (std::optional<ObjectSerial> serial = getObjectSerial(node.properties))
            return QByteArray::number(qulonglong(serial->value));

        if (std::optional<std::string_view> nodeName = getNodeName(node.properties))
            return QByteArray(nodeName->data(), qsizetype(nodeName->size()));

        return std::nullopt;
    }

    return std::nullopt;
}

const PwPropertyDict *
QCameraDeviceMonitor::findDeviceProperties(const PwPropertyDict &nodeProperties) const
{
    const std::optional<ObjectId> deviceId = getDeviceId(nodeProperties);
    if (!deviceId)
        return nullptr;

    const auto device = m_devices.find(*deviceId);
    if (device == m_devices.end())
        return nullptr;

    return &device->second.properties;
}

namespace {

std::vector<SpaObjectVideoFormat> enumerateNodeFormats(QCameraContextManager &context,
                                                       ObjectId node)
{
    using namespace std::chrono_literals;
    QPipeWireInstance &pwInstance = context.pwInstance();

    auto formats = std::make_shared<std::vector<SpaObjectVideoFormat>>();

    auto onParam = [formats](int /*seq*/, uint32_t /*id*/, uint32_t /*index*/, uint32_t /*next*/,
                             const spa_pod *param) {
        if (std::optional<SpaObjectVideoFormat> format = SpaObjectVideoFormat::parse(param))
            formats->push_back(std::move(*format));
    };

    std::optional<NodeEventListener> listener;

    pwInstance.runWithEventLoopLock([&] {
        listener.emplace(context.bindNode(node),
                         NodeEventListener::NodeHandler{
                                 {},
                                 std::move(onParam),
                         });
        listener->enumParams(SPA_PARAM_EnumFormat);
    });

    CoreEventSyncHelper sync;
    // wait for the enumeration to complete, so that we can return the formats to the caller.
    auto syncOrErr = sync.sync(context.coreConnection().get(), 3s);
    if (syncOrErr != true)
        qCWarning(lcPipewireCameraMonitor) << "timed out enumerating the formats of node" << node;

    // The listener has to go before the formats are handed out, so that no
    // further param event can append to them.
    listener.reset();

    return std::move(*formats);
}

} // namespace

QList<QCameraDevice> QCameraDeviceMonitor::cameraDevices()
{
    QCameraContextManager &context = *QCameraContextManager::instance();
    Q_ASSERT(!context.isInPwThreadLoop());

    // Make sure everything the server had to say about the graph has arrived
    // before deciding which cameras there are.
    context.syncRegistry();

    QList<QCameraDevice> result;

    for (const CameraNode &node : cameraNodes()) {
        const QByteArray id = cameraDeviceId(node.properties);
        if (id.isEmpty())
            continue;

        const std::vector<SpaObjectVideoFormat> formats = enumerateNodeFormats(context, node.id);

        auto info = std::make_unique<QCameraDevicePrivate>();
        info->id = id;
        info->description = cameraDescription(node.properties, node.deviceProperties);
        info->position = cameraPosition(node.properties);
        info->videoFormats = toCameraFormats(formats);

        if (info->videoFormats.isEmpty()) {
            qCDebug(lcPipewireCameraMonitor) << "skipping camera without usable formats" << id;
            continue;
        }

        for (const QCameraFormat &format : info->videoFormats) {
            if (!info->photoResolutions.contains(format.resolution()))
                info->photoResolutions.push_back(format.resolution());
        }

        // PipeWire has no notion of a default video source, unlike
        // default.audio.source, so the first camera in a stable order it is.
        info->isDefault = result.isEmpty();

        result.push_back(QCameraDevicePrivate::create(std::move(info)));
    }

    return result;
}

void QCameraDeviceMonitor::objectAdded(ObjectId id, PipewireRegistryType type,
                                       const spa_dict &properties)
{
    using namespace QtMultimediaPrivate;
    bool changed = false;

    switch (type) {
    case PipewireRegistryType::Device: {
        PwPropertyDict dict = toPropertyDict(properties);

        std::lock_guard guard{ m_mutex };
        m_devices[id] = DeviceRecord{ std::move(dict) };

        // A device may show up after the nodes that belong to it, so the
        // descriptions of those nodes can change now.
        for (auto &[nodeId, node] : m_nodes) {
            if (getDeviceId(node.properties) == id) {
                node.deviceProperties = m_devices[id].properties;
                changed = true;
            }
        }
        break;
    }

    case PipewireRegistryType::Node: {
        PwPropertyDict dict = toPropertyDict(properties);
        if (!isCameraNode(dict))
            return;

        qCDebug(lcPipewireCameraMonitor) << "camera node added" << id << cameraDeviceId(dict);

        CameraNode node{
            id,
            std::move(dict),
            PwPropertyDict{},
        };

        std::lock_guard guard{ m_mutex };
        if (const PwPropertyDict *deviceProperties = findDeviceProperties(node.properties))
            node.deviceProperties = *deviceProperties;

        m_nodes.insert_or_assign(id, std::move(node));
        changed = true;
        break;
    }

    default:
        return;
    }

    if (changed)
        emit camerasChanged();
}

void QCameraDeviceMonitor::objectRemoved(ObjectId id)
{
    bool changed = false;

    {
        std::lock_guard guard{ m_mutex };

        if (m_nodes.erase(id) != 0) {
            qCDebug(lcPipewireCameraMonitor) << "camera node removed" << id;
            changed = true;
        }

        if (m_devices.erase(id) != 0) {
            // Drop the borrowed description of any node that outlives its
            // device, rather than keeping stale properties around.
            for (auto &[nodeId, node] : m_nodes) {
                if (getDeviceId(node.properties) == id) {
                    node.deviceProperties.clear();
                    changed = true;
                }
            }
        }
    }

    if (changed)
        emit camerasChanged();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
