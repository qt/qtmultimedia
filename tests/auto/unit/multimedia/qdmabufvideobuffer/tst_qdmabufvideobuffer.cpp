// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimediaTestLib/private/udmabuftestutils_p.h>
#include <QtTest/qtest.h>
#include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#include <QtMultimedia/private/qdmabufvideobuffer_p.h>
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qspan.h>
#include <QtCore/private/quniquehandle_types_p.h>

using namespace QtMultimediaPrivate;

namespace {

constexpr int width = 4;
constexpr int height = 4;
constexpr int bytesPerPixel = 4;
constexpr int stride = width * bytesPerPixel;

// A solid-color RGBA8888 buffer; byte order in memory is R, G, B, A.
QByteArray solidColorPixels()
{
    QByteArray pixels(stride * height, '\0');
    for (int i = 0; i < width * height; ++i) {
        char *p = pixels.data() + i * bytesPerPixel;
        p[0] = char(10);
        p[1] = char(20);
        p[2] = char(30);
        p[3] = char(255);
    }
    return pixels;
}

constexpr int nv12Width = 4;
constexpr int nv12Height = 4;
constexpr int nv12YPitch = nv12Width;
constexpr int nv12YSize = nv12YPitch * nv12Height;
constexpr int nv12UVPitch = nv12Width;
constexpr int nv12UVSize = nv12UVPitch * nv12Height / 2;

struct Nv12UdmabufBacking
{
    QByteArray yPlane;
    QByteArray uvPlane;
    QUniqueFileDescriptorHandle memfd;
    QUniqueFileDescriptorHandle dmabufFd;
};

Nv12UdmabufBacking createNv12UdmabufBacking()
{
    Nv12UdmabufBacking backing;
    backing.yPlane.resize(nv12YSize);
    for (int i = 0; i < nv12YSize; ++i)
        backing.yPlane[i] = char(16 + i);
    backing.uvPlane.resize(nv12UVSize);
    for (int i = 0; i < nv12UVSize; ++i)
        backing.uvPlane[i] = char(64 + i);

    const QByteArray pixels = backing.yPlane + backing.uvPlane;
    const quint64 bufferSize = udmabufPageAlignedSize(pixels.size());

    backing.memfd = createUdmabufTestMemfd(pixels, bufferSize, "tst_qdmabufvideobuffer");
    if (backing.memfd)
        backing.dmabufFd = createUdmabufFd(backing.memfd, bufferSize);
    return backing;
}

} // namespace

class tst_QDmaBufVideoBuffer : public QObject
{
    Q_OBJECT

private slots:
    void map_roundTripsPixelData_whenGivenUdmabufBackedPlane();
    void mapTextures_roundTripsPixelData_whenGivenUdmabufBackedPlane();
    void map_roundTripsPixelData_whenGivenUdmabufBackedNV12Planes();
    void mapTextures_exposesBothHandles_whenGivenUdmabufBackedNV12Planes();
    void mapTextures_exposesSingleHandle_whenGivenUdmabufBackedNV12PlanesSingleImage();
};

void tst_QDmaBufVideoBuffer::map_roundTripsPixelData_whenGivenUdmabufBackedPlane()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    const QByteArray pixels = solidColorPixels();
    const quint64 bufferSize = udmabufPageAlignedSize(pixels.size());

    QUniqueFileDescriptorHandle memfd =
            createUdmabufTestMemfd(pixels, bufferSize, "tst_qdmabufvideobuffer");
    if (!memfd)
        QSKIP("Could not create a memfd-backed test buffer");

    QUniqueFileDescriptorHandle dmabufFd = createUdmabufFd(memfd, bufferSize);
    if (!dmabufFd)
        QSKIP("Could not wrap the test buffer as a udmabuf dma-buf fd");

    QSpan<const DRMFormat> drmFormats =
            dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_RGBA8888);
    QVERIFY(!drmFormats.empty());

    DmaBufPlane plane;
    plane.fd = dmabufFd.get();
    plane.offset = 0;
    plane.pitch = stride;
    plane.drmFormat = drmFormats[0];

    QDmaBufVideoBuffer buffer(QVideoFrameFormat::Format_RGBA8888, QSize(width, height),
                              QSpan(&plane, 1));

    QAbstractVideoBuffer::MapData mapData = buffer.map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 1);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[0]), stride * height), pixels);

    buffer.unmap();
}

void tst_QDmaBufVideoBuffer::mapTextures_roundTripsPixelData_whenGivenUdmabufBackedPlane()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    const QByteArray pixels = solidColorPixels();
    const quint64 bufferSize = udmabufPageAlignedSize(pixels.size());

    QUniqueFileDescriptorHandle memfd =
            createUdmabufTestMemfd(pixels, bufferSize, "tst_qdmabufvideobuffer");
    if (!memfd)
        QSKIP("Could not create a memfd-backed test buffer");

    QUniqueFileDescriptorHandle dmabufFd = createUdmabufFd(memfd, bufferSize);
    if (!dmabufFd)
        QSKIP("Could not wrap the test buffer as a udmabuf dma-buf fd");

    QRhiGles2InitParams glParams;
    glParams.format = QSurfaceFormat::defaultFormat();
    QScopedPointer<QOffscreenSurface> fallbackSurface(QRhiGles2InitParams::newFallbackSurface());
    glParams.fallbackSurface = fallbackSurface.data();

    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &glParams));
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    QSpan<const DRMFormat> drmFormats =
            dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_RGBA8888);
    QVERIFY(!drmFormats.empty());

    DmaBufPlane plane;
    plane.fd = dmabufFd.get();
    plane.offset = 0;
    plane.pitch = stride;
    plane.drmFormat = drmFormats[0];

    QDmaBufVideoBuffer buffer(QVideoFrameFormat::Format_RGBA8888, QSize(width, height),
                              QSpan(&plane, 1));

    QVideoFrameTexturesUPtr textures = buffer.mapTextures(*rhi);
    if (!textures)
        QSKIP("Importing a CPU-backed dma-buf as a GL texture failed; the driver/EGL "
              "implementation on this system likely doesn't support it (see the qWarning "
              "above for the specific EGL/GL error)");

    rhi->makeThreadLocalNativeContextCurrent();
    QVERIFY(QOpenGLContext::currentContext());
    QOpenGLFunctions gl(QOpenGLContext::currentContext());

    QRhiTexture *rhiTexture = textures->texture(0);
    QVERIFY(rhiTexture);
    const auto texture = static_cast<GLuint>(rhiTexture->nativeTexture().object);
    QVERIFY(texture != 0);

    GLuint fbo = 0;
    gl.glGenFramebuffers(1, &fbo);
    gl.glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    QCOMPARE(gl.glCheckFramebufferStatus(GL_FRAMEBUFFER), GLenum(GL_FRAMEBUFFER_COMPLETE));

    QByteArray readback(stride * height, '\0');
    gl.glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, readback.data());

    gl.glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl.glDeleteFramebuffers(1, &fbo);

    QCOMPARE(readback, pixels);
}

void tst_QDmaBufVideoBuffer::map_roundTripsPixelData_whenGivenUdmabufBackedNV12Planes()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    Nv12UdmabufBacking backing = createNv12UdmabufBacking();
    if (!backing.dmabufFd)
        QSKIP("Could not wrap the test buffer as a udmabuf dma-buf fd");

    QSpan<const DRMFormat> drmFormats = dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_NV12);
    QVERIFY(drmFormats.size() == 2);

    DmaBufPlane planes[2] = {
        { backing.dmabufFd.get(), 0, quint32(nv12YPitch), drmFormats[0] },
        { backing.dmabufFd.get(), quint32(nv12YSize), quint32(nv12UVPitch), drmFormats[1] },
    };

    QDmaBufVideoBuffer buffer(QVideoFrameFormat::Format_NV12, QSize(nv12Width, nv12Height),
                              QSpan<const DmaBufPlane>(planes, 2));

    QAbstractVideoBuffer::MapData mapData = buffer.map(QVideoFrame::ReadOnly);
    QCOMPARE(mapData.planeCount, 2);
    QCOMPARE(mapData.bytesPerLine[0], nv12YPitch);
    QCOMPARE(mapData.bytesPerLine[1], nv12UVPitch);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[0]), nv12YSize),
             backing.yPlane);
    QCOMPARE(QByteArray(reinterpret_cast<const char *>(mapData.data[1]), nv12UVSize),
             backing.uvPlane);

    buffer.unmap();
}

void tst_QDmaBufVideoBuffer::mapTextures_exposesBothHandles_whenGivenUdmabufBackedNV12Planes()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    Nv12UdmabufBacking backing = createNv12UdmabufBacking();
    if (!backing.dmabufFd)
        QSKIP("Could not wrap the test buffer as a udmabuf dma-buf fd");

    QRhiGles2InitParams glParams;
    glParams.format = QSurfaceFormat::defaultFormat();
    QScopedPointer<QOffscreenSurface> fallbackSurface(QRhiGles2InitParams::newFallbackSurface());
    glParams.fallbackSurface = fallbackSurface.data();

    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &glParams));
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    QSpan<const DRMFormat> drmFormats = dmaBufFourccFromPixelFormat(QVideoFrameFormat::Format_NV12);
    QVERIFY(drmFormats.size() == 2);

    DmaBufPlane planes[2] = {
        { backing.dmabufFd.get(), 0, quint32(nv12YPitch), drmFormats[0] },
        { backing.dmabufFd.get(), quint32(nv12YSize), quint32(nv12UVPitch), drmFormats[1] },
    };

    QDmaBufVideoBuffer buffer(QVideoFrameFormat::Format_NV12, QSize(nv12Width, nv12Height),
                              QSpan<const DmaBufPlane>(planes, 2));

    QVideoFrameTexturesUPtr textures = buffer.mapTextures(*rhi);
    if (!textures)
        QSKIP("Importing a CPU-backed dma-buf as GL textures failed; the driver/EGL "
              "implementation on this system likely doesn't support it (see the qWarning "
              "above for the specific EGL/GL error)");

    QVERIFY(textures->texture(0));
    QVERIFY(textures->texture(1));
}

void tst_QDmaBufVideoBuffer::
        mapTextures_exposesSingleHandle_whenGivenUdmabufBackedNV12PlanesSingleImage()
{
    if (!canUseUdmabuf())
        QSKIP("/dev/udmabuf is not accessible on this system");

    const bool previousForce = QVideoTextureHelper::forceGlTextureExternalOesIsSet();
    auto restoreForce = qScopeGuard([&] {
        QVideoTextureHelper::setForceGlTextureExternalOesIsSet(previousForce);
    });
    QVideoTextureHelper::setForceGlTextureExternalOesIsSet(false);

    Nv12UdmabufBacking backing = createNv12UdmabufBacking();
    if (!backing.dmabufFd)
        QSKIP("Could not wrap the test buffer as a udmabuf dma-buf fd");

    QRhiGles2InitParams glParams;
    glParams.format = QSurfaceFormat::defaultFormat();
    QScopedPointer<QOffscreenSurface> fallbackSurface(QRhiGles2InitParams::newFallbackSurface());
    glParams.fallbackSurface = fallbackSurface.data();

    QScopedPointer<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &glParams));
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

    DmaBufEglContext eglContext(rhi.get());
    if (!eglContext.isValid())
        QSKIP("EGL DMABUF import is not available (no EGL display or missing GL extensions); "
              "try running with QT_XCB_GL_INTEGRATION=xcb_egl");

    DmaBufPlane planes[2] = {
        { backing.dmabufFd.get(), 0, quint32(nv12YPitch), DRMFormat::NV12 },
        { backing.dmabufFd.get(), quint32(nv12YSize), quint32(nv12UVPitch), DRMFormat::NV12 },
    };
    const QSpan<const DmaBufPlane> planeSpan(planes, 2);

    {
        // Probe the driver first, so that an unsupported driver skips instead of looking
        // like a failure to build the texture wrappers.
        auto imported = importDmaBufTextures(
                *rhi, eglContext, planeSpan, QVideoFrameFormat::Format_NV12,
                QSize(nv12Width, nv12Height), {}, DmaBufImportMode::SingleImage);
        if (!imported)
            QSKIP("Importing a CPU-backed dma-buf as a single GL texture failed; the driver/EGL "
                  "implementation on this system likely doesn't support multi-plane EGL images");
    }

    QDmaBufVideoBuffer buffer(QVideoFrameFormat::Format_NV12, QSize(nv12Width, nv12Height),
                              planeSpan, {}, DmaBufImportMode::SingleImage);

    QVideoFrameTexturesUPtr textures = buffer.mapTextures(*rhi);
    QVERIFY(textures);
    QVERIFY(textures->texture(0));
    QVERIFY(!textures->texture(1));
}

int main(int argc, char *argv[])
{
    // Force EGL-based GL integration on xcb so that QGuiApplication exposes the
    // "egldisplay" native resource that DmaBufEglContext relies on (the default xcb_glx
    // integration does not provide one).
    qputenv("QT_XCB_GL_INTEGRATION", "xcb_egl");

    QGuiApplication app(argc, argv);
    tst_QDmaBufVideoBuffer tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_qdmabufvideobuffer.moc"
