// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiocontextmanager_p.h"

#include "qpipewire_instance_p.h"
#include "qpipewire_propertydict_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>

#include <system_error>

#if !PW_CHECK_VERSION(0, 3, 75)
extern "C" {
bool pw_check_library_version(int major, int minor, int micro);
}
#endif

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_GLOBAL_STATIC(QAudioContextManager, s_audioContextInstance);

Q_STATIC_LOGGING_CATEGORY(lcPipewireRegistry, "qt.multimedia.pipewire.registry");

QAudioContextManager::QAudioContextManager():
    m_libraryInstance{
        QPipeWireInstance::instance(),
    },
    m_deviceMonitor {
        std::make_unique<QAudioDeviceMonitor>(),
    }
{
    prepareEventLoop();
    prepareContext();
    connectToPipewireInstance();
    if (!isConnected())
        return;

    startDeviceMonitor();
    startEventLoop();
}

QAudioContextManager::~QAudioContextManager()
{
    if (isConnected())
        stopEventLoop();

    m_deviceMonitor.reset();
    m_registry.reset();
    m_coreConnection.reset();
    m_context.reset();
    m_eventLoop.reset();
}

bool QAudioContextManager::minimumRequirementMet()
{
    return pw_check_library_version(0, 3, 44); // we require PW_KEY_OBJECT_SERIAL
}

QAudioContextManager *QAudioContextManager::instance()
{
    return s_audioContextInstance;
}

bool QAudioContextManager::isConnected() const
{
    return bool(m_coreConnection);
}

QAudioDeviceMonitor &QAudioContextManager::deviceMonitor()
{
    return *instance()->m_deviceMonitor;
}

bool QAudioContextManager::isInPwThreadLoop()
{
    return pw_thread_loop_in_thread(instance()->m_eventLoop.get());
}

pw_loop *QAudioContextManager::getEventLoop()
{
    return pw_thread_loop_get_loop(instance()->m_eventLoop.get());
}

PwNodeHandle QAudioContextManager::bindNode(ObjectId id)
{
    return PwNodeHandle{
        reinterpret_cast<pw_node *>(pw_registry_bind(m_registry.get(), id.value,
                                                     PW_TYPE_INTERFACE_Node, PW_VERSION_NODE,
                                                     sizeof(void *))),
    };
}

void QAudioContextManager::prepareEventLoop()
{
    m_eventLoop = PwThreadLoopHandle{
        pw_thread_loop_new("QAudioContext", /*props=*/nullptr),
    };
    if (!m_eventLoop) {
        qFatal() << "Failed to create pipewire main loop" << make_error_code().message();
        return;
    }
}

void QAudioContextManager::startEventLoop()
{
    int status = pw_thread_loop_start(m_eventLoop.get());
    if (status < 0)
        qFatal() << "Failed to start event loop" << make_error_code(-status).message();
}

void QAudioContextManager::stopEventLoop()
{
    pw_thread_loop_stop(m_eventLoop.get());
}

void QAudioContextManager::prepareContext()
{
    PwPropertiesHandle props = makeProperties({
            { PW_KEY_APP_NAME, qApp->applicationName().toUtf8().data() },
    });

    Q_ASSERT(m_eventLoop);
    m_context = PwContextHandle{
        pw_context_new(pw_thread_loop_get_loop(m_eventLoop.get()), props.release(),
                       /*user_data_size=*/0),
    };
    if (!m_context)
        qFatal() << "Failed to create pipewire context" << make_error_code().message();
}

void QAudioContextManager::connectToPipewireInstance()
{
    Q_ASSERT(m_eventLoop && m_context);
    m_coreConnection = PwCoreConnectionHandle{
        pw_context_connect(m_context.get(), /*props=*/nullptr,
                           /*user_data_size=*/0),
    };

    if (!m_coreConnection)
        qInfo() << "Failed to connect to pipewire instance" << make_error_code().message();
}

void QAudioContextManager::objectAddedCb(void *data, uint32_t id, uint32_t permissions,
                                         const char *type, uint32_t version, const spa_dict *props)
{
    Q_ASSERT(isInPwThreadLoop());

    qCDebug(lcPipewireRegistry) << "objectAdded" << id << QString::number(permissions, 8) << type
                                << version << *props;

    reinterpret_cast<QAudioContextManager *>(data)->m_deviceMonitor->objectAdded(
            ObjectId{ id }, permissions, type, version, props);
}

void QAudioContextManager::objectRemovedCb(void *data, uint32_t id)
{
    Q_ASSERT(isInPwThreadLoop());

    qCDebug(lcPipewireRegistry) << "objectRemoved" << id;

    reinterpret_cast<QAudioContextManager *>(data)->m_deviceMonitor->objectRemoved(ObjectId{ id });
}

void QAudioContextManager::startDeviceMonitor()
{
    m_registry = PwRegistryHandle{
        pw_core_get_registry(m_coreConnection.get(), PW_VERSION_REGISTRY,
                             /*user_data_size=*/sizeof(QAudioContextManager *)),
    };
    if (!m_registry) {
        qFatal() << "Failed to create pipewire registry" << make_error_code().message();
        return;
    }

    spa_zero(m_registryListener);

    static constexpr struct pw_registry_events registry_events = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = QAudioContextManager::objectAddedCb,
        .global_remove = QAudioContextManager::objectRemovedCb,
    };
    int status =
            pw_registry_add_listener(m_registry.get(), &m_registryListener, &registry_events, this);
    if (status < 0)
        qFatal() << "Failed to add listener" << make_error_code(-status).message();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
