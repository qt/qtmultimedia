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
#include <pipewire/extensions/metadata.h>
#include <system_error>

#include <pipewire/pipewire.h>

#if !PW_CHECK_VERSION(0, 3, 75)
extern "C" {
bool pw_check_library_version(int major, int minor, int micro);
}
#endif

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
using PwMetadataHandle = MakeUniquePtr<pw_proxy, pw_proxy_destroy, pw_metadata>::type;

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

// strong id types

template <typename T, typename Tag>
struct StrongIdType
{
    explicit StrongIdType(T arg) : value{ arg } { }

    T value;
    friend QDebug operator<<(QDebug dbg, const StrongIdType &self) { return dbg << self.value; }

#ifdef __cpp_impl_three_way_comparison
    auto operator<=>(const StrongIdType &) const = default;
#else
    friend bool comparesEqual(const StrongIdType &lhs, const StrongIdType &rhs) noexcept
    {
        return lhs.value == rhs.value;
    }

    friend Qt::strong_ordering compareThreeWay(const StrongIdType &lhs,
                                               const StrongIdType &rhs) noexcept
    {
        return qCompareThreeWay(lhs.value, rhs.value);
    }

    Q_DECLARE_STRONGLY_ORDERED(StrongIdType)
#endif
};

struct ObjectIdTag
{
};

// PW_KEY_OBJECT_ID
// global object ID, can be reused
using ObjectId = StrongIdType<uint32_t, ObjectIdTag>;

struct ObjectSerialTag
{
};

// PW_KEY_OBJECT_SERIAL
// unique serial for each object
using ObjectSerial = StrongIdType<uint64_t, ObjectSerialTag>;

} // namespace QtPipeWire

// debug support
QDebug operator<<(QDebug dbg, const spa_dict &dict);
QDebug operator<<(QDebug dbg, const spa_pod &pod);
QDebug operator<<(QDebug dbg, enum pw_stream_state);
QDebug operator<<(QDebug dbg, const pw_time &state);

// Address sanitizer helper
// for now these annotations only function as documentation
#define QT_MM_GUARDED_BY(Mutex)

QT_END_NAMESPACE

#endif // QPIPEWIRE_SUPPORT_P_H
