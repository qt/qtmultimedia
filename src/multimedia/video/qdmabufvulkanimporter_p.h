// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QDMABUFVULKANIMPORTER_P_H
#define QDMABUFVULKANIMPORTER_P_H

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

#include "qhwvideobuffer_p.h"

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qdmabufimportfailure_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

#include <QtGui/rhi/qrhi.h>
#include <QtGui/qvulkaninstance.h> // defines VK_NO_PROTOTYPES before pulling in vulkan.h

#include <QtCore/qspan.h>
#include <QtCore/private/qexpected_p.h>

#include <array>
#include <memory>
#include <optional>

static_assert(QT_CONFIG(linux_dmabuf_vulkan));

QT_BEGIN_NAMESPACE

class QRhi;

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT DmaBufVulkanContext
{
public:
    explicit DmaBufVulkanContext(QRhi *rhi);

    bool isValid() const { return m_valid; }
    void invalidate() { m_valid = false; }

    VkPhysicalDevice physicalDevice() const { return m_physicalDevice; }
    VkDevice device() const { return m_device; }
    QVulkanFunctions *functions() const { return m_functions; }
    QVulkanDeviceFunctions *deviceFunctions() const { return m_deviceFunctions; }

    std::optional<VkResult>
    vkGetMemoryFdPropertiesKHR(VkExternalMemoryHandleTypeFlagBits handleType, int fd,
                               VkMemoryFdPropertiesKHR *properties) const
    {
        if (!m_vkGetMemoryFdPropertiesKHR)
            return std::nullopt;
        return m_vkGetMemoryFdPropertiesKHR(m_device, handleType, fd, properties);
    }

private:
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    QVulkanFunctions *m_functions = nullptr;
    QVulkanDeviceFunctions *m_deviceFunctions = nullptr;
    PFN_vkGetMemoryFdPropertiesKHR m_vkGetMemoryFdPropertiesKHR = nullptr;
    bool m_valid = false;
};

class Q_MULTIMEDIA_EXPORT VulkanDmaBufTextureHandles final : public QVideoFrameTexturesHandles
{
public:
    struct Plane
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
    };

    VulkanDmaBufTextureHandles(QVulkanDeviceFunctions *deviceFunctions, VkDevice device,
                               int nPlanes, std::array<Plane, 4> planes,
                               std::shared_ptr<void> parentKeepAlive = {});

    ~VulkanDmaBufTextureHandles() override;

    quint64 textureHandle(QRhi &, int plane) override { return quint64(m_planes[plane].image); }

private:
    const std::shared_ptr<void> m_parentKeepAlive; // keep the backend alive
    QVulkanDeviceFunctions *const m_deviceFunctions;
    const VkDevice m_device;
    const int m_nPlanes;
    const std::array<Plane, 4> m_planes;
};

// Same recoverable/unrecoverable failure semantics as importDmaBufTextures (the EGL importer).
Q_MULTIMEDIA_EXPORT q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importDmaBufTexturesVulkan(QRhi &rhi, const DmaBufVulkanContext &vulkanContext,
                           QSpan<const DmaBufPlane> planes, QVideoFrameFormat::PixelFormat qtFormat,
                           QSize frameSize, std::shared_ptr<void> parentKeepAlive = {});

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QDMABUFVULKANIMPORTER_P_H
