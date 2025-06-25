// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiodevicemonitor_p.h"

#include "qpipewire_audiocontextmanager_p.h"
#include "qpipewire_audiodevice_p.h"
#include "qpipewire_registry_support_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qflatmap_p.h>

#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <mutex>
#include <q20vector.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

using namespace QtMultimediaPrivate;

Q_STATIC_LOGGING_CATEGORY(lcPipewireDeviceMonitor, "qt.multimedia.pipewire.devicemonitor");

ObjectRemoveObserver::ObjectRemoveObserver(ObjectSerial objectSerial)
    : m_observedSerial(objectSerial)
{
}

ObjectSerial ObjectRemoveObserver::serial() const
{
    return m_observedSerial;
}

QAudioDeviceMonitor::QAudioDeviceMonitor()
{
    if (!QThread::isMainThread()) {
        // ensure that device monitor runs on application thread
        moveToThread(qApp->thread());
        m_compressionTimer.moveToThread(qApp->thread());
    }

    constexpr auto compressionTime = std::chrono::milliseconds(50);

    m_compressionTimer.setTimerType(Qt::TimerType::CoarseTimer);
    m_compressionTimer.setInterval(compressionTime);
    m_compressionTimer.setSingleShot(true);

    m_compressionTimer.callOnTimeout(this, [this] {
        audioDevicesChanged();
    });
}

void QAudioDeviceMonitor::objectAdded(ObjectId id, uint32_t /*permissions*/,
                                      PipewireRegistryType objectType, uint32_t /*version*/,
                                      const spa_dict &propDict)
{
    Q_ASSERT(QAudioContextManager::isInPwThreadLoop());

    Q_ASSERT(objectType == PipewireRegistryType::Device
             || objectType == PipewireRegistryType::Node);

    PwPropertyDict props = toPropertyDict(propDict);
    std::optional<std::string_view> mediaClass = getMediaClass(props);
    if (!mediaClass)
        return;

    std::optional<ObjectSerial> serial = getObjectSerial(props);
    Q_ASSERT(serial);
    {
        QWriteLocker lock{ &m_objectDictMutex };
        m_objectSerialDict.emplace(id, *serial);
        m_serialObjectDict.emplace(*serial, id);
    }

    switch (objectType) {
    case PipewireRegistryType::Device: {
        if (mediaClass != "Audio/Device")
            return;

        // we can store devices immediately
        qCDebug(lcPipewireDeviceMonitor)
                << "added device" << *serial << getDeviceDescription(props).value_or("");

        QWriteLocker lock{ &m_mutex };
        m_devices.emplace(*serial, DeviceRecord{ *serial, std::move(props) });

        return;
    }
    case PipewireRegistryType::Node: {
        // for nodes we need to enumerate the formats

        auto addPendingNode = [&](std::list<PendingNodeRecord> &pendingRecords) {
            std::optional<ObjectId> deviceId = getDeviceId(props);
            if (!deviceId) {
                // pipewire will create a dummy output in case theres' no physical output. We want
                // to filter that out
                qCDebug(lcPipewireDeviceMonitor) << "no device ID in node (ignoring):" << props;
                return;
            }

            std::optional<ObjectSerial> deviceSerial = findObjectSerial(*deviceId);
            if (!deviceSerial) {
                qCInfo(lcPipewireDeviceMonitor) << "Cannot add node: device removed";
                return;
            }

            std::lock_guard guard{ m_pendingRecordsMutex };

            qCDebug(lcPipewireDeviceMonitor)
                    << "added node for device " << *serial << *deviceSerial;

            // enumerating the audio format is asynchronous: we enumerate the formats asynchronously
            // and wait for the result before updating the device list
            pendingRecords.emplace_back(id, *serial, *deviceSerial, std::move(props));
            pendingRecords.back().formatFuture.then(
                    &m_compressionTimer, [this](std::optional<SpaObjectAudioFormat> const &) {
                startCompressionTimer();
            });
        };

        if (mediaClass == "Audio/Source")
            addPendingNode(m_pendingRecords.m_sources);
        else if (mediaClass == "Audio/Sink")
            addPendingNode(m_pendingRecords.m_sinks);
        break;
    }
    default:
        return;
    }
}

void QAudioDeviceMonitor::objectRemoved(ObjectId id)
{
    Q_ASSERT(QAudioContextManager::isInPwThreadLoop());

    std::optional<ObjectSerial> serial = findObjectSerial(id);

    if (!serial)
        return; // we didn't track the object.

    qCDebug(lcPipewireDeviceMonitor) << "removing object" << *serial;

    std::vector<SharedObjectRemoveObserver> removalObserversForObject;
    {
        QWriteLocker lock{ &m_objectDictMutex };

        for (const auto &observer : m_objectRemoveObserver) {
            if (observer->serial() == serial)
                removalObserversForObject.push_back(observer);
        }
        q20::erase_if(m_objectRemoveObserver, [&](const SharedObjectRemoveObserver &element) {
            return element->serial() == serial;
        });

        m_objectSerialDict.erase(id);
        m_serialObjectDict.erase(*serial);
    }

    for (const SharedObjectRemoveObserver &element : removalObserversForObject)
        emit element->objectRemoved();

    {
        std::lock_guard guard{ m_pendingRecordsMutex };

        m_pendingRecords.removeRecordsForObject(*serial);
        m_pendingRecords.m_removals.push_back(*serial);
    }

    startCompressionTimer();
}

void QAudioDeviceMonitor::setDefaultAudioSink(
        std::variant<QByteArray, NoDefaultDeviceType> newDefault)
{
    std::lock_guard guard{ m_pendingRecordsMutex };
    m_pendingRecords.m_defaultSink = std::move(newDefault);
    startCompressionTimer();
}

void QAudioDeviceMonitor::setDefaultAudioSource(
        std::variant<QByteArray, NoDefaultDeviceType> newDefault)
{
    std::lock_guard guard{ m_pendingRecordsMutex };
    m_pendingRecords.m_defaultSource = std::move(newDefault);
    startCompressionTimer();
}

void QAudioDeviceMonitor::audioDevicesChanged(bool verifyThreading)
{
    // Note: we don't want to assert here if we're called from the QtPipeWire::QAudioDevices()
    // constructor, as that might run on a worker thread (which pushed the instance to the app
    // thread)
    if (verifyThreading)
        Q_ASSERT(this->thread()->isCurrentThread());

    PendingRecords pendingRecords = [&] {
        std::lock_guard guard{ m_pendingRecordsMutex };
        PendingRecords resolvedRecords;

        std::swap(m_pendingRecords.m_removals, resolvedRecords.m_removals);
        std::swap(m_pendingRecords.m_defaultSource, resolvedRecords.m_defaultSource);
        std::swap(m_pendingRecords.m_defaultSink, resolvedRecords.m_defaultSink);

        // we may still have unresolved records, which wait on their format, but we only want to
        // handle the fully resolved elements
        auto takeFullyResolvedRecords = [](std::list<PendingNodeRecord> &toResolve,
                                           std::list<PendingNodeRecord> &resolved) {
            auto it = toResolve.begin();
            while (it != toResolve.end()) {
                if (it->formatFuture.isFinished()) {
                    auto next = std::next(it);
                    resolved.splice(resolved.end(), toResolve, it);
                    it = next;
                } else {
                    it++;
                }
            }
        };
        takeFullyResolvedRecords(m_pendingRecords.m_sources, resolvedRecords.m_sources);
        takeFullyResolvedRecords(m_pendingRecords.m_sinks, resolvedRecords.m_sinks);

        return resolvedRecords;
    }();

    auto getNodeName =
            [](std::variant<QByteArray, NoDefaultDeviceType> arg) -> std::optional<QByteArray> {
        if (std::holds_alternative<NoDefaultDeviceType>(arg))
            return std::nullopt;

        return std::get<QByteArray>(arg);
    };

    bool defaultSourceChanged = pendingRecords.m_defaultSource.has_value();
    if (defaultSourceChanged)
        m_defaultSourceName = getNodeName(*pendingRecords.m_defaultSource);

    bool defaultSinkChanged = pendingRecords.m_defaultSink.has_value();
    if (defaultSinkChanged)
        m_defaultSinkName = getNodeName(*pendingRecords.m_defaultSink);

    if (!pendingRecords.m_sources.empty() || !pendingRecords.m_removals.empty()
        || defaultSourceChanged)
        updateSources(std::move(pendingRecords.m_sources), pendingRecords.m_removals);

    if (!pendingRecords.m_sinks.empty() || !pendingRecords.m_removals.empty() || defaultSinkChanged)
        updateSinks(std::move(pendingRecords.m_sinks), pendingRecords.m_removals);
}

void QAudioDeviceMonitor::PendingRecords::removeRecordsForObject(ObjectSerial id)
{
    for (std::list<PendingNodeRecord> *recordList : { &m_sources, &m_sinks }) {
        recordList->remove_if([&](const PendingNodeRecord &record) {
            return record.serial == id || record.deviceSerial == id;
        });
    }
}

template <QAudioDeviceMonitor::Direction Mode>
std::optional<ObjectSerial>
QAudioDeviceMonitor::findNodeSerialForNodeName(std::string_view nodeName) const
{
    // find node by name
    QReadLocker guard(&m_mutex);

    QSpan records = Mode == Direction::sink ? QSpan{ m_sinks } : QSpan{ m_sources };
    auto it = std::find_if(records.begin(), records.end(), [&](const NodeRecord &sink) {
        return getNodeName(sink.properties) == nodeName;
    });

    if (it == records.end())
        return std::nullopt;
    return it->serial;
}

std::optional<ObjectSerial> QAudioDeviceMonitor::findSinkNodeSerial(std::string_view nodeName) const
{
    return findNodeSerialForNodeName<Direction::sink>(nodeName);
}

std::optional<ObjectSerial>
QAudioDeviceMonitor::findSourceNodeSerial(std::string_view nodeName) const
{
    return findNodeSerialForNodeName<Direction::source>(nodeName);
}

template <QAudioDeviceMonitor::Direction Mode>
void QAudioDeviceMonitor::updateSourcesOrSinks(std::list<PendingNodeRecord> addedNodes,
                                               QSpan<const ObjectSerial> removedObjects)
{
    QWriteLocker guard(&m_mutex);

    std::vector<NodeRecord> &sinksOrSources = Mode == Direction::sink ? m_sinks : m_sources;

    if (!removedObjects.empty()) {
        for (ObjectSerial id : removedObjects) {
            q20::erase_if(sinksOrSources, [&](const auto &record) {
                return record.serial == id || record.deviceSerial == id;
            });
        }
    }

    for (PendingNodeRecord &record : addedNodes) {
        std::optional<SpaObjectAudioFormat> result = record.formatFuture.result();
        if (result) {
            sinksOrSources.push_back(NodeRecord{
                    record.serial,
                    record.deviceSerial,
                    std::move(record.properties),
                    std::move(*result),
            });
        } else {
            qDebug(lcPipewireDeviceMonitor)
                    << "Could not resolve audio format for" << record.serial;
        }
    }

    QList<QAudioDevice> oldDeviceList =
            Mode == Direction::sink ? m_sinkDeviceList : m_sourceDeviceList;

    const std::optional<QByteArray> &defaultSinkOrSourceNodeNameBA =
            Mode == Direction::sink ? m_defaultSinkName : m_defaultSourceName;

    // revert once QTBUG-134902 is fixed
    const auto defaultSinkOrSourceNodeName = [&]() -> std::optional<std::string_view> {
        if (defaultSinkOrSourceNodeNameBA)
            return std::string_view{
                defaultSinkOrSourceNodeNameBA->data(),
                std::size_t(defaultSinkOrSourceNodeNameBA->size()),
            };
        return std::nullopt;
    }();

    QList<QAudioDevice> newDeviceList;

    // we brute-force re-create the device list ... not smart and it can certainly be improved
    for (NodeRecord &sinkOrSource : sinksOrSources) {
        ObjectSerial deviceSerial = sinkOrSource.deviceSerial;

        auto deviceIt = m_devices.find(deviceSerial);
        if (deviceIt == m_devices.end()) {
            qDebug(lcPipewireDeviceMonitor) << "No device for device id" << deviceSerial;
            continue;
        }

        std::optional<std::string_view> nodeName = getNodeName(sinkOrSource.properties);
        bool isDefault = (defaultSinkOrSourceNodeName == nodeName);

        auto devicePrivate = std::make_unique<QPipewireAudioDevicePrivate>(
                sinkOrSource.properties, deviceIt->second.properties, sinkOrSource.format,
                QAudioDevice::Mode::Output, isDefault);

        QAudioDevice device = QAudioDevicePrivate::createQAudioDevice(std::move(devicePrivate));

        newDeviceList.push_back(device);

        qDebug(lcPipewireDeviceMonitor) << "adding device" << deviceIt->second.properties;
    }

    // sort by description
    std::sort(newDeviceList.begin(), newDeviceList.end(),
              [](const QAudioDevice &lhs, const QAudioDevice &rhs) {
        return lhs.description() < rhs.description();
    });

    guard.unlock();

    bool deviceListsEqual = ranges::equal(oldDeviceList, newDeviceList,
                                          [](const QAudioDevice &lhs, const QAudioDevice &rhs) {
        return (lhs.id() == rhs.id()) && (lhs.isDefault() == rhs.isDefault());
    });

    if (!deviceListsEqual) {
        qDebug(lcPipewireDeviceMonitor) << "updated device list";

        if constexpr (Mode == Direction::sink) {
            m_sinkDeviceList = newDeviceList;
            emit audioSinksChanged(m_sinkDeviceList);
        } else {
            m_sourceDeviceList = newDeviceList;
            emit audioSourcesChanged(m_sourceDeviceList);
        }
    }
}

void QAudioDeviceMonitor::updateSinks(std::list<PendingNodeRecord> addedNodes,
                                      QSpan<const ObjectSerial> removedObjects)
{
    updateSourcesOrSinks<Direction::sink>(std::move(addedNodes), removedObjects);
}

void QAudioDeviceMonitor::updateSources(std::list<PendingNodeRecord> addedNodes,
                                        QSpan<const ObjectSerial> removedObjects)
{
    updateSourcesOrSinks<Direction::source>(std::move(addedNodes), removedObjects);
}

std::optional<ObjectSerial> QAudioDeviceMonitor::findDeviceSerial(std::string_view deviceName) const
{
    QReadLocker guard(&m_mutex);
    auto it = std::find_if(m_devices.begin(), m_devices.end(), [&](auto const &entry) {
        return getDeviceName(entry.second.properties) == deviceName;
    });
    if (it == m_devices.end())
        return std::nullopt;
    return it->first;
}

std::optional<ObjectId> QAudioDeviceMonitor::findObjectId(ObjectSerial serial)
{
    QReadLocker lock{ &m_objectDictMutex };

    auto it = m_serialObjectDict.find(serial);
    if (it != m_serialObjectDict.end())
        return it->second;
    return std::nullopt;
}

std::optional<ObjectSerial> QAudioDeviceMonitor::findObjectSerial(ObjectId id)
{
    QReadLocker lock{ &m_objectDictMutex };

    auto it = m_objectSerialDict.find(id);
    if (it != m_objectSerialDict.end())
        return it->second;
    return std::nullopt;
}

bool QAudioDeviceMonitor::registerObserver(SharedObjectRemoveObserver observer)
{
    QWriteLocker lock{ &m_objectDictMutex };

    if (m_serialObjectDict.find(observer->serial()) == m_serialObjectDict.end())
        return false; // don't register observer if the object has already been removed

    m_objectRemoveObserver.push_back(std::move(observer));
    return true;
}

void QAudioDeviceMonitor::unregisterObserver(const SharedObjectRemoveObserver &observer)
{
    QWriteLocker lock{ &m_objectDictMutex };

    q20::erase(m_objectRemoveObserver, observer);
}

QAudioDeviceMonitor::DeviceLists QAudioDeviceMonitor::getDeviceLists(bool verifyThreading)
{
    // force initial device enumeration
    QAudioContextManager::instance()->syncRegistry();

    // sync with format futures
    for (;;) {
        QAudioContextManager::instance()->syncRegistry();

        std::lock_guard pendingRecordLock{
            m_pendingRecordsMutex,
        };

        for (ObjectSerial removed : m_pendingRecords.m_removals)
            m_pendingRecords.removeRecordsForObject(removed);

        auto allFormatsResolved = [](const std::list<PendingNodeRecord> &list) {
            return std::all_of(list.begin(), list.end(), [](const PendingNodeRecord &record) {
                return record.formatFuture.isFinished();
            });
        };

        if (allFormatsResolved(m_pendingRecords.m_sources)
            && allFormatsResolved(m_pendingRecords.m_sinks))
            break;
    }

    // now all formats have been resolved and we can update the device list
    audioDevicesChanged(verifyThreading);

    QReadLocker lock{ &m_mutex };
    return {
        .sources = m_sourceDeviceList,
        .sinks = m_sinkDeviceList,
    };
}

void QAudioDeviceMonitor::startCompressionTimer()
{
    QMetaObject::invokeMethod(this, [this] {
        if (m_compressionTimer.isActive())
            return;
        m_compressionTimer.start();
    });
}

QAudioDeviceMonitor::PendingNodeRecord::PendingNodeRecord(ObjectId object, ObjectSerial serial,
                                                          ObjectSerial deviceSerial,
                                                          PwPropertyDict properties):
    serial{
        serial,
    },
    deviceSerial{
        deviceSerial,
    },
    properties{
        std::move(properties),
    }
{
    Q_ASSERT(QAudioContextManager::isInPwThreadLoop());

    auto promise = std::make_shared<QPromise<std::optional<SpaObjectAudioFormat>>>();
    formatFuture = promise->future();

    auto onParam = [promise = std::move(promise)](int /*seq*/, uint32_t /*id*/, uint32_t /*index*/,
                                                  uint32_t /*next*/,
                                                  const struct spa_pod *param) mutable {
        std::optional<SpaObjectAudioFormat> format = SpaObjectAudioFormat::parse(param);
        promise->start();
        promise->addResult(format);
        promise->finish();
    };

    QAudioContextManager *context = QAudioContextManager::instance();
    PwNodeHandle nodeProxy = context->bindNode(object);

    enumFormatListener = std::make_unique<NodeEventListener>(std::move(nodeProxy),
                                                             NodeEventListener::NodeHandler{
                                                                     {},
                                                                     std::move(onParam),
                                                             });

    enumFormatListener->enumParams(SPA_PARAM_EnumFormat);
}

} // namespace QtPipeWire

QT_END_NAMESPACE

#include "moc_qpipewire_audiodevicemonitor_p.cpp"
