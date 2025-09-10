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

#ifdef Q_OS_UNIX
#include <QtCore/private/qcore_unix_p.h>
#else
#include <qplatformdefs.h>
#endif

#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qtconfigmacros.h>

#include <QtMultimedia/private/qtmultimedia-config_p.h>
#include <QtMultimedia/private/qsharedhandle_p.h>

#include <gst/gst.h>

#if QT_CONFIG(gstreamer_gl)
#  include <gst/gl/gstglcontext.h>
#endif

QT_BEGIN_NAMESPACE

namespace QGstImpl {

struct QGstTagListHandleTraits
{
    using Type = GstTagList *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static Type ref(Type handle) noexcept { return gst_tag_list_ref(handle); }
    static bool unref(Type handle) noexcept
    {
        gst_tag_list_unref(handle);
        return true;
    }
};

struct QGstSampleHandleTraits
{
    using Type = GstSample *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static Type ref(Type handle) noexcept { return gst_sample_ref(handle); }
    static bool unref(Type handle) noexcept
    {
        gst_sample_unref(handle);
        return true;
    }
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

struct QGstDateTimeHandleTraits
{
    using Type = GstDateTime *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static Type ref(Type handle) noexcept { return gst_date_time_ref(handle); }
    static bool unref(Type handle) noexcept
    {
        gst_date_time_unref(handle);
        return true;
    }
};

template <typename GstType>
struct QGstHandleHelper
{
    struct QGstSafeObjectHandleTraits
    {
        using Type = GstType *;
        static constexpr Type invalidValue() noexcept { return nullptr; }
        static Type ref(Type handle) noexcept
        {
            gst_object_ref_sink(G_OBJECT(handle));
            return handle;
        }

        static bool unref(Type handle) noexcept
        {
            gst_object_unref(G_OBJECT(handle));
            return true;
        }
    };

    using SharedHandle = QtPrivate::QSharedHandle<QGstSafeObjectHandleTraits>;
};

template <typename GstType>
struct QGstMiniObjectHandleHelper
{
    struct Traits
    {
        using Type = GstType *;
        static constexpr Type invalidValue() noexcept { return nullptr; }

        static Type ref(Type handle) noexcept
        {
            if (GST_MINI_OBJECT_CAST(handle))
                gst_mini_object_ref(GST_MINI_OBJECT_CAST(handle));
            return handle;
        }

        static bool unref(Type handle) noexcept
        {
            gst_mini_object_unref(GST_MINI_OBJECT_CAST(handle));
            return true;
        }
    };

    using SharedHandle = QtPrivate::QSharedHandle<Traits>;
};

template <typename TypeArg>
struct QGObjectHandleHelper
{
    struct Traits
    {
        using Type = TypeArg *;
        static constexpr Type invalidValue() noexcept { return nullptr; }
        static Type ref(Type handle) noexcept
        {
            if (G_OBJECT(handle))
                g_object_ref(G_OBJECT(handle));
            return handle;
        }

        static bool unref(Type handle) noexcept
        {
            g_object_unref(G_OBJECT(handle));
            return true;
        }
    };

    using SharedHandle = QtPrivate::QSharedHandle<Traits>;
};

} // namespace QGstImpl

using QGstClockHandle = QGstImpl::QGstHandleHelper<GstClock>::SharedHandle;
using QGstElementHandle = QGstImpl::QGstHandleHelper<GstElement>::SharedHandle;
using QGstElementFactoryHandle = QGstImpl::QGstHandleHelper<GstElementFactory>::SharedHandle;
using QGstDeviceHandle = QGstImpl::QGstHandleHelper<GstDevice>::SharedHandle;
using QGstDeviceMonitorHandle = QGstImpl::QGstHandleHelper<GstDeviceMonitor>::SharedHandle;
using QGstBusHandle = QGstImpl::QGstHandleHelper<GstBus>::SharedHandle;
using QGstStreamCollectionHandle = QGstImpl::QGstHandleHelper<GstStreamCollection>::SharedHandle;
using QGstStreamHandle = QGstImpl::QGstHandleHelper<GstStream>::SharedHandle;

using QGstTagListHandle = QtPrivate::QSharedHandle<QGstImpl::QGstTagListHandleTraits>;
using QGstSampleHandle = QtPrivate::QSharedHandle<QGstImpl::QGstSampleHandleTraits>;

using QUniqueGstStructureHandle = QUniqueHandle<QGstImpl::QUniqueGstStructureHandleTraits>;
using QUniqueGStringHandle = QUniqueHandle<QGstImpl::QUniqueGStringHandleTraits>;
using QUniqueGErrorHandle = QUniqueHandle<QGstImpl::QUniqueGErrorHandleTraits>;
using QUniqueGDateHandle = QUniqueHandle<QGstImpl::QUniqueGDateHandleTraits>;
using QGstDateTimeHandle = QtPrivate::QSharedHandle<QGstImpl::QGstDateTimeHandleTraits>;
using QGstBufferHandle = QGstImpl::QGstMiniObjectHandleHelper<GstBuffer>::SharedHandle;
using QGstContextHandle = QGstImpl::QGstMiniObjectHandleHelper<GstContext>::SharedHandle;
using QGstGstDateTimeHandle = QGstImpl::QGstMiniObjectHandleHelper<GstDateTime>::SharedHandle;
using QGstPluginFeatureHandle = QGstImpl::QGstHandleHelper<GstPluginFeature>::SharedHandle;
using QGstQueryHandle = QGstImpl::QGstMiniObjectHandleHelper<GstQuery>::SharedHandle;
using QGstMessageHandle = QGstImpl::QGstMiniObjectHandleHelper<GstMessage>::SharedHandle;

#if QT_CONFIG(gstreamer_gl)
using QGstGLContextHandle = QGstImpl::QGstHandleHelper<GstGLContext>::SharedHandle;
using QGstGLDisplayHandle = QGstImpl::QGstHandleHelper<GstGLDisplay>::SharedHandle;
#endif

QT_END_NAMESPACE

#endif
