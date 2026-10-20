// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/rhi_support_p.h>
#include <QtMultimediaTestLib/private/udmabuftestutils_p.h>
#include <QtTest/qtest.h>
#include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qmultimedia_gl_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qspan.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <array>
#include <optional>
#include <vector>

using namespace QtMultimediaPrivate;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

namespace {

constexpr int nv12Width = 4;
constexpr int nv12Height = 4;
constexpr int nv12YPitch = nv12Width;
constexpr int nv12YSize = nv12YPitch * nv12Height;
constexpr int nv12UVPitch = nv12Width;
constexpr int nv12UVSize = nv12UVPitch * nv12Height / 2;

struct Nv12UdmabufPlanes
{
    QUniqueFileDescriptorHandle memfd;
    QUniqueFileDescriptorHandle dmabufFd;
    std::array<DmaBufPlane, 2> planes;
};

QByteArray nv12TestPixels()
{
    QByteArray pixels(nv12YSize + nv12UVSize, '\0');
    for (int i = 0; i < nv12YSize; ++i)
        pixels[i] = char(16 + i);
    for (int i = 0; i < nv12UVSize; ++i)
        pixels[nv12YSize + i] = char(64 + i);
    return pixels;
}

std::optional<Nv12UdmabufPlanes> createNv12UdmabufPlanes(QSpan<const DRMFormat> formats)
{
    const QByteArray pixels = nv12TestPixels();
    const quint64 bufferSize = udmabufPageAlignedSize(pixels.size());

    QUniqueFileDescriptorHandle memfd =
            createUdmabufTestMemfd(pixels, bufferSize, "tst_qdmabuftextureimporter");
    if (!memfd)
        return std::nullopt;

    QUniqueFileDescriptorHandle dmabufFd = createUdmabufFd(memfd, bufferSize);
    if (!dmabufFd)
        return std::nullopt;

    Nv12UdmabufPlanes result;
    result.memfd = std::move(memfd);
    result.dmabufFd = std::move(dmabufFd);
    result.planes[0] = { result.dmabufFd.get(), 0, quint32(nv12YPitch), formats[0] };
    result.planes[1] = { result.dmabufFd.get(), quint32(nv12YSize), quint32(nv12UVPitch),
                         formats[1] };
    return result;
}

} // namespace

class tst_QDmaBufTextureImporter : public QObject
{
    Q_OBJECT

private slots:
    void importDmaBufTextures_roundTripsPixelData_whenGivenUdmabufBackedPlane();
    void importDmaBufTextures_exposesBothHandles_whenGivenUdmabufBackedNV12Planes();
    void importDmaBufTextures_exposesSingleHandle_whenGivenUdmabufBackedNV12PlanesSingleImage();
    void importDmaBufTextures_failsRecoverably_whenFormatHasNoSingleImageFourcc();
    void createTexturesFromHandles_exposesSingleTexture_whenGivenSingleImageHandles();
};

void tst_QDmaBufTextureImporter::
        importDmaBufTextures_roundTripsPixelData_whenGivenUdmabufBackedPlane()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    constexpr int width = 4;
    constexpr int height = 4;
    constexpr int bytesPerPixel = 4;
    constexpr int stride = width * bytesPerPixel;

    // A solid-color RGBA8888 buffer; byte order in memory is R, G, B, A.
    QByteArray pixels(stride * height, '\0');
    for (int i = 0; i < width * height; ++i) {
        char *p = pixels.data() + i * bytesPerPixel;
        p[0] = char(10);
        p[1] = char(20);
        p[2] = char(30);
        p[3] = char(255);
    }

    const quint64 bufferSize = udmabufPageAlignedSize(pixels.size());

    QUniqueFileDescriptorHandle memfd =
            createUdmabufTestMemfd(pixels, bufferSize, "tst_qdmabuftextureimporter");
    if (!memfd)
        QSKIP("Could not create a memfd-backed test buffer");

    QUniqueFileDescriptorHandle dmabufFd = createUdmabufFd(memfd, bufferSize);
    if (!dmabufFd)
        QSKIP("Could not wrap the test buffer as a udmabuf dma-buf fd");

    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    DmaBufEglContext eglContext(rhi.get());
    if (!eglContext.isValid())
        QSKIP("EGL DMABUF import is not available (no EGL display or missing GL extensions); "
              "try running with QT_XCB_GL_INTEGRATION=xcb_egl");

    QSpan<const DRMFormat> drmFormats =
            dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_RGBA8888);
    QVERIFY(!drmFormats.empty());

    DmaBufPlane plane;
    plane.fd = dmabufFd.get();
    plane.offset = 0;
    plane.pitch = stride;
    plane.drmFormat = drmFormats[0];

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles =
            importDmaBufTextures(*rhi, eglContext, QSpan(&plane, 1),
                                 QVideoFrameFormat::Format_RGBA8888, QSize(width, height));

    if (!handles)
        QSKIP("Importing a CPU-backed dma-buf as a GL texture failed; the driver/EGL "
              "implementation on this system likely doesn't support it (see the qWarning "
              "above for the specific EGL/GL error)");

    rhi->makeThreadLocalNativeContextCurrent();
    const quint64 texture = (*handles)->textureHandle(*rhi, 0);
    QVERIFY(texture != 0);

    auto readbackResult =
            readBackPlane(*rhi, texture, QSize(width, height), QRhiTexture::RGBA8);
    if (!readbackResult)
        QFAIL(qPrintable(u"Failed to read back RGBA plane: "_s + readbackResult.error()));

    QCOMPARE(*readbackResult, pixels);
}

void tst_QDmaBufTextureImporter::
        importDmaBufTextures_exposesBothHandles_whenGivenUdmabufBackedNV12Planes()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    QSpan<const DRMFormat> drmFormats = dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_NV12);
    QVERIFY(drmFormats.size() == 2);

    std::optional<Nv12UdmabufPlanes> nv12 = createNv12UdmabufPlanes(drmFormats);
    if (!nv12)
        QSKIP("Could not create udmabuf-backed NV12 test planes");

    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    DmaBufEglContext eglContext(rhi.get());
    if (!eglContext.isValid())
        QSKIP("EGL DMABUF import is not available (no EGL display or missing GL extensions); "
              "try running with QT_XCB_GL_INTEGRATION=xcb_egl");

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles = importDmaBufTextures(
            *rhi, eglContext, QSpan<const DmaBufPlane>(nv12->planes.data(), nv12->planes.size()),
            QVideoFrameFormat::Format_NV12, QSize(nv12Width, nv12Height));

    if (!handles)
        QSKIP("Importing a CPU-backed dma-buf as GL textures failed; the driver/EGL "
              "implementation on this system likely doesn't support it (see the qWarning "
              "above for the specific EGL/GL error)");

    rhi->makeThreadLocalNativeContextCurrent();
    QVERIFY((*handles)->textureHandle(*rhi, 0) != 0);
    QVERIFY((*handles)->textureHandle(*rhi, 1) != 0);
}

void tst_QDmaBufTextureImporter::
        importDmaBufTextures_exposesSingleHandle_whenGivenUdmabufBackedNV12PlanesSingleImage()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    const bool previousForce = QVideoTextureHelper::forceGlTextureExternalOesIsSet();
    auto restoreForce = qScopeGuard([&] {
        QVideoTextureHelper::setForceGlTextureExternalOesIsSet(previousForce);
    });
    QVideoTextureHelper::setForceGlTextureExternalOesIsSet(false);

    static constexpr std::array<DRMFormat, 2> combinedFormats = { DRMFormat::NV12,
                                                                  DRMFormat::NV12 };

    std::optional<Nv12UdmabufPlanes> nv12 = createNv12UdmabufPlanes(
            QSpan<const DRMFormat>(combinedFormats.data(), combinedFormats.size()));
    if (!nv12)
        QSKIP("Could not create udmabuf-backed NV12 test planes");

    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    DmaBufEglContext eglContext(rhi.get());
    if (!eglContext.isValid())
        QSKIP("EGL DMABUF import is not available (no EGL display or missing GL extensions); "
              "try running with QT_XCB_GL_INTEGRATION=xcb_egl");

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles = importDmaBufTextures(
            *rhi, eglContext, QSpan<const DmaBufPlane>(nv12->planes.data(), nv12->planes.size()),
            QVideoFrameFormat::Format_NV12, QSize(nv12Width, nv12Height), {},
            DmaBufImportMode::SingleImage);

    if (!handles)
        QSKIP("Importing a CPU-backed dma-buf as a single GL texture failed; the driver/EGL "
              "implementation on this system likely doesn't support multi-plane EGL images "
              "(see the qWarning above for the specific EGL/GL error)");

    rhi->makeThreadLocalNativeContextCurrent();
    QVERIFY((*handles)->textureHandle(*rhi, 0) != 0);
    QVERIFY((*handles)->texturePixelFormat().has_value());
    QCOMPARE_EQ(*(*handles)->texturePixelFormat(), QVideoFrameFormat::Format_RGBA8888);
}

void tst_QDmaBufTextureImporter::
        importDmaBufTextures_failsRecoverably_whenFormatHasNoSingleImageFourcc()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    DmaBufEglContext eglContext(rhi.get());
    if (!eglContext.isValid())
        QSKIP("EGL DMABUF import is not available (no EGL display or missing GL extensions); "
              "try running with QT_XCB_GL_INTEGRATION=xcb_egl");

    // The format is rejected before any plane is imported, so the fd is never used.
    const DmaBufPlane plane{ -1, 0, 0, DRMFormat::ARGB8888 };

    q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity> handles =
            importDmaBufTextures(*rhi, eglContext, QSpan<const DmaBufPlane>(&plane, 1),
                                 QVideoFrameFormat::Format_RGBA8888, QSize(nv12Width, nv12Height),
                                 {}, DmaBufImportMode::SingleImage);

    QVERIFY(!handles);
    QCOMPARE_EQ(handles.error(), FailureSeverity::recoverable);
}

void tst_QDmaBufTextureImporter::
        createTexturesFromHandles_exposesSingleTexture_whenGivenSingleImageHandles()
{
    q23::expected<OffscreenGlRhi, QString> offscreenRhi = createOffscreenGlRhi();
    if (!offscreenRhi)
        QSKIP(qPrintable(u"Could not create an OpenGL ES2 QRhi backend on this system: "_s
                         + offscreenRhi.error()));
    auto &[fallbackSurface, rhi] = *offscreenRhi;

    rhi->makeThreadLocalNativeContextCurrent();
    QOpenGLContext *glContext = QOpenGLContext::currentContext();
    if (!glContext)
        QSKIP("No current OpenGL context");

    QOpenGLFunctions gl(glContext);
    for (const QVideoFrameFormat::PixelFormat texturePixelFormat :
         { QVideoFrameFormat::Format_RGBA8888, QVideoFrameFormat::Format_SamplerExternalOES }) {
        GLuint rawTexture = 0;
        gl.glGenTextures(1, &rawTexture);
        gl.glBindTexture(GL_TEXTURE_2D, rawTexture);
        gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nv12Width, nv12Height, 0, GL_RGBA,
                        GL_UNSIGNED_BYTE, nullptr);
        gl.glBindTexture(GL_TEXTURE_2D, 0);

        std::vector<GlTextureHandle> handles;
        handles.emplace_back(rawTexture, GlTextureDeleter(glContext));

        QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
                std::make_unique<DmaBufTextureHandles>(*rhi, std::move(handles),
                                                       texturePixelFormat),
                *rhi, QVideoFrameFormat::Format_NV12, QSize(nv12Width, nv12Height));

        QVERIFY(textures);
        QVERIFY(textures->texture(0));
        QVERIFY(!textures->texture(1));
        QCOMPARE_EQ(textures->texture(0)->flags().testFlag(QRhiTexture::ExternalOES),
                    texturePixelFormat == QVideoFrameFormat::Format_SamplerExternalOES);
    }
}

int main(int argc, char *argv[])
{
    // Force EGL-based GL integration on xcb so that QGuiApplication exposes the
    // "egldisplay" native resource that DmaBufEglContext relies on (the default xcb_glx
    // integration does not provide one).
    qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");

    QGuiApplication app(argc, argv);
    tst_QDmaBufTextureImporter tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qdmabuftextureimporter.moc"
