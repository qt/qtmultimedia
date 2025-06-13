// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_AUDIOCONTEXTMANAGER_P_H
#define QPIPEWIRE_AUDIOCONTEXTMANAGER_P_H

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

#include <QtCore/qglobal.h>

#include "qpipewire_audiodevicemonitor_p.h"
#include "qpipewire_support_p.h"
#include "qpipewire_registry_support_p.h"

#include <pipewire/pipewire.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

class QPipeWireInstance;
struct QPipewireAudioStream;

class QAudioContextManager
{
public:
    QAudioContextManager();
    ~QAudioContextManager();

    static bool minimumRequirementMet();

    static QAudioContextManager *instance();
    bool isConnected() const;

    template <typename Closure>
    static auto withEventLoopLock(Closure &&c)
    {
        QAudioContextManager *self = instance();

        pw_thread_loop_lock(self->m_eventLoop.get());
        auto unlock = qScopeGuard([&] {
            pw_thread_loop_unlock(self->m_eventLoop.get());
        });

        return c();
    }

    static QAudioDeviceMonitor &deviceMonitor();

    static bool isInPwThreadLoop();
    static pw_loop *getEventLoop();

    PwNodeHandle bindNode(ObjectId id);

    void syncRegistry();

    void registerStreamReference(std::shared_ptr<QPipewireAudioStream>);
    void unregisterStreamReference(const std::shared_ptr<QPipewireAudioStream> &);

private:
    std::shared_ptr<QPipeWireInstance> m_libraryInstance;

    // event loop
    PwThreadLoopHandle m_eventLoop;
    void prepareEventLoop();
    void startEventLoop();
    void stopEventLoop();

    // pipewire context
    PwContextHandle m_context;
    void prepareContext();

    // pw_core connection
    PwCoreConnectionHandle m_coreConnection;
    void connectToPipewireInstance();

    // device monitor
    PwRegistryHandle m_registry;
    struct spa_hook m_registryListener{};
    std::unique_ptr<QAudioDeviceMonitor> m_deviceMonitor;

    void startDeviceMonitor();
    static void objectAddedCb(void *data, uint32_t id, uint32_t permissions, const char *type,
                              uint32_t version, const struct spa_dict *props);
    static void objectRemovedCb(void *data, uint32_t id);
    void objectAdded(ObjectId id, uint32_t permissions, PipewireRegistryType, uint32_t version,
                     const spa_dict &props);

    // default metadata
    void startListenDefaultMetadata(ObjectId, uint32_t version);
    struct MetadataRecord
    {
        const char *key;
        const char *type;
        const char *value;
    };

    int handleMetadata(const MetadataRecord &record);

    PwMetadataHandle m_defaultMetadata;
    struct spa_hook m_defaultMetadataListener{};

    QMutex m_activeStreamMutex;
    std::set<std::shared_ptr<QPipewireAudioStream>> m_activeStreams;
    void stopActiveStreams();
};

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_AUDIOCONTEXTMANAGER_P_H
