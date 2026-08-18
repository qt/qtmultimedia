// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_CAMERADEVICEMONITOR_P_H
#define QPIPEWIRE_CAMERADEVICEMONITOR_P_H

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

#include <QtMultimedia/private/qpipewire_propertydict_p.h>
#include <QtMultimedia/private/qpipewire_spa_video_format_support_p.h>
#include <QtMultimedia/private/qpipewire_registry_support_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtCore/qmutex.h>
#include <QtCore/qobject.h>

#include <vector>
#include <map>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class Q_MULTIMEDIA_EXPORT QCameraDeviceMonitor : public QObject
{
    Q_OBJECT

public:
    QCameraDeviceMonitor();

    struct CameraNode
    {
        ObjectId id;
        PwPropertyDict properties;
        PwPropertyDict deviceProperties; // of the parent device, empty if it has none
    };

    std::vector<CameraNode> cameraNodes() const;
    QList<QCameraDevice> cameraDevices();
    std::optional<ObjectId> findCameraNode(QByteArrayView deviceId) const;

    // FIXME: improve type safety!
    std::optional<QByteArray> findTargetObject(QByteArrayView deviceId) const;

    // Called on the PipeWire event loop thread.
    void objectAdded(ObjectId, PipewireRegistryType, const spa_dict &properties);
    void objectRemoved(ObjectId);

Q_SIGNALS:
    void camerasChanged();

private:
    struct DeviceRecord
    {
        PwPropertyDict properties;
    };

    mutable QMutex m_mutex;
    std::map<ObjectId, CameraNode> m_nodes;
    std::map<ObjectId, DeviceRecord> m_devices;

    // Requires m_mutex.
    const PwPropertyDict *findDeviceProperties(const PwPropertyDict &nodeProperties) const;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERADEVICEMONITOR_P_H
