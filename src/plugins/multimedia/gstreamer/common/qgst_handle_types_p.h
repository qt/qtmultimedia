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

template <typename GstType>
struct QGstSafeObjectHandleTraits
{
    using Type = GstType *;
    static constexpr Type invalidValue() noexcept { return nullptr; }
    static bool close(Type handle) noexcept
    {
        gst_object_unref(G_OBJECT(handle));
        return true;
    }
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
};

using QGstClockHandleTraits = QGstSafeObjectHandleTraits<GstClock>;
using QGstElementHandleTraits = QGstSafeObjectHandleTraits<GstElement>;
using QGstElementFactoryHandleTraits = QGstSafeObjectHandleTraits<GstElementFactory>;
using QGstDeviceHandleTraits = QGstSafeObjectHandleTraits<GstDevice>;

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

#if QT_CONFIG(gstreamer_gl)
using QGstGLContextHandleTraits = QGstSafeObjectHandleTraits<GstGLContext>;
using QGstGLDisplayHandleTraits = QGstSafeObjectHandleTraits<GstGLDisplay>;
#endif

} // namespace QGstImpl

using QGstTagListHandle = QUniqueHandle<QGstImpl::QGstTagListHandleTraits>;
using QGstClockHandle = QUniqueHandle<QGstImpl::QGstClockHandleTraits>;
using QGstElementHandle = QUniqueHandle<QGstImpl::QGstElementHandleTraits>;
using QGstElementFactoryHandle = QUniqueHandle<QGstImpl::QGstElementFactoryHandleTraits>;
using QGstDeviceHandle = QUniqueHandle<QGstImpl::QGstDeviceHandleTraits>;
using QGstSampleHandle = QUniqueHandle<QGstImpl::QGstSampleHandleTraits>;
using QUniqueGstStructureHandle = QUniqueHandle<QGstImpl::QUniqueGstStructureHandleTraits>;
using QUniqueGStringHandle = QUniqueHandle<QGstImpl::QUniqueGStringHandleTraits>;
using QUniqueGErrorHandle = QUniqueHandle<QGstImpl::QUniqueGErrorHandleTraits>;
using QFileDescriptorHandle = QUniqueHandle<QGstImpl::QFileDescriptorHandleTraits>;

#if QT_CONFIG(gstreamer_gl)
using QGstGLContextHandle = QUniqueHandle<QGstImpl::QGstGLContextHandleTraits>;
using QGstGLDisplayHandle = QUniqueHandle<QGstImpl::QGstGLDisplayHandleTraits>;
#endif

QT_END_NAMESPACE

#endif
