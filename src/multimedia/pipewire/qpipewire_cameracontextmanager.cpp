// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_cameracontextmanager_p.h"

#include <QtMultimedia/private/qpipewire_async_support_p.h>
#include <QtMultimedia/private/qpipewire_instance_p.h>
#include <QtCore/qapplicationstatic.h>
#include <QtCore/qloggingcategory.h>

#if QT_CONFIG(dbus)
#  include <QtMultimedia/private/qpipewire_cameraportalclient_p.h>
#  include <QtMultimedia/private/qpipewire_portal_p.h>
#  include <QtDBus/qdbusunixfiledescriptor.h>
#endif

#include <pipewire/pipewire.h>

#if !PW_CHECK_VERSION(0, 3, 75)
extern "C" {
bool pw_check_library_version(int major, int minor, int micro);
}
#endif

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_APPLICATION_STATIC(QCameraContextManager, s_cameraContextInstance)
Q_STATIC_LOGGING_CATEGORY(lcPipewireCameraRegistry, "qt.multimedia.pipewire.camera.registry")

QCameraContextManager::QCameraContextManager()
    : m_pwInstance{
          QPipeWireInstance::instance(),
      },
      m_deviceMonitor{
          std::make_unique<QCameraDeviceMonitor>(),
      }
{
    static const bool forcePortal =
            qEnvironmentVariableIntValue("QT_PIPEWIRE_CAMERA_FORCE_PORTAL") != 0;

    if (!forcePortal) {
        bool daemonConnected = connectToDaemon();
        if (daemonConnected)
            return;
    }

#if QT_CONFIG(dbus)
    connectViaPortal();
#else
    qCWarning(lcPipewireCameraRegistry)
            << "Cannot fall back to the camera portal: built without D-Bus support";
#endif
}

bool QCameraContextManager::connectToDaemon()
{
    return m_pwInstance->runWithEventLoopLock([&] {
        auto connection = m_pwInstance->connectToDaemon();
        if (!connection) {
            qCDebug(lcPipewireCameraRegistry) << "Failed to connect to pipewire instance directly:"
                                              << connection.error().message();
            return false;
        }
        m_coreConnection = std::move(*connection);
        startDeviceMonitor();
        return true;
    });
}

#if QT_CONFIG(dbus)
bool QCameraContextManager::connectViaPortal()
{
    using namespace std::chrono_literals;

    if (!portalInterfaceVersion(QLatin1StringView("org.freedesktop.portal.Camera"))) {
        qCWarning(lcPipewireCameraRegistry) << "No camera portal available either";
        return false;
    }

    // brings up a dialog for the user to approve camera access
    auto fd = requestCameraPortalFd(20s);
    if (!fd) {
        qCWarning(lcPipewireCameraRegistry)
                << "Failed to obtain a camera file descriptor from the portal:" << fd.error();
        return false;
    }

    return m_pwInstance->runWithEventLoopLock([&] {
        auto connection = m_pwInstance->connectToPortal(fd->takeFileDescriptor());
        if (!connection) {
            qCWarning(lcPipewireCameraRegistry)
                    << "Failed to connect via the camera portal" << connection.error().message();
            return false;
        }
        m_coreConnection = std::move(*connection);
        startDeviceMonitor();
        return true;
    });
}
#endif

QCameraContextManager::~QCameraContextManager()
{
    if (isConnected())
        stopDeviceMonitor();

    m_pwInstance->runWithEventLoopLock([&] {
        m_deviceMonitor.reset();
        m_registry.reset();
        m_coreConnection.reset();
    });
}

QCameraContextManager *QCameraContextManager::instance()
{
    return s_cameraContextInstance;
}

bool QCameraContextManager::isSupported()
{
    if (!QPipeWireInstance::isLoaded())
        return false;

    if (!pw_check_library_version(0, 3, 44))
        return false;

    QCameraContextManager *self = instance();
    return self && self->isConnected();
}

QCameraDeviceMonitor &QCameraContextManager::deviceMonitor()
{
    return *instance()->m_deviceMonitor;
}

bool QCameraContextManager::isInPwThreadLoop()
{
    return instance()->m_pwInstance->isInPwThreadLoop();
}

PwNodeHandle QCameraContextManager::bindNode(ObjectId id)
{
    return PwNodeHandle{
        reinterpret_cast<pw_node *>(pw_registry_bind(m_registry.get(), id.value,
                                                     PW_TYPE_INTERFACE_Node, PW_VERSION_NODE,
                                                     sizeof(void *))),
    };
}

void QCameraContextManager::syncRegistry()
{
    using namespace std::chrono_literals;
    CoreEventSyncHelper syncHelper;

    auto syncOrErr = syncHelper.sync(m_coreConnection.get(), 3s);
    if (syncOrErr == true)
        return;
    if (syncOrErr == false)
        qWarning() << "pw_core_sync timed out";
    else if (syncOrErr.error()) {
        int err = syncOrErr.error();
        qWarning() << "CoreEventSyncHelper::sync failed:" << make_error_code(err).message();
    }
}

void QCameraContextManager::objectAddedCb(void *data, uint32_t id, uint32_t permissions,
                                          const char *type, uint32_t version, const spa_dict *props)
{
    Q_ASSERT(isInPwThreadLoop());

    std::optional<PipewireRegistryType> objectType = parsePipewireRegistryType(type);
    if (!objectType) {
        qCritical() << "object type cannot be parsed:" << type;
        return;
    }

    if (!props) {
        qCritical() << "null property received";
        return;
    }

    reinterpret_cast<QCameraContextManager *>(data)->objectAdded(ObjectId{ id }, permissions,
                                                                 *objectType, version, *props);
}

void QCameraContextManager::objectRemovedCb(void *data, uint32_t id)
{
    Q_ASSERT(isInPwThreadLoop());

    auto *self = reinterpret_cast<QCameraContextManager *>(data);
    self->objectRemoved(ObjectId{ id });
}

void QCameraContextManager::objectAdded(ObjectId id, uint32_t /*permissions*/,
                                        PipewireRegistryType type, uint32_t /*version*/,
                                        const spa_dict &props)
{
    m_deviceMonitor->objectAdded(id, type, props);
}

void QCameraContextManager::objectRemoved(ObjectId id)
{
    m_deviceMonitor->objectRemoved(id);
}

void QCameraContextManager::startDeviceMonitor()
{
    m_registry = PwRegistryHandle{
        pw_core_get_registry(m_coreConnection.get(), PW_VERSION_REGISTRY,
                             /*user_data_size=*/sizeof(QCameraContextManager *)),
    };
    if (!m_registry) {
        qFatal() << "Failed to create pipewire registry" << make_error_code().message();
        return;
    }

    spa_zero(m_registryListener);

    static constexpr struct pw_registry_events registry_events = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = QCameraContextManager::objectAddedCb,
        .global_remove = QCameraContextManager::objectRemovedCb,
    };
    int status =
            pw_registry_add_listener(m_registry.get(), &m_registryListener, &registry_events, this);
    if (status < 0)
        qFatal() << "Failed to add listener" << make_error_code(-status).message();
}

void QCameraContextManager::stopDeviceMonitor()
{
    if (!m_registry)
        return;

    m_pwInstance->runWithEventLoopLock([&] {
        spa_hook_remove(&m_registryListener);
        m_registry.reset();
    });
}

} // namespace QtPipeWire

QT_END_NAMESPACE
