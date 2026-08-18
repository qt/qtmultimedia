// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_CAMERACONTEXTMANAGER_P_H
#define QPIPEWIRE_CAMERACONTEXTMANAGER_P_H

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

#include <QtMultimedia/private/qpipewire_cameradevicemonitor_p.h>
#include <QtMultimedia/private/qpipewire_instance_p.h>
#include <QtMultimedia/private/qpipewire_registry_support_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtCore/qglobal.h>

#include <pipewire/pipewire.h>

#include <memory>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class Q_MULTIMEDIA_EXPORT QCameraContextManager
{
public:
    QCameraContextManager();
    ~QCameraContextManager();

    static QCameraContextManager *instance();

    static bool isSupported();

    bool isConnected() const { return bool(m_coreConnection); }

    static QCameraDeviceMonitor &deviceMonitor();

    QPipeWireInstance &pwInstance() const { return *m_pwInstance; }
    const PwCoreConnectionHandle &coreConnection() const { return m_coreConnection; }

    static bool isInPwThreadLoop();

    PwNodeHandle bindNode(ObjectId);

    void syncRegistry();

private:
    static void objectAddedCb(void *data, uint32_t id, uint32_t permissions, const char *type,
                              uint32_t version, const struct spa_dict *props);
    static void objectRemovedCb(void *data, uint32_t id);
    void objectAdded(ObjectId, uint32_t permissions, PipewireRegistryType, uint32_t version,
                     const spa_dict &props);
    void objectRemoved(ObjectId);

    void startDeviceMonitor();
    void stopDeviceMonitor();

    const std::shared_ptr<QPipeWireInstance> m_pwInstance;

    PwCoreConnectionHandle m_coreConnection;
    PwRegistryHandle m_registry;
    spa_hook m_registryListener{};

    std::unique_ptr<QCameraDeviceMonitor> m_deviceMonitor;
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_CAMERACONTEXTMANAGER_P_H
