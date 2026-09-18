// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabufvulkanimporter_p.h"

#include <QtMultimedia/private/qmultimedia_vulkan_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/qvulkanfunctions.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <unistd.h>

static_assert(QT_CONFIG(linux_dmabuf_vulkan));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLDmaBufVulkan, "qt.multimedia.dmabuf.vulkan")

namespace QtMultimediaPrivate {

namespace {

// Packed-RGBA is deliberately left unsupported: vaExportSurfaceHandle() never produces it.
VkFormat vkFormatForDrmFourcc(DRMFormat format)
{
    switch (format) {
    case DRMFormat::R8:
        return VK_FORMAT_R8_UNORM;
    case DRMFormat::R16:
        return VK_FORMAT_R16_UNORM;
    case DRMFormat::GR88:
    case DRMFormat::RG88:
        // No Vulkan format has swapped (G,R) byte order; chroma shaders must swap R/G themselves.
        return VK_FORMAT_R8G8_UNORM;
    case DRMFormat::GR1616:
    case DRMFormat::RG1616:
        return VK_FORMAT_R16G16_UNORM;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

} // namespace

DmaBufVulkanContext::DmaBufVulkanContext(QRhi *rhi)
{
    if (!rhi || rhi->backend() != QRhi::Vulkan) {
        qCDebug(qLDmaBufVulkan) << "no rhi or non-Vulkan RHI";
        return;
    }

    const auto *handles = static_cast<const QRhiVulkanNativeHandles *>(rhi->nativeHandles());
    if (!handles || !handles->inst || !handles->dev || !handles->physDev) {
        qCDebug(qLDmaBufVulkan) << "    missing Vulkan native handles, disabling";
        return;
    }

    m_physicalDevice = handles->physDev;
    m_device = handles->dev;

    m_functions = handles->inst->functions();
    m_deviceFunctions = handles->inst->deviceFunctions(m_device);
    if (!m_functions || !m_deviceFunctions) {
        qCDebug(qLDmaBufVulkan) << "    no QVulkanFunctions/QVulkanDeviceFunctions, disabling";
        return;
    }

    m_vkGetMemoryFdPropertiesKHR = reinterpret_cast<PFN_vkGetMemoryFdPropertiesKHR>(
            m_functions->vkGetDeviceProcAddr(m_device, "vkGetMemoryFdPropertiesKHR"));
    if (!m_vkGetMemoryFdPropertiesKHR) {
        qCDebug(qLDmaBufVulkan) << "    vkGetMemoryFdPropertiesKHR unavailable "
                                   "(VK_KHR_external_memory_fd likely not enabled), disabling";
        return;
    }

    m_valid = true;
}

VulkanDmaBufTextureHandles::VulkanDmaBufTextureHandles(QVulkanDeviceFunctions *deviceFunctions,
                                                       VkDevice device, int nPlanes,
                                                       std::array<Plane, 4> planes,
                                                       std::shared_ptr<void> parentKeepAlive)
    : m_parentKeepAlive(std::move(parentKeepAlive)),
      m_deviceFunctions(deviceFunctions),
      m_device(device),
      m_nPlanes(nPlanes),
      m_planes(planes)
{
}

VulkanDmaBufTextureHandles::~VulkanDmaBufTextureHandles()
{
    for (int i = 0; i < m_nPlanes; ++i) {
        if (m_planes[i].image != VK_NULL_HANDLE)
            m_deviceFunctions->vkDestroyImage(m_device, m_planes[i].image, nullptr);
        if (m_planes[i].memory != VK_NULL_HANDLE)
            m_deviceFunctions->vkFreeMemory(m_device, m_planes[i].memory, nullptr);
    }
}

q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importDmaBufTexturesVulkan(QRhi &rhi, const DmaBufVulkanContext &vulkanContext,
                           QSpan<const DmaBufPlane> planes, QVideoFrameFormat::PixelFormat qtFormat,
                           QSize frameSize, std::shared_ptr<void> parentKeepAlive)
{
    auto *desc = QVideoTextureHelper::textureDescription(qtFormat);
    int nPlanes = desc->nplanes;
    Q_ASSERT(planes.size() == nPlanes);

    const VkDevice device = vulkanContext.device();
    const VkPhysicalDevice physicalDevice = vulkanContext.physicalDevice();
    QVulkanFunctions *functions = vulkanContext.functions();
    QVulkanDeviceFunctions *deviceFunctions = vulkanContext.deviceFunctions();

    VkPhysicalDeviceMemoryProperties memProperties{};
    functions->vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    std::array<VulkanDmaBufTextureHandles::Plane, 4> resultPlanes{};

    auto releasePlanes = qScopeGuard([&] {
        for (int i = 0; i < nPlanes; ++i) {
            if (resultPlanes[i].image != VK_NULL_HANDLE)
                deviceFunctions->vkDestroyImage(device, resultPlanes[i].image, nullptr);
            if (resultPlanes[i].memory != VK_NULL_HANDLE)
                deviceFunctions->vkFreeMemory(device, resultPlanes[i].memory, nullptr);
        }
    });

    for (int i = 0; i < nPlanes; ++i) {
        const DmaBufPlane &plane = planes[i];
        const QSize planeSize = desc->rhiPlaneSize(frameSize, i, &rhi);

        const VkFormat format = vkFormatForDrmFourcc(plane.drmFormat);
        if (format == VK_FORMAT_UNDEFINED) {
            qCWarning(qLDmaBufVulkan) << "no VkFormat mapping for DRM format" << plane.drmFormat
                                      << ", disabling Vulkan dma-buf import for this frame";
            return q23::unexpected{ FailureSeverity::recoverable };
        }

        QUniqueFileDescriptorHandle dupFd{ dup(plane.fd) };
        if (!dupFd) {
            qCWarning(qLDmaBufVulkan) << "dup() failed for dma-buf plane" << i;
            return q23::unexpected{ FailureSeverity::recoverable };
        }

        const uint64_t modifier = plane.modifier == DmaBufFormatModifierInvalid
                ? uint64_t(DrmFormatModifierLinear)
                : uint64_t(plane.modifier);

        const VkSubresourceLayout planeLayout{
            .offset = VkDeviceSize(plane.offset),
            .size = 0,
            .rowPitch = VkDeviceSize(plane.pitch),
            .arrayPitch = 0,
            .depthPitch = 0,
        };

        const VkImageDrmFormatModifierExplicitCreateInfoEXT modifierInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT,
            .pNext = nullptr,
            .drmFormatModifier = modifier,
            .drmFormatModifierPlaneCount = 1,
            .pPlaneLayouts = &planeLayout
        };

        const VkExternalMemoryImageCreateInfo extMemImageInfo{
            .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
            .pNext = &modifierInfo,
            .handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
        };

        const VkImageCreateInfo imageInfo{
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .pNext = &extMemImageInfo,
            .flags = 0,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent =
                    VkExtent3D{
                            .width = uint32_t(planeSize.width()),
                            .height = uint32_t(planeSize.height()),
                            .depth = 1u,
                    },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
            .usage = VK_IMAGE_USAGE_SAMPLED_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VkImage image = VK_NULL_HANDLE;
        const VkResult createImageResult =
                deviceFunctions->vkCreateImage(device, &imageInfo, nullptr, &image);
        if (createImageResult != VK_SUCCESS) {
            // This device can't create a DRM-format-modifier-tiled image in this format at all;
            // every subsequent frame of the same pixel format will fail identically.
            qCWarning(qLDmaBufVulkan)
                    << "vkCreateImage failed for dma-buf plane" << i << ":" << createImageResult;
            return q23::unexpected{ FailureSeverity::unrecoverable };
        }
        auto destroyImageOnFailure = qScopeGuard([&] {
            deviceFunctions->vkDestroyImage(device, image, nullptr);
        });

        VkMemoryFdPropertiesKHR fdProperties{
            .sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR,
            .pNext = nullptr,
            .memoryTypeBits = 0,
        };
        const std::optional<VkResult> fdPropertiesResult = vulkanContext.vkGetMemoryFdPropertiesKHR(
                VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, dupFd.get(), &fdProperties);
        if (fdPropertiesResult != VK_SUCCESS) {
            qCWarning(qLDmaBufVulkan) << "vkGetMemoryFdPropertiesKHR failed for dma-buf plane" << i
                                      << ":" << fdPropertiesResult.value_or(VK_ERROR_UNKNOWN);
            return q23::unexpected{ FailureSeverity::recoverable };
        }

        VkMemoryRequirements memRequirements{};
        deviceFunctions->vkGetImageMemoryRequirements(device, image, &memRequirements);

        const uint32_t typeBits = memRequirements.memoryTypeBits & fdProperties.memoryTypeBits;
        int memoryTypeIndex = -1;
        for (uint32_t t = 0; t < memProperties.memoryTypeCount; ++t) {
            if (typeBits & (1u << t)) {
                memoryTypeIndex = int(t);
                break;
            }
        }
        if (memoryTypeIndex < 0) {
            // This device's memory heaps never overlap with what the exporting driver offers;
            // every subsequent frame will hit the same mismatch.
            qCWarning(qLDmaBufVulkan) << "no compatible Vulkan memory type for dma-buf plane" << i;
            return q23::unexpected{ FailureSeverity::unrecoverable };
        }

        const VkImportMemoryFdInfoKHR importInfo{
            .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
            .pNext = nullptr,
            .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
            .fd = dupFd.get(),
        };

        const VkMemoryDedicatedAllocateInfo dedicatedInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
            .pNext = &importInfo,
            .image = image,
            .buffer = VK_NULL_HANDLE,
        };

        const VkMemoryAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .pNext = &dedicatedInfo,
            .allocationSize = memRequirements.size,
            .memoryTypeIndex = uint32_t(memoryTypeIndex),
        };

        VkDeviceMemory memory = VK_NULL_HANDLE;
        const VkResult allocateMemoryResult =
                deviceFunctions->vkAllocateMemory(device, &allocInfo, nullptr, &memory);
        if (allocateMemoryResult != VK_SUCCESS) {
            qCWarning(qLDmaBufVulkan) << "vkAllocateMemory (dma-buf import) failed for plane" << i
                                      << ":" << allocateMemoryResult;
            return q23::unexpected{ FailureSeverity::recoverable };
        }
        std::ignore = dupFd.release(); // vkAllocateMemory took ownership of dupFd

        const VkResult bindImageMemoryResult =
                deviceFunctions->vkBindImageMemory(device, image, memory, 0);
        if (bindImageMemoryResult != VK_SUCCESS) {
            qCWarning(qLDmaBufVulkan) << "vkBindImageMemory failed for dma-buf plane" << i << ":"
                                      << bindImageMemoryResult;
            deviceFunctions->vkFreeMemory(device, memory, nullptr);
            return q23::unexpected{ FailureSeverity::recoverable };
        }

        destroyImageOnFailure.dismiss();
        resultPlanes[i] = VulkanDmaBufTextureHandles::Plane{
            .image = image,
            .memory = memory,
        };
    }

    releasePlanes.dismiss();

    return std::make_unique<VulkanDmaBufTextureHandles>(deviceFunctions, device, nPlanes,
                                                        resultPlanes, std::move(parentKeepAlive));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
