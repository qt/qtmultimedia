// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <private/qdmabuftextureimporter_p.h>
#include <private/qvideotexturehelper_p.h>
#include <private/qmultimedia_drm_support_p.h>
#include <private/udmabuftestutils_p.h>

#include <QtMultimediaTestLib/private/rhi_support_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qsurfaceformat.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qspan.h>
#include <QtCore/private/quniquehandle_types_p.h>

using namespace QtMultimediaPrivate;
using namespace QtMultimediaTest;
using namespace Qt::Literals;

class tst_QDmaBufTextureImporter : public QObject
{
    Q_OBJECT

private slots:
    void importDmaBufTextures_roundTripsPixelData_whenGivenUdmabufBackedPlane();
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

    std::unique_ptr<QOffscreenSurface> fallbackSurface;
    std::unique_ptr<QRhi> rhi = createOffscreenGlRhi(fallbackSurface);
    if (!rhi)
        QSKIP("Could not create an OpenGL ES2 QRhi backend on this system");

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
