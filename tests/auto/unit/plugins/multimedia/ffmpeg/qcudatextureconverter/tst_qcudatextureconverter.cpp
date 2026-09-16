// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/hwframecontext_p.h"

#include <QtTest/qtest.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_cuda_p.h>

#include <QtCore/qsize.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/rhi/qrhi.h>

extern "C" {
#include <libavutil/hwcontext.h>
}

#include <memory>
#include <utility>
#include <vector>

using namespace QFFmpeg;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

namespace {

// Whether "swFormat" stores its samples as 8-bit (NV12/NV21) or 16-bit (P010/P016) words.
int cudaBytesPerSample(AVPixelFormat swFormat)
{
    return swFormat == AV_PIX_FMT_P010 || swFormat == AV_PIX_FMT_P016 ? 2 : 1;
}

QRhiTexture::Format cudaTextureFormat(int sampleSize, int numChannels)
{
    if (sampleSize == 1)
        return numChannels == 1 ? QRhiTexture::R8 : QRhiTexture::RG8;
    return numChannels == 1 ? QRhiTexture::R16 : QRhiTexture::RG16;
}

void configureFramesContext(AVHWFramesContext &)
{
    // No CUDA-specific frames-context fields to set; NVDEC's semi-planar pool defaults are
    // sufficient.
}

// swFormat must be one of the semi-planar formats CUDATextureConverter supports (NV12, NV21,
// P010, P016). CUDATextureConverter copies planes verbatim without interpreting sample bit
// depth, so this is a raw plane-byte round trip: lumaValue/chromaUValue/chromaVValue only need
// to be distinct byte patterns, not semantically valid pixel values.
q23::expected<HwFrameContext, QString> createCudaFrameContext(AVPixelFormat swFormat, QSize size,
                                                              quint16 lumaValue,
                                                              quint16 chromaUValue,
                                                              quint16 chromaVValue)
{
    const int sampleSize = cudaBytesPerSample(swFormat);
    const auto toBytes = [sampleSize](quint16 value) {
        return QByteArray(reinterpret_cast<const char *>(&value), sampleSize);
    };
    return createHwFrameContext(AV_HWDEVICE_TYPE_CUDA, AV_PIX_FMT_CUDA, swFormat, size,
                                { toBytes(lumaValue), toBytes(chromaUValue) + toBytes(chromaVValue) },
                                configureFramesContext);
}

q23::expected<HwFrameContext, QString> createCudaNv12(QSize size)
{
    return createCudaFrameContext(AV_PIX_FMT_NV12, size, 0x40, 0x60, 0x80);
}

q23::expected<HwFrameContext, QString> createCudaNv21(QSize size)
{
    return createCudaFrameContext(AV_PIX_FMT_NV21, size, 0x41, 0x61, 0x81);
}

// Chosen so that a byte-swap or a missing "<< 6" shift would produce a different (and thus
// detectable) byte pattern.
q23::expected<HwFrameContext, QString> createCudaP010(QSize size)
{
    return createCudaFrameContext(AV_PIX_FMT_P010, size, 0x141 << 6, 0x161 << 6, 0x181 << 6);
}

q23::expected<HwFrameContext, QString> createCudaP016(QSize size)
{
    return createCudaFrameContext(AV_PIX_FMT_P016, size, 0x1241, 0x2361, 0x3481);
}

// NV21/P016 share cudaFrameGeometry()'s NV12/P010 case, respectively.
q23::expected<HwFrameContext, QString> createCudaFrame(QVideoFrameFormat::PixelFormat pixelFormat,
                                                       QSize size)
{
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
        return createCudaNv12(size);
    case QVideoFrameFormat::Format_NV21:
        return createCudaNv21(size);
    case QVideoFrameFormat::Format_P010:
        return createCudaP010(size);
    case QVideoFrameFormat::Format_P016:
        return createCudaP016(size);
    default:
        Q_UNREACHABLE_RETURN(q23::unexpected(u"unsupported pixel format"_s));
    }
}

// Allocates a further hw frame with the same pixel format and size as an already-created
// CudaFrameContext, from its own hw frames context - e.g. to simulate a second frame of the same
// decode session (same underlying hw context, as consecutive decoded frames would share).
// Returns null on failure.
AVFrameUPtr createSecondCudaFrame(const HwFrameContext &context,
                                  const QList<QByteArray> &planeTexels)
{
    AVFrameUPtr swFrame = makeAVFrame();
    swFrame->format = QFFmpegVideoBuffer::toAVPixelFormat(context.pixelFormat);
    swFrame->width = context.size.width();
    swFrame->height = context.size.height();
    if (av_frame_get_buffer(swFrame.get(), 32) < 0)
        return nullptr;

    const auto *desc = QVideoTextureHelper::textureDescription(context.pixelFormat);
    for (int plane = 0; plane < desc->nplanes; ++plane) {
        const int width = desc->widthForPlane(context.size.width(), plane);
        const int height = desc->heightForPlane(context.size.height(), plane);
        QtMultimediaTest::fillPlaneRows(swFrame->data[plane], swFrame->linesize[plane], width,
                                        height, planeTexels[plane]);
    }

    AVFrameUPtr hwFrame = makeAVFrame();
    if (av_hwframe_get_buffer(context.framesContext.get(), hwFrame.get(), 0) < 0)
        return nullptr;
    if (av_hwframe_transfer_data(hwFrame.get(), swFrame.get(), 0) < 0)
        return nullptr;
    return hwFrame;
}

// Owns whichever backend-specific "keep alive" object (QOffscreenSurface or QVulkanInstance) the
// requested QRhi backend was built from. "rhi" is declared last so it is destroyed first (locals/
// members are destroyed in reverse declaration order), before the object(s) it depends on.
struct OffscreenRhi
{
#if QT_CONFIG(opengl)
    OffscreenGlRhi glRhi;
#endif
#if QT_CONFIG(vulkan)
    OffscreenVulkanRhi vulkanRhi;
#endif
    std::unique_ptr<QRhi> rhi;
};

OffscreenRhi createOffscreenRhi(QRhi::Implementation backend)
{
    OffscreenRhi result;
    switch (backend) {
#if QT_CONFIG(opengl)
    case QRhi::OpenGLES2: {
        q23::expected<OffscreenGlRhi, QString> glRhi = createOffscreenGlRhi();
        if (glRhi) {
            result.glRhi = std::move(*glRhi);
            result.rhi = std::move(result.glRhi.rhi);
        }
        break;
    }
#endif
#if QT_CONFIG(vulkan)
    case QRhi::Vulkan: {
        // The device extension CUDATextureConverter's Vulkan path needs enabled on the QRhi's
        // VkDevice in order to export a plane's VkImage memory as a POSIX fd for import into
        // CUDA.
        q23::expected<OffscreenVulkanRhi, QString> vulkanRhi =
                createOffscreenVulkanRhi({ "VK_KHR_external_memory_fd" });
        if (vulkanRhi) {
            result.vulkanRhi = std::move(*vulkanRhi);
            result.rhi = std::move(result.vulkanRhi.rhi);
        }
        break;
    }
#endif
    default:
        Q_UNREACHABLE();
    }
    return result;
}

} // namespace

class tst_QCudaTextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void create_succeedsAndKeepsRhi_whenCudaAndDriverApiAreAvailable_data();
    void create_succeedsAndKeepsRhi_whenCudaAndDriverApiAreAvailable();

    void create_disablesVulkanInterop_whenDeviceExtensionIsNotEnabled();

    void createTextureHandles_roundTripsPixelData_forHwFrame_data();
    void createTextureHandles_roundTripsPixelData_forHwFrame();

    void map_returnsRealPixelData_forHwFrame_data();
    void map_returnsRealPixelData_forHwFrame();

    void createTextureHandles_reusesPlaneResources_onSecondCall();
};

void tst_QCudaTextureConverter::create_succeedsAndKeepsRhi_whenCudaAndDriverApiAreAvailable_data()
{
    QTest::addColumn<QRhi::Implementation>("backend");

#if QT_CONFIG(opengl)
    QTest::newRow("OpenGL") << QRhi::OpenGLES2;
#endif
#if QT_CONFIG(vulkan)
    QTest::newRow("Vulkan") << QRhi::Vulkan;
#endif
}

void tst_QCudaTextureConverter::create_succeedsAndKeepsRhi_whenCudaAndDriverApiAreAvailable()
{
    QFETCH(QRhi::Implementation, backend);

    OffscreenRhi offscreen = createOffscreenRhi(backend);
    QRhi *const rhi = offscreen.rhi.get();
    if (!rhi)
        QSKIP("Could not create a QRhi backend for this test on this system");

    std::shared_ptr converter = CUDATextureConverterFactory::create(rhi);
    if (!converter)
        QSKIP("CUDA/GPU interop is not available on this system for this backend (missing "
              "driver, ffnvcodec headers, or - for Vulkan - VK_KHR_external_memory_fd)");

    QCOMPARE(converter->rhi, rhi);
}

// An app that does not request VK_KHR_external_memory_fd on the QRhi's VkDevice (e.g. Qt Quick
// does not, by default) must not crash in CUDATextureConverter: vkGetMemoryFdKHR resolved via
// vkGetInstanceProcAddr is a non-null loader trampoline even when the extension is not enabled,
// and calling it jumps to null. The converter has to detect that and disable itself.
void tst_QCudaTextureConverter::create_disablesVulkanInterop_whenDeviceExtensionIsNotEnabled()
{
#if QT_CONFIG(vulkan)
    // Control: with the extension enabled, the interop must be available. Otherwise (e.g. no CUDA
    // driver) the check below would pass without testing anything.
    {
        OffscreenRhi withExtension = createOffscreenRhi(QRhi::Vulkan);
        if (!withExtension.rhi)
            QSKIP("Could not create a Vulkan QRhi on this system");

        std::shared_ptr control = CUDATextureConverterFactory::create(withExtension.rhi.get());
        if (!control)
            QSKIP("CUDA-Vulkan interop is not available on this system");
    }

    // No device extensions requested, like a QRhi created by Qt Quick.
    q23::expected<OffscreenVulkanRhi, QString> vulkanRhi = createOffscreenVulkanRhi();
    if (!vulkanRhi)
        QSKIP("Could not create a Vulkan QRhi without device extensions on this system");

    std::shared_ptr converter = CUDATextureConverterFactory::create(vulkanRhi->rhi.get());
    QVERIFY2(!converter,
             "The converter must disable itself when VK_KHR_external_memory_fd is not enabled on "
             "the QRhi's VkDevice, otherwise its first use calls a null vkGetMemoryFdKHR");
#else
    QSKIP("Vulkan is not available in this build");
#endif
}

void tst_QCudaTextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame_data()
{
    QTest::addColumn<QRhi::Implementation>("backend");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    const std::pair<const char *, QRhi::Implementation> backends[] = {
#if QT_CONFIG(opengl)
        { "OpenGL", QRhi::OpenGLES2 },
#endif
#if QT_CONFIG(vulkan)
        { "Vulkan", QRhi::Vulkan },
#endif
    };
    // NV21/P016 share cudaFrameGeometry()'s NV12/P010 case, respectively.
    const std::pair<const char *, QVideoFrameFormat::PixelFormat> pixelFormats[] = {
        { "NV12", QVideoFrameFormat::Format_NV12 },
        { "NV21", QVideoFrameFormat::Format_NV21 },
        { "P010", QVideoFrameFormat::Format_P010 },
        { "P016", QVideoFrameFormat::Format_P016 },
    };

    for (const auto &backend : backends)
        for (const auto &pixelFormat : pixelFormats)
            QTest::addRow("%s_%s", backend.first, pixelFormat.first)
                    << backend.second << pixelFormat.second;
}

// GPU-import coverage for the two semi-planar formats (NV12/NV21, 8-bit; P010/P016, 10-/16-bit)
// a CUDA decode can produce, on both QRhi backends CUDATextureConverter supports.
void tst_QCudaTextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame()
{
    QFETCH(QRhi::Implementation, backend);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    const int sampleSize = cudaBytesPerSample(QFFmpegVideoBuffer::toAVPixelFormat(pixelFormat));
    const std::vector<QRhiTexture::Format> planeFormats{ cudaTextureFormat(sampleSize, 1),
                                                          cudaTextureFormat(sampleSize, 2) };

    // The QRhi must outlive the converter/handles below, so it is declared before them (locals
    // are destroyed in reverse declaration order).
    OffscreenRhi offscreen = createOffscreenRhi(backend);
    QRhi *const rhi = offscreen.rhi.get();
    if (!rhi)
        QSKIP("Could not create a QRhi backend for this test on this system");

    const QSize frameSize(64, 64);
    // Declared before converter: the converter's m_cudaContext points into this frame's hw
    // device context, so the converter (and thus its context guards) must be destroyed first
    // - locals are destroyed in reverse declaration order, so this holds on every return
    // path, not just the one at the end of this function.
    q23::expected<HwFrameContext, QString> frameContext = createCudaFrame(pixelFormat, frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a CUDA hw frame on this system: "_s
                         + frameContext.error()));

    std::shared_ptr converter = CUDATextureConverterFactory::create(rhi);
    if (!converter)
        QSKIP("CUDA/GPU interop is not available on this system for this backend");

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("CUDA texture interop failed on this system; see qWarning above for details");

    QCOMPARE_NE(handles->textureHandle(*rhi, 0), handles->textureHandle(*rhi, 1));

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    for (int plane = 0; plane < int(planeFormats.size()); ++plane) {
        const quint64 handle = handles->textureHandle(*rhi, plane);
        QCOMPARE_NE(handle, quint64(0));

        const QSize planeSize(desc->widthForPlane(frameSize.width(), plane),
                              desc->heightForPlane(frameSize.height(), plane));
        const auto readback = readBackPlane(*rhi, handle, planeSize, planeFormats[plane]);
        if (!readback)
            QFAIL(qPrintable(u"Failed to read back plane %1: "_s.arg(plane) + readback.error()));
        QCOMPARE(*readback,
                 frameContext->planeTexels[plane].repeated(planeSize.width() * planeSize.height()));
    }
}

void tst_QCudaTextureConverter::map_returnsRealPixelData_forHwFrame_data()
{
    QTest::addColumn<QRhi::Implementation>("backend");
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    const std::pair<const char *, QRhi::Implementation> backends[] = {
#if QT_CONFIG(opengl)
        { "OpenGL", QRhi::OpenGLES2 },
#endif
#if QT_CONFIG(vulkan)
        { "Vulkan", QRhi::Vulkan },
#endif
    };
    const std::pair<const char *, QVideoFrameFormat::PixelFormat> pixelFormats[] = {
        { "NV12", QVideoFrameFormat::Format_NV12 },
        { "NV21", QVideoFrameFormat::Format_NV21 },
        { "P010", QVideoFrameFormat::Format_P010 },
        { "P016", QVideoFrameFormat::Format_P016 },
    };

    for (const auto &backend : backends)
        for (const auto &pixelFormat : pixelFormats)
            QTest::addRow("%s_%s", backend.first, pixelFormat.first)
                    << backend.second << pixelFormat.second;
}

// End-to-end coverage for the CPU (QVideoFrame::map()) fallback path on a real CUDA hw frame,
// constructed and wrapped exactly as QFFmpeg::VideoRenderer does: map() always goes through
// FFmpeg's own av_hwframe_transfer_data() and never touches the QRhi. Parametrized over both
// backends anyway (mirroring the GPU-import test above), so each backend's absence is reported
// as a QSKIP here too, rather than this test silently running only the rows the GPU-import test
// happens to cover.
void tst_QCudaTextureConverter::map_returnsRealPixelData_forHwFrame()
{
    QFETCH(QRhi::Implementation, backend);
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    OffscreenRhi offscreen = createOffscreenRhi(backend);
    if (!offscreen.rhi)
        QSKIP("Could not create a QRhi backend for this test on this system");

    const QSize frameSize(64, 64);
    q23::expected<HwFrameContext, QString> frameContext = createCudaFrame(pixelFormat, frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a CUDA hw frame on this system: "_s
                         + frameContext.error()));

    const q23::expected<void, QString> result =
            verifyMappedHwFrame(std::move(frameContext->hwFrame), frameContext->planeTexels);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error()));
}

void tst_QCudaTextureConverter::createTextureHandles_reusesPlaneResources_onSecondCall()
{
#if !QT_CONFIG(opengl)
    QSKIP("This test requires OpenGL support");
#else
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    // Both frames come from the same hw device/frames context (and thus the same
    // CUcontext), as consecutive decoded frames of a real session would. Declared before
    // converter so it outlives it - see the comment in the previous test.
    const QSize frameSize(64, 64);
    q23::expected<HwFrameContext, QString> frameContext = createCudaNv12(frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a CUDA hw frame on this system: "_s
                         + frameContext.error()));

    std::shared_ptr converter = CUDATextureConverterFactory::create(rhi.get());
    if (!converter)
        QSKIP("CUDA driver API is not available on this system");

    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!firstHandles)
        QSKIP("CUDA texture interop failed on this system; see qWarning above for details");

    const quint64 firstLumaTexture = firstHandles->textureHandle(*rhi, 0);

    AVFrameUPtr secondFrame = createSecondCudaFrame(
            *frameContext, { QByteArray(1, char(0x50)), makeBytes({ 0x60, 0x70 }) });
    QVERIFY(secondFrame);

    QVideoFrameTexturesHandlesUPtr secondHandles = converter->createTextureHandles(
            secondFrame.get(), std::move(firstHandles));
    QVERIFY(secondHandles);

    // Same frame size and format: the converter is expected to reuse its cached GL/CUDA
    // plane resources (see ensurePlaneResources) rather than reallocate them every frame.
    QCOMPARE(secondHandles->textureHandle(*rhi, 0), firstLumaTexture);
#endif
}

QTEST_MAIN(tst_QCudaTextureConverter)

#include "tst_qcudatextureconverter.moc"
