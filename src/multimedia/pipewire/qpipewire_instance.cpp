// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_instance_p.h"

#include "qpipewire_symbolloader_p.h"
#include "qpipewire_support_p.h"

#include <QtCore/qmutex.h>
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

std::shared_ptr<QPipeWireInstance> QPipeWireInstance::instance()
{
    std::lock_guard guard{ s_pipeWireInstance->mutex };
    std::shared_ptr<QPipeWireInstance> ret = s_pipeWireInstance->instance.lock();
    if (!ret) {
        ret = std::make_shared<QPipeWireInstance>();
        s_pipeWireInstance->instance = ret;
    }
    return ret;
}

bool QPipeWireInstance::isLoaded()
{
    return qPipewireIsLoaded();
}

QPipeWireInstance::QPipeWireInstance()
{
    pw_init(nullptr, nullptr);

    qCDebug(lcPipewire) << "PipeWire initialized: compiled against" << pw_get_headers_version()
                        << " running " << pw_get_library_version();
}

QPipeWireInstance::~QPipeWireInstance()
{
    if (pw_check_library_version(0, 3, 49))
        pw_deinit();
}

} // namespace QtPipeWire

QT_END_NAMESPACE
