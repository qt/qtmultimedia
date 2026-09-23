// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/hwframecontext_p.h"

#include <QtMultimediaTestLib/private/rhi_support_p.h>
#include <QtTest/qtest.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_vaapi_p.h>

#include <QtMultimedia/private/qmultimedia_gl_support_p.h>

#include <QtCore/qsize.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/rhi/qrhi.h>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vaapi.h>
}

#include <va/va.h>

#include <memory>
#include <optional>
#include <vector>

using namespace QFFmpeg;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

using VaapiFrameContext = HwFrameContext;

namespace {

// Request surfaces suitable for zero-copy export, matching what a real decode+display pipeline
// asks for (VAAPITextureConverter exports via vaExportSurfaceHandle).
void configureExportableFramesContext(AVHWFramesContext &ctx)
{
    static VASurfaceAttrib usageHintAttrib{
        .type = VASurfaceAttribUsageHint,
        .flags = VA_SURFACE_ATTRIB_SETTABLE,
        .value = VAGenericValue{
            .type = VAGenericValueTypeInteger,
            .value = {
                .i = VA_SURFACE_ATTRIB_USAGE_HINT_EXPORT,
            },
        },
    };

    // VAAPI surfaces are allocated up front; a small fixed pool is enough for these tests.
    ctx.initial_pool_size = 4;

    auto *vaapiFramesCtx = reinterpret_cast<AVVAAPIFramesContext *>(ctx.hwctx);
    vaapiFramesCtx->attributes = &usageHintAttrib;
    vaapiFramesCtx->nb_attributes = 1;
}

// Returns an error message if no usable VAAPI device is available on this machine.
q23::expected<VaapiFrameContext, QString>
createVaapiFrameContext(QSize size, quint8 lumaValue, quint8 chromaUValue, quint8 chromaVValue)
{
    return createNv12HwFrameContext(AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI, size, lumaValue,
                                    chromaUValue, chromaVValue, configureExportableFramesContext);
}

// Same as above, but with a 10-bit P010 surface.
q23::expected<VaapiFrameContext, QString> createVaapiP010FrameContext(QSize size,
                                                                      quint16 lumaValue,
                                                                      quint16 chromaUValue,
                                                                      quint16 chromaVValue)
{
    return createP010HwFrameContext(AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI, size, lumaValue,
                                    chromaUValue, chromaVValue, configureExportableFramesContext);
}

// NV12: full-size luma plane, half-size interleaved-chroma plane.
q23::expected<VaapiFrameContext, QString> createVaapiNv12(QSize size)
{
    return createVaapiFrameContext(size, 0x40, 0x60, 0x80);
}

// P010: same plane layout as NV12, but 16-bit samples. Chosen so that a byte-swap or a missing
// "<< 6" shift would produce a different (and thus detectable) byte pattern.
q23::expected<VaapiFrameContext, QString> createVaapiP010(QSize size)
{
    return createVaapiP010FrameContext(size, 0x141, 0x161, 0x181);
}

// Same as above, but with a single-plane BGRA8888 surface.
q23::expected<VaapiFrameContext, QString> createVaapiBgraFrameContext(QSize size,
                                                                      QByteArray texel)
{
    return createSinglePlaneHwFrameContext(AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI, size,
                                           std::move(texel), configureExportableFramesContext);
}

// BGRA8888: single packed plane. Unlike VideoToolbox's per-plane Metal cache import (which never
// sees a packed CVPixelBuffer as importable, see createVideoToolboxBgraFrameContext),
// VAAPITextureConverter derives its plane count generically from the pixel format's DRM fourcc
// list, so a single-plane surface is a perfectly regular case for its GPU import path too.
q23::expected<VaapiFrameContext, QString> createVaapiBgra(QSize size)
{
    return createVaapiBgraFrameContext(size, makeBytes({ 0x11, 0x22, 0x33, 0x44 }));
}

q23::expected<VaapiFrameContext, QString>
createVaapiFrame(QVideoFrameFormat::PixelFormat pixelFormat, QSize size)
{
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
        return createVaapiNv12(size);
    case QVideoFrameFormat::Format_P010:
        return createVaapiP010(size);
    case QVideoFrameFormat::Format_BGRA8888:
        return createVaapiBgra(size);
    default:
        Q_UNREACHABLE_RETURN(q23::unexpected(u"unsupported pixel format"_s));
    }
}

} // namespace

class tst_QVaapiTextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void create_succeedsAndKeepsRhi_whenVaapiAndEglAreAvailable();

    void createTextureHandles_roundTripsPixelData_forHwFrame_data();
    void createTextureHandles_roundTripsPixelData_forHwFrame();

    void map_returnsRealPixelData_forHwFrame_data();
    void map_returnsRealPixelData_forHwFrame();

    void createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed();
    void createTextureHandles_acceptsOldHandles_onSecondCall();
};

void tst_QVaapiTextureConverter::create_succeedsAndKeepsRhi_whenVaapiAndEglAreAvailable()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    QVERIFY(converter);
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system; try running with "
              "QT_XCB_GL_INTEGRATION=xcb_egl");

    QCOMPARE(converter->rhi, rhi.get());
}

void tst_QVaapiTextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
    QTest::newRow("BGRA") << QVideoFrameFormat::Format_BGRA8888;
}

// GPU-import coverage for the 8-bit (NV12) and 10-bit (P010) bi-planar surfaces, and the
// single-plane BGRA8888 surface, a VAAPI decoder can produce. Unlike VideoToolbox (see
// createVideoToolboxBgraFrameContext), VAAPITextureConverter's import path doesn't special-case
// plane count, so BGRA is a regular case here rather than map()-only coverage.
void tst_QVaapiTextureConverter::createTextureHandles_roundTripsPixelData_forHwFrame()
{
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);
    std::vector<QRhiTexture::Format> planeFormats;
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
        planeFormats = { QRhiTexture::R8, QRhiTexture::RG8 };
        break;
    case QVideoFrameFormat::Format_P010:
        planeFormats = { QRhiTexture::R16, QRhiTexture::RG16 };
        break;
    case QVideoFrameFormat::Format_BGRA8888:
        planeFormats = { QRhiTexture::RGBA8 };
        break;
    default:
        Q_UNREACHABLE();
    }

    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system");

    const QSize frameSize(64, 64);
    q23::expected<VaapiFrameContext, QString> frameContext =
            createVaapiFrame(pixelFormat, frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a VAAPI hw frame on this system: "_s
                         + frameContext.error()));

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("VAAPI texture import failed on this system; see qWarning above for details");

    const int nPlanes = int(planeFormats.size());
    if (nPlanes > 1)
        QVERIFY(handles->textureHandle(*rhi, 0) != handles->textureHandle(*rhi, 1));

    // A successful readback proves every plane is a real, sampleable texture backed by the
    // imported dma-buf plane(s), and round-trips the pixel values the frame was filled with.
    rhi->makeThreadLocalNativeContextCurrent();

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    for (int plane = 0; plane < nPlanes; ++plane) {
        const quint64 handle = handles->textureHandle(*rhi, plane);
        QVERIFY(handle != 0);

        const QSize planeSize(desc->widthForPlane(frameSize.width(), plane),
                              desc->heightForPlane(frameSize.height(), plane));
        const auto readback = readBackPlane(*rhi, handle, planeSize, planeFormats[plane]);
        if (!readback)
            QFAIL(qPrintable(u"Failed to read back plane %1: "_s.arg(plane) + readback.error()));
        QCOMPARE(*readback,
                 frameContext->planeTexels[plane].repeated(planeSize.width() * planeSize.height()));
    }
}

void tst_QVaapiTextureConverter::map_returnsRealPixelData_forHwFrame_data()
{
    QTest::addColumn<QVideoFrameFormat::PixelFormat>("pixelFormat");

    QTest::newRow("NV12") << QVideoFrameFormat::Format_NV12;
    QTest::newRow("P010") << QVideoFrameFormat::Format_P010;
    QTest::newRow("BGRA") << QVideoFrameFormat::Format_BGRA8888;
}

// End-to-end coverage for the CPU (QVideoFrame::map()) fallback path on a real VAAPI hw frame,
// constructed and wrapped exactly as QFFmpeg::VideoRenderer does: map() always goes through
// FFmpeg's own av_hwframe_transfer_data(), independent of whether GPU texture import (tested
// above) is even available on this system.
void tst_QVaapiTextureConverter::map_returnsRealPixelData_forHwFrame()
{
    QFETCH(QVideoFrameFormat::PixelFormat, pixelFormat);

    const QSize frameSize(64, 64);
    q23::expected<VaapiFrameContext, QString> frameContext =
            createVaapiFrame(pixelFormat, frameSize);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a VAAPI hw frame on this system: "_s
                         + frameContext.error()));

    const q23::expected<void, QString> result =
            verifyMappedHwFrame(std::move(frameContext->hwFrame), frameContext->planeTexels);
    QVERIFY2(result.has_value(), result ? "" : qPrintable(result.error()));
}

void tst_QVaapiTextureConverter::
        createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system");

    const QSize frameSize(64, 64);
    q23::expected<VaapiFrameContext, QString> frameContextResult =
            createVaapiFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContextResult)
        QSKIP(qPrintable(u"Could not create a VAAPI hw frame on this system: "_s
                         + frameContextResult.error()));
    std::optional<VaapiFrameContext> frameContext = std::move(*frameContextResult);

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("VAAPI texture import failed on this system; see qWarning above for details");

    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    QVERIFY(lumaHandle != 0);

    // The imported GL texture remains readable after the source hw-frames context is destroyed
    // (measured below); it does not depend on frameContext staying alive.
    frameContext.reset();

    rhi->makeThreadLocalNativeContextCurrent();
    auto readbackResult = readBackPlane(*rhi, lumaHandle, frameSize, QRhiTexture::R8);
    if (!readbackResult)
        QFAIL(qPrintable(u"Failed to read back luma plane: "_s + readbackResult.error()));
    const QByteArray lumaReadback = *readbackResult;
    QCOMPARE(lumaReadback, QByteArray(frameSize.width() * frameSize.height(), char(0x10)));
}

void tst_QVaapiTextureConverter::createTextureHandles_acceptsOldHandles_onSecondCall()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system");

    const QSize frameSize(64, 64);
    q23::expected<VaapiFrameContext, QString> frameContext =
            createVaapiFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContext)
        QSKIP(qPrintable(u"Could not create a VAAPI hw frame on this system: "_s
                         + frameContext.error()));

    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!firstHandles)
        QSKIP("VAAPI texture import failed on this system; see qWarning above for details");

    q23::expected<VaapiFrameContext, QString> secondFrameContext =
            createVaapiFrameContext(frameSize, 0x50, 0x60, 0x70);
    QVERIFY(secondFrameContext);

    QVideoFrameTexturesHandlesUPtr secondHandles = converter->createTextureHandles(
            secondFrameContext->hwFrame.get(), std::move(firstHandles));
    QVERIFY(secondHandles);
    QVERIFY(GLuint(secondHandles->textureHandle(*rhi, 0)) != 0);
}

int main(int argc, char *argv[])
{
    // Force EGL-based GL integration on xcb so that QGuiApplication exposes the
    // "egldisplay" native resource that DmaBufEglContext relies on (the default xcb_glx
    // integration does not provide one).
    qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");

    QGuiApplication app(argc, argv);
    tst_QVaapiTextureConverter tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qvaapitextureconverter.moc"
