// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_VULKAN_SUPPORT_P_H
#define QMULTIMEDIA_VULKAN_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtGui/qvulkaninstance.h> // defines VK_NO_PROTOTYPES before pulling in vulkan.h
#include <QtCore/qspan.h>
#include <QtCore/quuid.h>

static_assert(QT_CONFIG(vulkan));

QT_BEGIN_NAMESPACE

class QDebug;

Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug debug, VkResult result);

namespace QtMultimediaPrivate {

template <typename Func>
Func resolveVulkanFunction(QVulkanInstance &inst, const char *name)
{
    return reinterpret_cast<Func>(inst.getInstanceProcAddr(name));
}

// Resolves a device-level function, e.g. one of an extension such as VK_KHR_external_memory_fd.
// Unlike resolveVulkanFunction(), which yields a loader trampoline even when the extension is not
// enabled on the device (and crashes when called), this returns nullptr if the function is not
// available on the given device.
template <typename Func>
Func resolveVulkanDeviceFunction(QVulkanInstance &inst, VkDevice dev, const char *name)
{
    const auto getDeviceProcAddr =
            resolveVulkanFunction<PFN_vkGetDeviceProcAddr>(inst, "vkGetDeviceProcAddr");
    return getDeviceProcAddr ? reinterpret_cast<Func>(getDeviceProcAddr(dev, name)) : nullptr;
}

inline QUuid uuidFromVkUuid(QSpan<const uint8_t, VK_UUID_SIZE> vkUuidBytes)
{
    static_assert(VK_UUID_SIZE == 16);
    return QUuid::fromRfc4122(
            QByteArrayView(reinterpret_cast<const char *>(vkUuidBytes.data()), VK_UUID_SIZE));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QMULTIMEDIA_VULKAN_SUPPORT_P_H
