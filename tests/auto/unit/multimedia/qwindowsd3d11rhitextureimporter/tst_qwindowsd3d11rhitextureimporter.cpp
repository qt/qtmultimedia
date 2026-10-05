// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>

#include <QtMultimedia/private/qwindowsd3d11rhitextureimporter_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>
#include <QtGui/qoffscreensurface.h>
#include <QtGui/qcolor.h>

#include <QtCore/private/qsystemerror_p.h>

using namespace Qt::StringLiterals;
using namespace QtMultimediaTest;
using QWindowsD3D11RhiTextureImporter = QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter;

// Helper macro to verify q23::expected<T, HRESULT>
#define QVERIFYCOMRESULT(comresult) \
    QVERIFY2(comresult,             \
             comresult ? "no error" \
                       : qPrintable(QSystemError::windowsComString((comresult).error())))

namespace {

std::unique_ptr<QRhi> createD3D12Rhi()
{
    QRhiD3D12InitParams params;
    return std::unique_ptr<QRhi>{ QRhi::create(QRhi::D3D12, &params) };
}

#if QT_CONFIG(opengl)
std::unique_ptr<QRhi> createGles2Rhi(QOffscreenSurface *fallbackSurface)
{
    QRhiGles2InitParams params;
    params.fallbackSurface = fallbackSurface;
    return std::unique_ptr<QRhi>{ QRhi::create(QRhi::OpenGLES2, &params) };
}
#endif

ComPtr<ID3D11Texture2D> nativeTextureFromHandle(quint64 handle)
{
    return reinterpret_cast<ID3D11Texture2D *>(handle);
}

} // namespace

class tst_QWindowsD3D11RhiTextureImporter : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void importTexture_returnsWorkingTexture_whenRhiIsD3D11();
    void importTexture_returnsPlaneShapes_whenNV12AndRhiIsD3D11();
    void importTexture_reusesResources_whenCalledRepeatedly();
    void importTexture_recreatesSharedTexture_whenFrameSizeChanges();

    void isRhiBackendSupported_returnsFalse_whenRhiIsNull();
    void isRhiBackendSupported_returnsFalse_whenRhiIsWarpD3D11();
    void isRhiBackendSupported_returnsFalse_whenRhiIsD3D12();
#if QT_CONFIG(opengl)
    void isRhiBackendSupported_returnsFalse_whenRhiIsOpenGL();
#endif

private:
    ComResult<QWindowsD3D11TestDeviceContext> m_srcDevice = q23::unexpected{ E_FAIL };
};

void tst_QWindowsD3D11RhiTextureImporter::init()
{
    m_srcDevice = createD3D11TestDeviceContext();
    QVERIFYCOMRESULT(m_srcDevice);
}

void tst_QWindowsD3D11RhiTextureImporter::importTexture_returnsWorkingTexture_whenRhiIsD3D11()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this machine");

    QVERIFY(QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_srcDevice->createTextureArray(frameSize, { Qt::red });
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11RhiTextureImporter importer;
    QVideoFrameTexturesHandlesUPtr handles = importer.importTexture(
            *rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(), *srcTex, frameSize);
    QVERIFY(handles);

    const quint64 handle = handles->textureHandle(*rhi, 0);
    QVERIFY(handle != 0);

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, QVideoFrameFormat::Format_RGBA8888, frameSize);
    QVERIFY(textures);

    QRhiTexture *tex = textures->texture(0);
    QVERIFY(tex);
    QCOMPARE(tex->pixelSize(), frameSize);

    // Verify the copied pixel data at the raw D3D11 level, on the same native resource that
    // the QRhiTexture above wraps.
    const ComResult<QWindowsD3D11TestDeviceContext> rhiDevice = wrapRhiD3D11Device(*rhi);
    QVERIFYCOMRESULT(rhiDevice);

    const ComResult<QColor> actualColor =
            rhiDevice->getFirstPixelColor(nativeTextureFromHandle(handle));
    QVERIFYCOMRESULT(actualColor);
    QCOMPARE_EQ(*actualColor, QColor(Qt::red));
}

void tst_QWindowsD3D11RhiTextureImporter::importTexture_returnsPlaneShapes_whenNV12AndRhiIsD3D11()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this machine");

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex = m_srcDevice->createNV12Texture(frameSize);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11RhiTextureImporter importer;
    QVideoFrameTexturesHandlesUPtr handles = importer.importTexture(
            *rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(), *srcTex, frameSize);
    QVERIFY(handles);

    // A single physical resource backs both planes for D3D11: the SRV format (R8 for luma,
    // RG8 for chroma) is what makes them behave as different planes.
    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    const quint64 chromaHandle = handles->textureHandle(*rhi, 1);
    QVERIFY(lumaHandle != 0);
    QCOMPARE_EQ(chromaHandle, lumaHandle);

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, QVideoFrameFormat::Format_NV12, frameSize);
    QVERIFY(textures);

    QRhiTexture *luma = textures->texture(0);
    QRhiTexture *chroma = textures->texture(1);
    QVERIFY(luma);
    QVERIFY(chroma);

    QCOMPARE(luma->pixelSize(), frameSize);
    QCOMPARE(chroma->pixelSize(), QSize(frameSize.width() / 2, frameSize.height() / 2));

    const auto *desc = QVideoTextureHelper::textureDescription(QVideoFrameFormat::Format_NV12);
    QCOMPARE(luma->format(), desc->rhiTextureFormat(0, rhi.get()));
    QCOMPARE(chroma->format(), desc->rhiTextureFormat(1, rhi.get()));
}

void tst_QWindowsD3D11RhiTextureImporter::importTexture_reusesResources_whenCalledRepeatedly()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this machine");

    constexpr QSize frameSize{ 64, 32 };
    QWindowsD3D11RhiTextureImporter importer;

    const ComResult<QWindowsD3D11TestDeviceContext> rhiDevice = wrapRhiD3D11Device(*rhi);
    QVERIFYCOMRESULT(rhiDevice);

    const std::vector<QColor> colors{ Qt::yellow, Qt::cyan, Qt::magenta };
    for (const QColor &color : colors) {
        const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                m_srcDevice->createTextureArray(frameSize, { color });
        QVERIFYCOMRESULT(srcTex);

        QVideoFrameTexturesHandlesUPtr handles = importer.importTexture(
                *rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(), *srcTex, frameSize);
        QVERIFY(handles);

        const quint64 handle = handles->textureHandle(*rhi, 0);
        const ComResult<QColor> actualColor =
                rhiDevice->getFirstPixelColor(nativeTextureFromHandle(handle));
        QVERIFYCOMRESULT(actualColor);
        QCOMPARE_EQ(*actualColor, color);
    }
}

void tst_QWindowsD3D11RhiTextureImporter::
        importTexture_recreatesSharedTexture_whenFrameSizeChanges()
{
    std::unique_ptr<QRhi> rhi = createOffscreenD3D11Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi on this machine");

    QWindowsD3D11RhiTextureImporter importer;

    const ComResult<QWindowsD3D11TestDeviceContext> rhiDevice = wrapRhiD3D11Device(*rhi);
    QVERIFYCOMRESULT(rhiDevice);

    const std::vector<QSize> frameSizes{ QSize{ 64, 32 }, QSize{ 65, 32 }, QSize{ 200, 100 } };
    for (const QSize &frameSize : frameSizes) {
        const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
                m_srcDevice->createTextureArray(frameSize, { Qt::green });
        QVERIFYCOMRESULT(srcTex);

        QVideoFrameTexturesHandlesUPtr handles = importer.importTexture(
                *rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(), *srcTex, frameSize);
        QVERIFY(handles);

        const quint64 handle = handles->textureHandle(*rhi, 0);
        D3D11_TEXTURE2D_DESC desc{};
        nativeTextureFromHandle(handle)->GetDesc(&desc);
        QCOMPARE(QSize(int(desc.Width), int(desc.Height)), frameSize);

        const ComResult<QColor> actualColor =
                rhiDevice->getFirstPixelColor(nativeTextureFromHandle(handle));
        QVERIFYCOMRESULT(actualColor);
        QCOMPARE_EQ(*actualColor, QColor(Qt::green));
    }
}

void tst_QWindowsD3D11RhiTextureImporter::isRhiBackendSupported_returnsFalse_whenRhiIsNull()
{
    // importTexture() asserts isRhiBackendSupported() as a precondition rather than failing
    // gracefully - callers must check this themselves before ever calling it.
    std::unique_ptr<QRhi> rhi = createNullRhi();
    QVERIFY(rhi);

    QVERIFY(!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));
}

void tst_QWindowsD3D11RhiTextureImporter::isRhiBackendSupported_returnsFalse_whenRhiIsWarpD3D11()
{
    // A WARP (software) D3D11 device lives on a different adapter than the source device, so
    // sharing textures with it would either fail or, worse, stall for the keyed mutex timeout.
    const ComResult<QWindowsD3D11TestDeviceContext> warpDevice = createWarpD3D11TestDeviceContext();
    if (!warpDevice)
        QSKIP("Could not create a WARP D3D11 device on this machine");

    std::unique_ptr<QRhi> rhi = createWarpD3D11Rhi(*warpDevice);
    if (!rhi)
        QSKIP("Could not create a D3D11 QRhi wrapping a WARP device on this machine");
    QCOMPARE_EQ(rhi->driverInfo().deviceType, QRhiDriverInfo::CpuDevice);

    QVERIFY(!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));
}

void tst_QWindowsD3D11RhiTextureImporter::isRhiBackendSupported_returnsFalse_whenRhiIsD3D12()
{
    // D3D12 support is not implemented yet; verify the gap is reported rather than asserting.
    std::unique_ptr<QRhi> rhi = createD3D12Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D12 QRhi on this machine");

    QVERIFY(!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));
}

#if QT_CONFIG(opengl)
void tst_QWindowsD3D11RhiTextureImporter::isRhiBackendSupported_returnsFalse_whenRhiIsOpenGL()
{
    // OpenGL support is not implemented yet; verify the gap is reported rather than asserting.
    std::unique_ptr<QOffscreenSurface> fallbackSurface{ QRhiGles2InitParams::newFallbackSurface() };
    std::unique_ptr<QRhi> rhi = createGles2Rhi(fallbackSurface.get());
    if (!rhi)
        QSKIP("Could not create an OpenGL QRhi on this machine");

    QVERIFY(!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));
}
#endif

QTEST_MAIN(tst_QWindowsD3D11RhiTextureImporter)

#include "tst_qwindowsd3d11rhitextureimporter.moc"
