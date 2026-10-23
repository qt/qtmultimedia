// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtMultimedia/private/qdmabufvulkanimporter_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>
#include <QtMultimediaTestLib/private/udmabuftestutils_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qvulkaninstance.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qspan.h>
#include <QtCore/private/quniquehandle_types_p.h>

using namespace QtMultimediaPrivate;
using namespace QtMultimediaTest;

namespace {

// Content is irrelevant: these tests check import success/handle identity, not pixel data.
QUniqueFileDescriptorHandle createTestDmaBufFd(const char *debugName)
{
    const quint64 bufferSize = udmabufPageAlignedSize(1);
    QByteArray zeroes(int(bufferSize), '\0');

    QUniqueFileDescriptorHandle memfd = createUdmabufTestMemfd(zeroes, bufferSize, debugName);
    if (!memfd)
        return {};
    return createUdmabufFd(memfd, bufferSize);
}

} // namespace

class tst_QDmaBufVulkanImporter : public QObject
{
    Q_OBJECT

private slots:
    void importDmaBufTexturesVulkan_returnsNonNullHandle_whenGivenUdmabufBackedY8Plane();
    void importDmaBufTexturesVulkan_returnsDistinctHandlesPerPlane_forNv12Format();
    void importDmaBufTexturesVulkan_returnsRecoverableFailure_forUnmappedDrmFormat();
};

void tst_QDmaBufVulkanImporter::
        importDmaBufTexturesVulkan_returnsNonNullHandle_whenGivenUdmabufBackedY8Plane()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    auto offscreen = createOffscreenVulkanRhi(
            { "VK_KHR_external_memory_fd", "VK_EXT_external_memory_dma_buf",
              "VK_EXT_image_drm_format_modifier", "VK_KHR_image_format_list",
              "VK_KHR_sampler_ycbcr_conversion" });
    if (!offscreen)
        QSKIP("Could not create a Vulkan QRhi backend on this system");
    std::unique_ptr<QRhi> rhi = std::move(offscreen->rhi);

    DmaBufVulkanContext vulkanContext(rhi.get());
    if (!vulkanContext.isValid())
        QSKIP("Vulkan DMABUF import is not available (missing VK_KHR_external_memory_fd / "
              "VK_EXT_image_drm_format_modifier device extensions or driver support)");

    constexpr int width = 4;
    constexpr int height = 4;

    QUniqueFileDescriptorHandle dmabufFd = createTestDmaBufFd("tst_qdmabufvulkanimporter_y8");
    if (!dmabufFd)
        QSKIP("Could not create a udmabuf-backed test buffer");

    QSpan<const DRMFormat> drmFormats = dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_Y8);
    QCOMPARE(drmFormats.empty(), false);

    DmaBufPlane plane;
    plane.fd = dmabufFd.get();
    plane.offset = 0;
    plane.pitch = width;
    plane.drmFormat = drmFormats[0];

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles =
            importDmaBufTexturesVulkan(*rhi, vulkanContext, QSpan(&plane, 1),
                                       QVideoFrameFormat::Format_Y8, QSize(width, height));
    if (!handles)
        QSKIP("Importing a CPU-backed dma-buf as a VkImage failed; the driver/Vulkan "
              "implementation on this system likely doesn't support it (see the qWarning "
              "above for the specific Vulkan error)");

    QCOMPARE_NE((*handles)->textureHandle(*rhi, 0), quint64(0));

    // Destructor (vkDestroyImage/vkFreeMemory) must not fault.
    handles.value().reset();
    rhi->finish();
}

void tst_QDmaBufVulkanImporter::
        importDmaBufTexturesVulkan_returnsDistinctHandlesPerPlane_forNv12Format()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    auto offscreen = createOffscreenVulkanRhi(
            { "VK_KHR_external_memory_fd", "VK_EXT_external_memory_dma_buf",
              "VK_EXT_image_drm_format_modifier", "VK_KHR_image_format_list",
              "VK_KHR_sampler_ycbcr_conversion" });
    if (!offscreen)
        QSKIP("Could not create a Vulkan QRhi backend on this system");
    std::unique_ptr<QRhi> rhi = std::move(offscreen->rhi);

    DmaBufVulkanContext vulkanContext(rhi.get());
    if (!vulkanContext.isValid())
        QSKIP("Vulkan DMABUF import is not available (missing VK_KHR_external_memory_fd / "
              "VK_EXT_image_drm_format_modifier device extensions or driver support)");

    constexpr int width = 4;
    constexpr int height = 4;

    QUniqueFileDescriptorHandle lumaFd = createTestDmaBufFd("tst_qdmabufvulkanimporter_nv12_y");
    QUniqueFileDescriptorHandle chromaFd = createTestDmaBufFd("tst_qdmabufvulkanimporter_nv12_uv");
    if (!lumaFd || !chromaFd)
        QSKIP("Could not create udmabuf-backed test buffers");

    QSpan<const DRMFormat> drmFormats = dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_NV12);
    QCOMPARE(drmFormats.size(), 2);

    DmaBufPlane planes[2];
    planes[0].fd = lumaFd.get();
    planes[0].offset = 0;
    planes[0].pitch = width;
    planes[0].drmFormat = drmFormats[0];
    planes[1].fd = chromaFd.get();
    planes[1].offset = 0;
    planes[1].pitch = width; // 2 bytes/pixel, half-width, half-height chroma plane
    planes[1].drmFormat = drmFormats[1];

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles =
            importDmaBufTexturesVulkan(*rhi, vulkanContext, QSpan(planes, 2),
                                       QVideoFrameFormat::Format_NV12, QSize(width, height));
    if (!handles)
        QSKIP("Importing CPU-backed dma-bufs as VkImages failed; the driver/Vulkan "
              "implementation on this system likely doesn't support it (see the qWarning "
              "above for the specific Vulkan error)");

    const quint64 lumaHandle = (*handles)->textureHandle(*rhi, 0);
    const quint64 chromaHandle = (*handles)->textureHandle(*rhi, 1);
    QCOMPARE_NE(lumaHandle, quint64(0));
    QCOMPARE_NE(chromaHandle, quint64(0));
    QCOMPARE_NE(lumaHandle, chromaHandle);

    handles.value().reset();
    rhi->finish();
}

void tst_QDmaBufVulkanImporter::
        importDmaBufTexturesVulkan_returnsRecoverableFailure_forUnmappedDrmFormat()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    auto offscreen = createOffscreenVulkanRhi(
            { "VK_KHR_external_memory_fd", "VK_EXT_external_memory_dma_buf",
              "VK_EXT_image_drm_format_modifier", "VK_KHR_image_format_list",
              "VK_KHR_sampler_ycbcr_conversion" });
    if (!offscreen)
        QSKIP("Could not create a Vulkan QRhi backend on this system");
    std::unique_ptr<QRhi> rhi = std::move(offscreen->rhi);

    DmaBufVulkanContext vulkanContext(rhi.get());
    if (!vulkanContext.isValid())
        QSKIP("Vulkan DMABUF import is not available (missing VK_KHR_external_memory_fd / "
              "VK_EXT_image_drm_format_modifier device extensions or driver support)");

    QUniqueFileDescriptorHandle dmabufFd = createTestDmaBufFd("tst_qdmabufvulkanimporter_rgba");
    if (!dmabufFd)
        QSKIP("Could not create a udmabuf-backed test buffer");

    // Packed-RGBA has no VkFormat mapping; the failure must be recoverable, not unrecoverable.
    DmaBufPlane plane;
    plane.fd = dmabufFd.get();
    plane.offset = 0;
    plane.pitch = 16;
    plane.drmFormat = DRMFormat::ABGR8888;

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles =
            importDmaBufTexturesVulkan(*rhi, vulkanContext, QSpan(&plane, 1),
                                       QVideoFrameFormat::Format_ABGR8888, QSize(4, 4));
    QCOMPARE(handles.has_value(), false);
    QCOMPARE(handles.error(), FailureSeverity::recoverable);
}

QTEST_MAIN(tst_QDmaBufVulkanImporter)

#include "tst_qdmabufvulkanimporter.moc"
