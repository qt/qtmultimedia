// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_async_support_p.h"

#include "qpipewire_audiocontextmanager_p.h"

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

// SpaListenerBase

static std::atomic_int s_sequenceNumberAllocator;

SpaListenerBase::SpaListenerBase()
    : m_sequenceNumber(s_sequenceNumberAllocator.fetch_add(1, std::memory_order_relaxed))
{
}

void SpaListenerBase::removeHooks()
{
    spa_hook_remove(&m_listenerHook);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// NodeEventListener

NodeEventListener::NodeEventListener(PwNodeHandle node, NodeHandler handler)
    : m_node(std::move(node)), m_handler(std::move(handler))
{
    static constexpr struct pw_node_events nodeEvents{
        .version = PW_VERSION_NODE_EVENTS,
        .info = NodeEventListener::onInfo,
        .param = NodeEventListener::onParam,
    };

    int status = pw_node_add_listener(m_node.get(), &m_listenerHook, &nodeEvents, this);
    if (status < 0)
        qFatal() << "Failed to add listener: " << make_error_code(-status).message();
}

NodeEventListener::~NodeEventListener()
{
    removeHooks();
    QAudioContextManager::withEventLoopLock([&] {
        m_node = {};
    });
}

void NodeEventListener::enumParams(spa_param_type type)
{
    int status = pw_node_enum_params(m_node.get(), m_sequenceNumber, type, 0, 0, nullptr);
    if (status < 0)
        qCritical() << "pw_node_enum_params failed:" << make_error_code(-status).message();
}

void NodeEventListener::onInfo(void *data, const pw_node_info *info)
{
    NodeEventListener *self = reinterpret_cast<NodeEventListener *>(data);
    if (self->m_handler.infoHandler)
        self->m_handler.infoHandler(info);
}

void NodeEventListener::onParam(void *data, int seq, uint32_t id, uint32_t index, uint32_t next,
                                const spa_pod *param)
{
    NodeEventListener *self = reinterpret_cast<NodeEventListener *>(data);
    if (self->m_handler.paramHandler)
        self->m_handler.paramHandler(seq, id, index, next, param);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// CoreEventListener

CoreEventListener::CoreEventListener()
{
    coreEvents.version = PW_VERSION_CORE_EVENTS;
}

CoreEventListener::~CoreEventListener()
{
    removeHooks();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// CoreEventDoneListener

CoreEventDoneListener::CoreEventDoneListener()
{
    coreEvents.done = [](void *self, uint32_t id, int seq) {
        Q_ASSERT(QAudioContextManager::isInPwThreadLoop());
        CoreEventDoneListener *listener = reinterpret_cast<CoreEventDoneListener *>(self);
        if (id == PW_ID_CORE && listener->m_seqnum == seq) {
            listener->m_seqnum = -1;
            if (listener->m_handler)
                listener->m_handler();
        }
    };
}

q23::expected<void, int> CoreEventDoneListener::asyncWait(pw_core *coreConnection,
                                                          std::function<void()> handler)
{
    m_handler = std::move(handler);

    return QAudioContextManager::withEventLoopLock([&]() -> q23::expected<void, int> {
        int status = pw_core_add_listener(coreConnection, &m_listenerHook, &coreEvents, this);
        if (status < 0) {
            qFatal() << "pw_core_add_listener failed" << make_error_code(-status).message();
            return q23::unexpected(status);
        }

        Q_ASSERT(m_seqnum == -1);
        status = pw_core_sync(coreConnection, PW_ID_CORE, 0);
        if (status < 0)
            return q23::unexpected(status);
        m_seqnum = status;
        return {};
    });
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CoreEventSyncHelper::CoreEventSyncHelper() = default;

q23::expected<bool, int> CoreEventSyncHelper::sync(pw_core *coreConnection,
                                                   std::optional<std::chrono::nanoseconds> timeout)
{
    auto voidOrError = CoreEventDoneListener::asyncWait(coreConnection, [&] {
        m_semaphore.release();
    });
    if (voidOrError) {
        if (timeout)
            return m_semaphore.try_acquire_for(*timeout);

        m_semaphore.acquire();
        return true;
    }
    return voidOrError.error();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
