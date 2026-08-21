// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_instance_p.h"

#include <QtMultimedia/private/qpipewire_propertydict_p.h>
#include <QtMultimedia/private/qpipewire_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#if QT_CONFIG(pipewire_symbolloader)
#  include <QtMultimedia/private/qpipewire_symbolloader_p.h>
#endif

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmutex.h>

#include <spa/utils/names.h>

#include <array>
#include <mutex>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

Q_LOGGING_CATEGORY(lcPipewire, "qt.multimedia.pipewire");

namespace {

struct InstanceHolder
{
    QMutex mutex;
    std::weak_ptr<QPipeWireInstance> instance;
};

Q_GLOBAL_STATIC(InstanceHolder, s_pipeWireInstance);

} // namespace

QPipewireInitializer::QPipewireInitializer()
{
    pw_init(nullptr, nullptr);

    qCDebug(lcPipewire) << "PipeWire initialized: compiled against" << pw_get_headers_version()
                        << " running " << pw_get_library_version();
}

QPipewireInitializer::~QPipewireInitializer()
{
    if (pw_check_library_version(0, 3, 49))
        pw_deinit();
}

std::shared_ptr<QPipeWireInstance> QPipeWireInstance::instance()
{
    std::lock_guard guard{ s_pipeWireInstance->mutex };
    std::shared_ptr<QPipeWireInstance> ret = s_pipeWireInstance->instance.lock();
    if (!ret) {
        ret = QPipeWireInstance::create();
        if (ret)
            s_pipeWireInstance->instance = ret;
    }
    return ret;
}

bool QPipeWireInstance::isLoaded()
{
#if QT_CONFIG(pipewire_symbolloader)
    return qPipewireIsLoaded();
#else
    return 1;
#endif
}

q23::expected<void, std::error_code> QPipeWireInstance::hasSPAFactory(const char *factoryName)
{
    std::array<spa_support, 32> support{};
    uint32_t n_support = pw_get_support(support.data(), uint32_t(support.size()));

    SpaHandleHandle handle{
        pw_load_spa_handle(/*lib=*/nullptr, factoryName, /*info=*/nullptr, n_support,
                           support.data()),
    };
    if (!handle)
        return q23::unexpected{ make_error_code() };

    return {};
}

bool QPipeWireInstance::isInPwThreadLoop() const
{
    return m_eventLoop.isInThread();
}

pw_loop *QPipeWireInstance::eventLoop() const
{
    return m_eventLoop.loop();
}

pw_context *QPipeWireInstance::context() const
{
    return m_context.get();
}

std::unique_ptr<QPipeWireInstance> QPipeWireInstance::create()
{
    auto instance = std::unique_ptr<QPipeWireInstance>(new QPipeWireInstance);
    if (!instance->m_eventLoop || !instance->m_context)
        return nullptr;

    return instance;
}

QPipeWireInstance::QPipeWireInstance()
{
    if (!m_eventLoop)
        return;

    int status = m_eventLoop.start();
    if (status < 0)
        qFatal() << "Failed to start event loop" << make_error_code(-status).message();

    m_context = runWithEventLoopLock([&] {
        auto applicationName = qApp->applicationName().toUtf8();
        PwPropertiesHandle props = makeProperties({
                { PW_KEY_APP_NAME, applicationName.data() },
        });

        return PwContextHandle{
            pw_context_new(m_eventLoop.loop(), props.release(), /*user_data_size=*/0),
        };
    });
}

QPipeWireInstance::~QPipeWireInstance()
{
    runWithEventLoopLock([&] { m_context = {}; });
    m_eventLoop.stop();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
