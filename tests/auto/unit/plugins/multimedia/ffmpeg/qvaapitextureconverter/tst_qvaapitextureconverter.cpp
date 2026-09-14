// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "../shared/nv12hwframecontext_p.h"

#include <QtMultimediaTestLib/private/rhi_support_p.h>
#include <QtTest/qtest.h>

#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_vaapi_p.h>

#include <QtMultimedia/private/qmultimedia_gl_support_p.h>

#include <QtCore/qsize.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qoffscreensurface.h>
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

using namespace QFFmpeg;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

using VaapiFrameContext = Nv12HwFrameContext;

namespace {

// Returns nullopt if no usable VAAPI device is available on this machine.
std::optional<VaapiFrameContext> createVaapiFrameContext(QSize size, quint8 lumaValue,
                                                         quint8 chromaUValue, quint8 chromaVValue)
{
    // Request surfaces suitable for zero-copy export, matching what a real decode+display
    // pipeline asks for (VAAPITextureConverter exports via vaExportSurfaceHandle).
    VASurfaceAttrib usageHintAttrib{
        .type = VASurfaceAttribUsageHint,
        .flags = VA_SURFACE_ATTRIB_SETTABLE,
        .value = VAGenericValue{
            .type = VAGenericValueTypeInteger,
            .value = {
                .i = VA_SURFACE_ATTRIB_USAGE_HINT_EXPORT,
            },
        },
    };

    return createNv12HwFrameContext(AV_HWDEVICE_TYPE_VAAPI, AV_PIX_FMT_VAAPI, size, lumaValue,
                                    chromaUValue, chromaVValue, [&](AVHWFramesContext &ctx) {
        // VAAPI surfaces are allocated up front; a small fixed pool is enough for this test.
        ctx.initial_pool_size = 4;

        auto *vaapiFramesCtx = reinterpret_cast<AVVAAPIFramesContext *>(ctx.hwctx);
        vaapiFramesCtx->attributes = &usageHintAttrib;
        vaapiFramesCtx->nb_attributes = 1;
    });
}

} // namespace

class tst_QVaapiTextureConverter : public QObject
{
    Q_OBJECT

private slots:
    void create_succeedsAndKeepsRhi_whenVaapiAndEglAreAvailable();
    void createTextureHandles_returnsOnePlanePerNv12Component_forHwFrame();
    void createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed();
    void createTextureHandles_acceptsOldHandles_onSecondCall();
};

void tst_QVaapiTextureConverter::create_succeedsAndKeepsRhi_whenVaapiAndEglAreAvailable()
{
    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi = createOffscreenGlRhi(fallbackSurface);
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    QVERIFY(converter);
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system; try running with "
              "QT_XCB_GL_INTEGRATION=xcb_egl");

    QCOMPARE(converter->rhi, rhi.get());
}

void tst_QVaapiTextureConverter::createTextureHandles_returnsOnePlanePerNv12Component_forHwFrame()
{
    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi = createOffscreenGlRhi(fallbackSurface);
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system");

    const QSize frameSize(64, 64);
    std::optional<VaapiFrameContext> frameContext =
            createVaapiFrameContext(frameSize, 0x40, 0x60, 0x80);
    if (!frameContext)
        QSKIP("Could not create a VAAPI hw frame on this system (no usable VAAPI driver)");

    QVideoFrameTexturesHandlesUPtr handles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!handles)
        QSKIP("VAAPI texture import failed on this system; see qWarning above for details");

    // NV12 has two planes: luma (R8) and interleaved chroma (RG8).
    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    const quint64 chromaHandle = handles->textureHandle(*rhi, 1);
    QVERIFY(lumaHandle != 0);
    QVERIFY(chromaHandle != 0);
    QVERIFY(lumaHandle != chromaHandle);

    // A successful readback proves both planes are real, sampleable textures backed by the
    // imported dma-buf planes, and round-trips the pixel values the frame was filled with.
    rhi->makeThreadLocalNativeContextCurrent();

    const auto lumaReadback = readBackPlane(*rhi, lumaHandle, frameSize, QRhiTexture::R8);
    if (!lumaReadback)
        QFAIL(qPrintable(u"Failed to read back luma plane: "_s + lumaReadback.error()));
    QCOMPARE(*lumaReadback, QByteArray(frameSize.width() * frameSize.height(), char(0x40)));

    const QSize chromaSize(frameSize.width() / 2, frameSize.height() / 2);
    const auto chromaReadback = readBackPlane(*rhi, chromaHandle, chromaSize, QRhiTexture::RG8);
    if (!chromaReadback)
        QFAIL(qPrintable(u"Failed to read back chroma plane: "_s + chromaReadback.error()));
    QByteArray expectedChroma;
    for (int i = 0; i < chromaSize.width() * chromaSize.height(); ++i)
        expectedChroma += QByteArrayView("\x60\x80");
    QCOMPARE(*chromaReadback, expectedChroma);
}

void tst_QVaapiTextureConverter::
        createTextureHandles_outlivesSourceFrame_afterFrameContextIsDestroyed()
{
    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi = createOffscreenGlRhi(fallbackSurface);
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system");

    const QSize frameSize(64, 64);
    std::optional<VaapiFrameContext> frameContext =
            createVaapiFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContext)
        QSKIP("Could not create a VAAPI hw frame on this system (no usable VAAPI driver)");

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
    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi = createOffscreenGlRhi(fallbackSurface);
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    std::shared_ptr<VAAPITextureConverter> converter = VAAPITextureConverter::create(rhi.get());
    if (!converter->rhi)
        QSKIP("VAAPI/EGL DMA-BUF import is not available on this system");

    const QSize frameSize(64, 64);
    std::optional<VaapiFrameContext> frameContext =
            createVaapiFrameContext(frameSize, 0x10, 0x20, 0x30);
    if (!frameContext)
        QSKIP("Could not create a VAAPI hw frame on this system (no usable VAAPI driver)");

    QVideoFrameTexturesHandlesUPtr firstHandles =
            converter->createTextureHandles(frameContext->hwFrame.get(), nullptr);
    if (!firstHandles)
        QSKIP("VAAPI texture import failed on this system; see qWarning above for details");

    std::optional<VaapiFrameContext> secondFrameContext =
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
