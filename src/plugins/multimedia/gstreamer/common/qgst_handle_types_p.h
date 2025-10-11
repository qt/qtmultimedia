// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGST_HANDLE_TYPES_P_H
#define QGST_HANDLE_TYPES_P_H

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

#include <QtCore/private/qcore_unix_p.h>
#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qtconfigmacros.h>

#include <QtMultimedia/private/qtmultimedia-config_p.h>

#include <gst/gst.h>

#if QT_CONFIG(gstreamer_gl)
#  include <gst/gl/gstglcontext.h>
#endif

QT_BEGIN_NAMESPACE

namespace QGstImpl {

template <typename HandleTraits>
struct QSharedHandle : private QUniqueHandle<HandleTraits>
{
    using BaseClass = QUniqueHandle<HandleTraits>;

    enum RefMode { HasRef, NeedsRef };

    QSharedHandle() = default;

    explicit QSharedHandle(typename HandleTraits::Type object, RefMode mode)
        : BaseClass{ mode == NeedsRef ? HandleTraits::ref(object) : object }
    {
    }

    QSharedHandle(const QSharedHandle &o)
        : BaseClass{
              HandleTraits::ref(o.get()),
          }
    {
    }

    QSharedHandle(QSharedHandle &&) noexcept = default;

    QSharedHandle &operator=(const QSharedHandle &o) // NOLINT: bugprone-unhandled-self-assign
    {
        if (BaseClass::get() != o.get())
            reset(HandleTraits::ref(o.get()));
        return *this;
    };

    QSharedHandle &operator=(QSharedHandle &&) noexcept = default;

    [[nodiscard]] friend bool operator==(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() == rhs.get();
    }

    [[nodiscard]] friend bool operator!=(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() != rhs.get();
    }

    [[nodiscard]] friend bool operator<(const QSharedHandle &lhs, const QSharedHandle &rhs) noexcept
    {
        return lhs.get() < rhs.get();
    }

    [[nodiscard]] friend bool operator<=(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() <= rhs.get();
    }

    [[nodiscard]] friend bool operator>(const QSharedHandle &lhs, const QSharedHandle &rhs) noexcept
    {
        return lhs.get() > rhs.get();
    }

    [[nodiscard]] friend bool operator>=(const QSharedHandle &lhs,
                                         const QSharedHandle &rhs) noexcept
    {
        return lhs.get() >= rhs.get();
    }

    using BaseClass::get;
    using BaseClass::isValid;
    using BaseClass::operator bool;
    using BaseClass::release;
    using BaseClass::reset;
    using BaseClass::operator&;
    using BaseClass::close;
};

struct QGstTagListHandleTraits
{
    using Type = GstTagList *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_tag_list_unref(handle);
        return true;
    }
    static Type ref(Type handle) noexcept { return gst_tag_list_ref(handle); }
};

struct QGstSampleHandleTraits
{
    using Type = GstSample *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_sample_unref(handle);
        return true;
    }
    static Type ref(Type handle) noexcept { return gst_sample_ref(handle); }
};

struct QUniqueGstStructureHandleTraits
{
    using Type = GstStructure *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_structure_free(handle);
        return true;
    }
};

struct QUniqueGStringHandleTraits
{
    using Type = gchar *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        g_free(handle);
        return true;
    }
};

struct QUniqueGErrorHandleTraits
{
    using Type = GError *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        g_error_free(handle);
        return true;
    }
};

struct QUniqueGDateHandleTraits
{
    using Type = GDate *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        g_date_free(handle);
        return true;
    }
};

struct QUniqueGstDateTimeHandleTraits
{
    using Type = GstDateTime *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_date_time_unref(handle);
        return true;
    }
};

struct QFileDescriptorHandleTraits
{
    using Type = int;
    static constexpr Type invalidValue() noexcept { return -1; }
    static bool close(Type fd) noexcept
    {
        int closeResult = qt_safe_close(fd);
        return closeResult == 0;
    }
};

template <typename GstType>
struct QGstHandleHelper
{
    struct QGstSafeObjectHandleTraits
    {
        using Type = GstType *;
        static constexpr Type invalidValue() noexcept { return nullptr; }
        static bool close(Type handle) noexcept
        {
            gst_object_unref(G_OBJECT(handle));
            return true;
        }

        static Type ref(Type handle) noexcept
        {
            gst_object_ref_sink(G_OBJECT(handle));
            return handle;
        }
    };

    using SharedHandle = QSharedHandle<QGstSafeObjectHandleTraits>;
    using UniqueHandle = QUniqueHandle<QGstSafeObjectHandleTraits>;
};

template <typename GstType>
struct QGstMiniObjectHandleHelper
{
    struct Traits
    {
        using Type = GstType *;
        static constexpr Type invalidValue() noexcept { return nullptr; }
        static bool close(Type handle) noexcept
        {
            gst_mini_object_unref(GST_MINI_OBJECT_CAST(handle));
            return true;
        }

        static Type ref(Type handle) noexcept
        {
            if (GST_MINI_OBJECT_CAST(handle))
                gst_mini_object_ref(GST_MINI_OBJECT_CAST(handle));
            return handle;
        }
    };

    using SharedHandle = QSharedHandle<Traits>;
    using UniqueHandle = QUniqueHandle<Traits>;
};

} // namespace QGstImpl

using QGstClockHandle = QGstImpl::QGstHandleHelper<GstClock>::UniqueHandle;
using QGstElementHandle = QGstImpl::QGstHandleHelper<GstElement>::UniqueHandle;
using QGstElementFactoryHandle = QGstImpl::QGstHandleHelper<GstElementFactory>::SharedHandle;
using QGstDeviceHandle = QGstImpl::QGstHandleHelper<GstDevice>::SharedHandle;
using QGstDeviceMonitorHandle = QGstImpl::QGstHandleHelper<GstDeviceMonitor>::UniqueHandle;
using QGstBusHandle = QGstImpl::QGstHandleHelper<GstBus>::SharedHandle;
using QGstStreamCollectionHandle = QGstImpl::QGstHandleHelper<GstStreamCollection>::SharedHandle;
using QGstStreamHandle = QGstImpl::QGstHandleHelper<GstStream>::SharedHandle;

using QGstTagListHandle = QGstImpl::QSharedHandle<QGstImpl::QGstTagListHandleTraits>;
using QGstSampleHandle = QGstImpl::QSharedHandle<QGstImpl::QGstSampleHandleTraits>;

using QUniqueGstStructureHandle = QUniqueHandle<QGstImpl::QUniqueGstStructureHandleTraits>;
using QUniqueGStringHandle = QUniqueHandle<QGstImpl::QUniqueGStringHandleTraits>;
using QUniqueGErrorHandle = QUniqueHandle<QGstImpl::QUniqueGErrorHandleTraits>;
using QUniqueGDateHandle = QUniqueHandle<QGstImpl::QUniqueGDateHandleTraits>;
using QUniqueGstDateTimeHandle = QUniqueHandle<QGstImpl::QUniqueGstDateTimeHandleTraits>;
using QFileDescriptorHandle = QUniqueHandle<QGstImpl::QFileDescriptorHandleTraits>;
using QGstBufferHandle = QGstImpl::QGstMiniObjectHandleHelper<GstBuffer>::SharedHandle;
using QGstContextHandle = QGstImpl::QGstMiniObjectHandleHelper<GstContext>::UniqueHandle;
using QGstGstDateTimeHandle = QGstImpl::QGstMiniObjectHandleHelper<GstDateTime>::SharedHandle;
using QGstPluginFeatureHandle = QGstImpl::QGstHandleHelper<GstPluginFeature>::SharedHandle;
using QGstQueryHandle = QGstImpl::QGstMiniObjectHandleHelper<GstQuery>::SharedHandle;

#if QT_CONFIG(gstreamer_gl)
using QGstGLContextHandle = QGstImpl::QGstHandleHelper<GstGLContext>::UniqueHandle;
using QGstGLDisplayHandle = QGstImpl::QGstHandleHelper<GstGLDisplay>::UniqueHandle;
#endif

QT_END_NAMESPACE

#endif
