// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qdmabufvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <private/qmultimedia_drm_support_p.h>
#include <private/udmabuftestutils_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qscopedpointer.h>
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

} // namespace

class tst_QDmaBufVideoBuffer : public QObject
{
    Q_OBJECT

private slots:
    void map_roundTripsPixelData_whenGivenUdmabufBackedPlane();
    void mapTextures_roundTripsPixelData_whenGivenUdmabufBackedPlane();
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

    QVideoFrameTexturesUPtr oldTextures;
    QVideoFrameTexturesUPtr textures = buffer.mapTextures(*rhi, oldTextures);
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
