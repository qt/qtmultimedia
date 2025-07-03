// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_audiocontextmanager_p.h"

#include "qpipewire_audiostream_p.h"
#include "qpipewire_instance_p.h"
#include "qpipewire_propertydict_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qsemaphore.h>

#include <pipewire/extensions/metadata.h>

#include <system_error>

#if __has_include(<spa/utils/json.h>)
#  include <spa/utils/json.h>
#else
#  include <QtCore/qjsondocument.h>
#  include <QtCore/qjsonvalue.h>
#endif

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
    if (!m_context) {
        // pipewire server not available
        return;
    }

    connectToPipewireInstance();
    if (!isConnected())
        return;

    startDeviceMonitor();
    startEventLoop();
}

QAudioContextManager::~QAudioContextManager()
{
    if (isConnected()) {
        stopEventLoop();
        stopActiveStreams();
    }

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

void QAudioContextManager::syncRegistry()
{
    QSemaphore sync;

    int seqnum{};

    auto handler = [&](uint32_t id, int seq) {
        Q_ASSERT(isInPwThreadLoop());
        if (id == PW_ID_CORE && seq == seqnum)
            sync.release();
    };

    pw_core_events coreEvents = {};
    spa_hook coreListener = {};
    coreEvents.version = PW_VERSION_CORE_EVENTS;

    coreEvents.done = [](void *object, uint32_t id, int seq) {
        Q_ASSERT(isInPwThreadLoop());
        (*reinterpret_cast<std::add_pointer_t<decltype(handler)>>(object))(id, seq);
    };

    bool syncStarted = withEventLoopLock([&] {
        int status =
                pw_core_add_listener(m_coreConnection.get(), &coreListener, &coreEvents, &handler);
        if (status < 0) {
            qFatal() << "pw_core_add_listener failed" << make_error_code(-status).message();
            return false;
        }

        status = pw_core_sync(m_coreConnection.get(), PW_ID_CORE, 0);
        if (status < 0) {
            qFatal() << "pw_core_sync failed" << make_error_code(-status).message();
            return false;
        }

        seqnum = status;
        return true;
    });

    if (syncStarted)
        sync.acquire();

    spa_hook_remove(&coreListener);
}

void QAudioContextManager::registerStreamReference(std::shared_ptr<QPipewireAudioStream> stream)
{
    std::lock_guard guard{ m_activeStreamMutex };
    m_activeStreams.emplace(std::move(stream));
}

void QAudioContextManager::unregisterStreamReference(
        const std::shared_ptr<QPipewireAudioStream> &stream)
{
    std::lock_guard guard{ m_activeStreamMutex };
    m_activeStreams.erase(stream);
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

    std::optional<PipewireRegistryType> objectType = parsePipewireRegistryType(type);
    if (!objectType) {
        qCritical() << "object type cannot be parsed:" << type;
        return;
    }

    if (!props) {
        qCritical() << "null property received";
        return;
    }

    reinterpret_cast<QAudioContextManager *>(data)->objectAdded(ObjectId{ id }, permissions,
                                                                *objectType, version, *props);
}

void QAudioContextManager::objectRemovedCb(void *data, uint32_t id)
{
    Q_ASSERT(isInPwThreadLoop());

    qCDebug(lcPipewireRegistry) << "objectRemoved" << id;

    reinterpret_cast<QAudioContextManager *>(data)->m_deviceMonitor->objectRemoved(ObjectId{ id });
}

void QAudioContextManager::objectAdded(ObjectId id, uint32_t permissions, PipewireRegistryType type,
                                       uint32_t version, const spa_dict &props)
{
    switch (type) {
    case PipewireRegistryType::Device:
    case PipewireRegistryType::Node:
        return m_deviceMonitor->objectAdded(id, permissions, type, version, props);

    case PipewireRegistryType::Metadata: {
        const char *name = spa_dict_lookup(&props, PW_KEY_METADATA_NAME);
        if (name == std::string_view("default"))
            // the "default" metadata will inform us about the "default" device
            return startListenDefaultMetadata(id, version);
        return;
    }

    default:
        return;
    }
}

void QAudioContextManager::startListenDefaultMetadata(ObjectId id, uint32_t version)
{
    if (m_defaultMetadata) {
        qWarning(lcPipewireRegistry) << "metadata already registered";
        return;
    }

    static constexpr pw_metadata_events metadata_events = {
        .version = PW_VERSION_METADATA_EVENTS,
        .property = [](void *data, uint32_t subject, const char *key, const char *type,
                       const char *value) -> int {
        Q_ASSERT(subject == PW_ID_CORE);

        auto self = reinterpret_cast<QAudioContextManager *>(data);

        Q_ASSERT(key);
        return self->handleMetadata(MetadataRecord{
                .key = key,
                .type = type,
                .value = value,
        });
    },
    };

    m_defaultMetadata.reset(reinterpret_cast<pw_metadata *>(pw_registry_bind(
            m_registry.get(), id.value, PW_TYPE_INTERFACE_Metadata, version, sizeof(this))));
    if (!m_defaultMetadata) {
        qFatal() << "cannot bind to metadata";
        return;
    }

    int status = pw_metadata_add_listener(m_defaultMetadata.get(), &m_defaultMetadataListener,
                                          &metadata_events, this);
    if (status < 0)
        qFatal() << "Failed to add listener" << make_error_code(-status).message();
}

namespace {

// parse json object with one "name" member
std::optional<QByteArray> jsonParseObjectName(const char *json_str)
{
#if __has_include(<spa/utils/json.h>)
    using namespace std::string_view_literals;

    struct spa_json json;
    spa_json_init(&json, json_str, strlen(json_str));

    struct spa_json it;
    if (spa_json_enter_object(&json, &it) > 0) {
        char key[256];
        while (spa_json_get_string(&it, key, sizeof(key)) > 0) {
            if (key == "name"sv) {
                char value[16384];
                if (spa_json_get_string(&it, value, sizeof(value)) >= 0)
                    return QByteArray{ value };
            } else {
                spa_json_next(&it, nullptr);
            }
        }
    }

    return std::nullopt;
#else
    // old pipewire does not provide json.h, so we use Qt to parse

    using namespace Qt::Literals;

    QByteArray value{ json_str };
    QJsonDocument doc = QJsonDocument::fromJson(value);
    if (doc.isNull()) {
        qWarning() << "JSON parse error:" << json_str;
        return std::nullopt;
    }

    QJsonValue name = doc[u"name"_s];
    if (!name.isString())
        return std::nullopt;
    return name.toString().toUtf8();
#endif
}

} // namespace

int QAudioContextManager::handleMetadata(const MetadataRecord &record)
{
    using namespace std::string_view_literals;

    qDebug(lcPipewireRegistry) << "metadata:" << record.key << record.type << record.value;

    if (record.key == nullptr) {
        // "NULL clears all metadata for the subject"
        m_deviceMonitor->setDefaultAudioSource(QAudioDeviceMonitor::NoDefaultDevice);
        m_deviceMonitor->setDefaultAudioSink(QAudioDeviceMonitor::NoDefaultDevice);
        return 0;
    }

    auto extractName = [&]() -> std::optional<QByteArray> {
        if (record.type != "Spa:String:JSON"sv)
            return std::nullopt;
        return jsonParseObjectName(record.value);
    };

    if (record.key == "default.audio.source"sv) {
        if (record.value) {
            std::optional<QByteArray> name = extractName();
            if (name)
                m_deviceMonitor->setDefaultAudioSource(std::move(*name));
        } else {
            m_deviceMonitor->setDefaultAudioSource(QAudioDeviceMonitor::NoDefaultDevice);
        }

        return 0;
    }

    if (record.key == "default.audio.sink"sv) {
        if (record.value) {
            std::optional<QByteArray> name = extractName();
            if (name)
                m_deviceMonitor->setDefaultAudioSink(std::move(*name));
        } else {
            m_deviceMonitor->setDefaultAudioSink(QAudioDeviceMonitor::NoDefaultDevice);
        }
        return 0;
    }

    return 0;
}

void QAudioContextManager::stopActiveStreams()
{
    auto streams = std::exchange(m_activeStreams, {});

    for (const auto &stream : streams)
        stream->resetStream();
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
