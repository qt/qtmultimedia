// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPIPEWIRE_SUPPORT_P_H
#define QPIPEWIRE_SUPPORT_P_H

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
#include <QtCore/qdebug.h>

#include <memory>
#include <system_error>

#include <pipewire/pipewire.h>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

// errno helper
inline std::error_code make_error_code(int errnoValue = errno)
{
    return std::error_code(errnoValue, std::generic_category());
}

// unique_ptr utilities
template <typename BaseType, void(Deleter)(BaseType *), typename Type = BaseType>
struct HandleDeleter
{
    void operator()(Type *handle) const
    {
        BaseType *base = reinterpret_cast<BaseType *>(handle);
        (Deleter)(base);
    }
};

template <typename BaseType, void(Deleter)(BaseType *), typename Type = BaseType>
struct MakeUniquePtr
{
    using type = std::unique_ptr<Type, HandleDeleter<BaseType, Deleter, Type>>;
};

// unique_ptr for pipewire types

using PwLoopHandle = MakeUniquePtr<pw_loop, pw_loop_destroy>::type;
using PwContextHandle = MakeUniquePtr<pw_context, pw_context_destroy>::type;
using PwPropertiesHandle = MakeUniquePtr<pw_properties, pw_properties_free>::type;
using PwThreadLoopHandle = MakeUniquePtr<pw_thread_loop, pw_thread_loop_destroy>::type;
using PwStreamHandle = MakeUniquePtr<pw_stream, pw_stream_destroy>::type;
using PwRegistryHandle = MakeUniquePtr<pw_proxy, pw_proxy_destroy, pw_registry>::type;
using PwNodeHandle = MakeUniquePtr<pw_proxy, pw_proxy_destroy, pw_node>::type;

struct PwCoreConnectionDeleter
{
    void operator()(pw_core *handle) const
    {
        int status = pw_core_disconnect(handle);
        if (status < 0)
            qWarning() << "Failed to disconnect pw_core:" << make_error_code(status).message();
    }
};

using PwCoreConnectionHandle = std::unique_ptr<pw_core, PwCoreConnectionDeleter>;

} // namespace QtPipeWire

QT_END_NAMESPACE

#endif // QPIPEWIRE_SUPPORT_P_H
