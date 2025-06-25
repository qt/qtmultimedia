// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIODEVICEMONITOR_P_H
#define QPIPEWIRE_AUDIODEVICEMONITOR_P_H

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

#include "qpipewire_async_support_p.h"
#include "qpipewire_propertydict_p.h"
#include "qpipewire_registry_support_p.h"
#include "qpipewire_spa_pod_support_p.h"

#include <QtCore/qfuture.h>
#include <QtCore/qtimer.h>
#include <QtCore/qreadwritelock.h>
#include <QtMultimedia/qaudiodevice.h>

#include <list>
#include <vector>

#include <pipewire/pipewire.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class ObjectRemoveObserver : public QObject
{
    Q_OBJECT
public:
    explicit ObjectRemoveObserver(ObjectSerial objectSerial);
    ObjectSerial serial() const;

signals:
    void objectRemoved();

private:
    const ObjectSerial m_observedSerial;
};

using SharedObjectRemoveObserver = std::shared_ptr<ObjectRemoveObserver>;

// TODO: can we make use of COW here?
class QAudioDeviceMonitor : public QObject
{
    Q_OBJECT

    enum class Direction : uint8_t
    {
        sink,
        source,
    };

public:
    QAudioDeviceMonitor();
    void objectAdded(ObjectId, uint32_t permissions, PipewireRegistryType, uint32_t version,
                     const spa_dict &props);
    void objectRemoved(ObjectId);

    struct NoDefaultDeviceType
    {
    };
    static constexpr NoDefaultDeviceType NoDefaultDevice{};
    void setDefaultAudioSink(std::variant<QByteArray, NoDefaultDeviceType>);
    void setDefaultAudioSource(std::variant<QByteArray, NoDefaultDeviceType>);

    std::optional<ObjectSerial> findSinkNodeSerial(std::string_view nodeName) const;
    std::optional<ObjectSerial> findSourceNodeSerial(std::string_view nodeName) const;

    // ObjectId/ObjectSerial mapping
    std::optional<ObjectId> findObjectId(ObjectSerial);
    std::optional<ObjectSerial> findObjectSerial(ObjectId);

    [[nodiscard]] bool registerObserver(SharedObjectRemoveObserver);
    void unregisterObserver(const SharedObjectRemoveObserver &);

    // Obtaining device lists
    struct DeviceLists
    {
        QList<QAudioDevice> sources;
        QList<QAudioDevice> sinks;
    };
    DeviceLists getDeviceLists(bool verifyThreading = true);

signals:
    void audioSinksChanged(QList<QAudioDevice>);
    void audioSourcesChanged(QList<QAudioDevice>);

private:
    struct DeviceRecord
    {
        ObjectSerial serial;
        PwPropertyDict properties;
    };

    struct PendingNodeRecord
    {
        PendingNodeRecord(ObjectId, ObjectSerial serial, ObjectSerial deviceSerial,
                          PwPropertyDict properties);

        ObjectSerial serial;
        ObjectSerial deviceSerial;
        PwPropertyDict properties;
        std::unique_ptr<NodeEventListener> enumFormatListener;
        QFuture<std::optional<SpaObjectAudioFormat>> formatFuture;
    };

    struct NodeRecord
    {
        ObjectSerial serial;
        ObjectSerial deviceSerial;
        PwPropertyDict properties;
        SpaObjectAudioFormat format;
    };

    // discovered, but format not resolved
    struct PendingRecords
    {
        std::list<PendingNodeRecord> m_sources;
        std::list<PendingNodeRecord> m_sinks;
        std::vector<ObjectSerial> m_removals;

        std::optional<std::variant<QByteArray, NoDefaultDeviceType>> m_defaultSource;
        std::optional<std::variant<QByteArray, NoDefaultDeviceType>> m_defaultSink;

        void removeRecordsForObject(ObjectSerial);
    };
    QMutex m_pendingRecordsMutex;
    PendingRecords m_pendingRecords QT_MM_GUARDED_BY(m_pendingRecordsMutex);

    // discovered, format resolved. living on application thread
    mutable QReadWriteLock m_mutex;
    std::map<ObjectSerial, DeviceRecord> m_devices QT_MM_GUARDED_BY(m_mutex);
    std::vector<NodeRecord> m_sources QT_MM_GUARDED_BY(m_mutex);
    std::vector<NodeRecord> m_sinks QT_MM_GUARDED_BY(m_mutex);
    std::optional<QByteArray> m_defaultSourceName;
    std::optional<QByteArray> m_defaultSinkName;

    QTimer m_compressionTimer;
    void startCompressionTimer();

    // Device list updates
    void audioDevicesChanged(bool verifyThreading = true);
    void updateSources(std::list<PendingNodeRecord> addedNodes,
                       QSpan<const ObjectSerial> removedObjects);
    void updateSinks(std::list<PendingNodeRecord> addedNodes,
                     QSpan<const ObjectSerial> removedObjects);
    template <Direction>
    void updateSourcesOrSinks(std::list<PendingNodeRecord>, QSpan<const ObjectSerial>);

    QList<QAudioDevice> m_sourceDeviceList;
    QList<QAudioDevice> m_sinkDeviceList;

    std::optional<ObjectSerial> findDeviceSerial(std::string_view deviceName) const;

    template <Direction>
    std::optional<ObjectSerial> findNodeSerialForNodeName(std::string_view nodeName) const;

    // ObjectId/ObjectSerial mapping
    // CHECK: can we completely rely on only accessing ObjectId under the pipewire event loop lock?
    mutable QReadWriteLock m_objectDictMutex;
    std::map<ObjectId, ObjectSerial> m_objectSerialDict QT_MM_GUARDED_BY(m_objectDictMutex);
    std::map<ObjectSerial, ObjectId> m_serialObjectDict QT_MM_GUARDED_BY(m_objectDictMutex);

    std::vector<SharedObjectRemoveObserver>
            m_objectRemoveObserver QT_MM_GUARDED_BY(m_objectDictMutex);
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_AUDIODEVICEMONITOR_P_H
