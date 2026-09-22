// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>
#include <QtMultimediaTestLib/private/rhi_support_p.h>

#include <QtMultimedia/private/qwindowsd3d11rhitextureimporter_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>
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

ComPtr<ID3D11Texture2D> nativeTextureFromHandle(quint64 handle)
{
    return reinterpret_cast<ID3D11Texture2D *>(handle);
}

// With D3D11_BIND_SHADER_RESOURCE set, matching the real decoder pool (the splitter needs a
// plane-view SRV). Not folded into createNV12Texture(): ID3D11VideoProcessorInputView rejects
// the flag, and that helper is shared with tst_qwindowsd3d11videoprocessor.
ComResult<ComPtr<ID3D11Texture2D>>
createNV12TextureForSplitting(const QWindowsD3D11TestDeviceContext &dev, QSize size)
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = static_cast<UINT>(size.width());
    texDesc.Height = static_cast<UINT>(size.height());
    texDesc.Format = DXGI_FORMAT_NV12;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> tex;
    if (const HRESULT hr = dev.device->CreateTexture2D(&texDesc, nullptr, &tex); hr != S_OK)
        return q23::unexpected{ hr };
    return tex;
}

// Reads back the texture behind 'handle' and checks that every texel equals 'texel'. Returns an
// error message, or an empty string on success.
QString checkPlaneContent(QRhi &rhi, quint64 handle, QSize planeSize, QRhiTexture::Format format,
                          QByteArrayView texel)
{
    const q23::expected<QByteArray, QString> data = readBackPlane(rhi, handle, planeSize, format);
    if (!data)
        return data.error();

    const qsizetype expectedSize = qsizetype(planeSize.width()) * planeSize.height() * texel.size();
    if (data->size() != expectedSize)
        return u"Read back %1 bytes, expected %2"_s.arg(data->size()).arg(expectedSize);

    for (qsizetype i = 0; i < data->size(); ++i) {
        const uchar actual = uchar(data->at(i));
        const uchar expected = uchar(texel[i % texel.size()]);
        if (actual != expected) {
            return u"Byte %1 is 0x%2, expected 0x%3"_s.arg(i)
                    .arg(actual, 2, 16, u'0')
                    .arg(expected, 2, 16, u'0');
        }
    }
    return {};
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
    void importTexture_returnsWorkingTexture_whenRhiIsD3D12();
    void importTexture_returnsDistinctHandlesPerPlane_whenNV12AndRhiIsD3D12();
    void importTexture_keepsEachFramesContent_whenNV12AndRhiIsD3D12();
#if QT_CONFIG(opengl)
    void importTexture_returnsWorkingTexture_whenRhiIsOpenGL();
    void importTexture_returnsDistinctHandlesPerPlane_whenNV12AndRhiIsOpenGL();
    void importTexture_keepsEachFramesContent_whenNV12AndRhiIsOpenGL();
#endif

    void isRhiBackendSupported_returnsFalse_whenRhiIsNull();
    void isRhiBackendSupported_returnsFalse_whenRhiIsWarpD3D11();

private:
    // Two frames with the first one's handles kept alive, like a renderer with frames in flight.
    void verifyNV12FramesKeepTheirContent(QRhi &rhi);

    ComResult<QWindowsD3D11TestDeviceContext> m_srcDevice = q23::unexpected{ E_FAIL };
};

void tst_QWindowsD3D11RhiTextureImporter::verifyNV12FramesKeepTheirContent(QRhi &rhi)
{
    constexpr QSize frameSize{ 64, 32 };
    constexpr QSize chromaSize{ frameSize.width() / 2, frameSize.height() / 2 };

    // Like the FFmpeg backend's frame copies: no D3D11_BIND_SHADER_RESOURCE.
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex = m_srcDevice->createNV12Texture(frameSize);
    QVERIFYCOMRESULT(srcTex);

    struct Frame
    {
        quint8 luma;
        quint8 chromaU;
        quint8 chromaV;
        QVideoFrameTexturesHandlesUPtr handles;
    };
    Frame frames[] = { { 0x42, 0x11, 0xEE, nullptr }, { 0x80, 0x22, 0xDD, nullptr } };

    QWindowsD3D11RhiTextureImporter importer;
    for (Frame &frame : frames) {
        QCOMPARE(m_srcDevice->fillNV12Texture(*srcTex, frameSize, frame.luma, frame.chromaU,
                                              frame.chromaV),
                 S_OK);
        frame.handles =
                importer.importTexture(rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                       *srcTex, frameSize, QVideoFrameFormat::Format_NV12);
        QVERIFY(frame.handles);
    }

    for (Frame &frame : frames) {
        const char luma[] = { char(frame.luma) };
        QString error = checkPlaneContent(rhi, frame.handles->textureHandle(rhi, 0), frameSize,
                                          QRhiTexture::R8, QByteArrayView(luma, sizeof(luma)));
        QVERIFY2(error.isEmpty(), qPrintable(u"Luma: "_s + error));

        const char chroma[] = { char(frame.chromaU), char(frame.chromaV) };
        error = checkPlaneContent(rhi, frame.handles->textureHandle(rhi, 1), chromaSize,
                                  QRhiTexture::RG8, QByteArrayView(chroma, sizeof(chroma)));
        QVERIFY2(error.isEmpty(), qPrintable(u"Chroma: "_s + error));
    }
}

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
    QVideoFrameTexturesHandlesUPtr handles =
            importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                   *srcTex, frameSize, QVideoFrameFormat::Format_RGBA8888);
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
    QVideoFrameTexturesHandlesUPtr handles =
            importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                   *srcTex, frameSize, QVideoFrameFormat::Format_NV12);
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

        QVideoFrameTexturesHandlesUPtr handles =
                importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                       *srcTex, frameSize, QVideoFrameFormat::Format_RGBA8888);
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

        QVideoFrameTexturesHandlesUPtr handles =
                importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                       *srcTex, frameSize, QVideoFrameFormat::Format_RGBA8888);
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

void tst_QWindowsD3D11RhiTextureImporter::importTexture_returnsWorkingTexture_whenRhiIsD3D12()
{
    std::unique_ptr<QRhi> rhi = createD3D12Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D12 QRhi on this machine");

    QVERIFY(QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_srcDevice->createTextureArray(frameSize, { Qt::red });
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11RhiTextureImporter importer;
    QVideoFrameTexturesHandlesUPtr handles =
            importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                   *srcTex, frameSize, QVideoFrameFormat::Format_RGBA8888);
    QVERIFY(handles);

    // The handle is a native ID3D12Resource*, which can't be read back with the D3D11 test
    // fixture; this at least exercises the whole D3D11On12 unwrap path (and, once 'handles' is
    // destroyed, ReturnUnderlyingResource) without crashing or silently failing.
    const quint64 handle = handles->textureHandle(*rhi, 0);
    QVERIFY(handle != 0);

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, QVideoFrameFormat::Format_RGBA8888, frameSize);
    QVERIFY(textures);

    QRhiTexture *tex = textures->texture(0);
    QVERIFY(tex);
    QCOMPARE(tex->pixelSize(), frameSize);
}

void tst_QWindowsD3D11RhiTextureImporter::
        importTexture_returnsDistinctHandlesPerPlane_whenNV12AndRhiIsD3D12()
{
    std::unique_ptr<QRhi> rhi = createD3D12Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D12 QRhi on this machine");

    QVERIFY(QWindowsD3D11RhiTextureImporter::isPixelFormatSupportedByRhiBackend(
            *rhi, QVideoFrameFormat::Format_NV12));

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            createNV12TextureForSplitting(*m_srcDevice, frameSize);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11RhiTextureImporter importer;
    QVideoFrameTexturesHandlesUPtr handles =
            importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                   *srcTex, frameSize, QVideoFrameFormat::Format_NV12);
    QVERIFY(handles);

    // D3D12 imports each plane as a genuinely distinct resource, unlike D3D11's views of one handle.
    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    const quint64 chromaHandle = handles->textureHandle(*rhi, 1);
    QVERIFY(lumaHandle != 0);
    QVERIFY(chromaHandle != 0);
    QVERIFY(lumaHandle != chromaHandle);

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, QVideoFrameFormat::Format_NV12, frameSize);
    QVERIFY(textures);

    QRhiTexture *luma = textures->texture(0);
    QRhiTexture *chroma = textures->texture(1);
    QVERIFY(luma);
    QVERIFY(chroma);
    QCOMPARE(luma->pixelSize(), frameSize);
    QCOMPARE(chroma->pixelSize(), QSize(frameSize.width() / 2, frameSize.height() / 2));
}

void tst_QWindowsD3D11RhiTextureImporter::
        importTexture_keepsEachFramesContent_whenNV12AndRhiIsD3D12()
{
    std::unique_ptr<QRhi> rhi = createD3D12Rhi();
    if (!rhi)
        QSKIP("Could not create a D3D12 QRhi on this machine");

    verifyNV12FramesKeepTheirContent(*rhi);
}

#if QT_CONFIG(opengl)
void tst_QWindowsD3D11RhiTextureImporter::importTexture_returnsWorkingTexture_whenRhiIsOpenGL()
{
    q23::expected<OffscreenGlRhi, QString> glRhi = createOffscreenGlRhi();
    if (!glRhi)
        QSKIP("Could not create an OpenGL QRhi on this machine");
    QRhi *rhi = glRhi->rhi.get();

    if (!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi))
        QSKIP("OpenGL import needs GL_ARB_copy_image or OpenGL 4.3");

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            m_srcDevice->createTextureArray(frameSize, { Qt::red });
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11RhiTextureImporter importer;
    QVideoFrameTexturesHandlesUPtr handles =
            importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                   *srcTex, frameSize, QVideoFrameFormat::Format_RGBA8888);
    QVERIFY(handles);

    const quint64 handle = handles->textureHandle(*rhi, 0);
    QVERIFY(handle != 0);

    const char red[] = { char(0xff), 0, 0, char(0xff) };
    const QString error =
            checkPlaneContent(*rhi, handle, frameSize, QRhiTexture::RGBA8, QByteArrayView(red, 4));
    QVERIFY2(error.isEmpty(), qPrintable(error));

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, QVideoFrameFormat::Format_RGBA8888, frameSize);
    QVERIFY(textures);

    QRhiTexture *tex = textures->texture(0);
    QVERIFY(tex);
    QCOMPARE(tex->pixelSize(), frameSize);
}

void tst_QWindowsD3D11RhiTextureImporter::
        importTexture_returnsDistinctHandlesPerPlane_whenNV12AndRhiIsOpenGL()
{
    q23::expected<OffscreenGlRhi, QString> glRhi = createOffscreenGlRhi();
    if (!glRhi)
        QSKIP("Could not create an OpenGL QRhi on this machine");
    QRhi *rhi = glRhi->rhi.get();

    if (!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi))
        QSKIP("OpenGL import needs GL_ARB_copy_image or OpenGL 4.3");

    QVERIFY(QWindowsD3D11RhiTextureImporter::isPixelFormatSupportedByRhiBackend(
            *rhi, QVideoFrameFormat::Format_NV12));

    constexpr QSize frameSize{ 64, 32 };
    const ComResult<ComPtr<ID3D11Texture2D>> srcTex =
            createNV12TextureForSplitting(*m_srcDevice, frameSize);
    QVERIFYCOMRESULT(srcTex);

    QWindowsD3D11RhiTextureImporter importer;
    QVideoFrameTexturesHandlesUPtr handles =
            importer.importTexture(*rhi, m_srcDevice->device.Get(), m_srcDevice->context.Get(),
                                   *srcTex, frameSize, QVideoFrameFormat::Format_NV12);
    QVERIFY(handles);

    const quint64 lumaHandle = handles->textureHandle(*rhi, 0);
    const quint64 chromaHandle = handles->textureHandle(*rhi, 1);
    QVERIFY(lumaHandle != 0);
    QVERIFY(chromaHandle != 0);
    QVERIFY(lumaHandle != chromaHandle);

    QVideoFrameTexturesUPtr textures = QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), *rhi, QVideoFrameFormat::Format_NV12, frameSize);
    QVERIFY(textures);

    QRhiTexture *luma = textures->texture(0);
    QRhiTexture *chroma = textures->texture(1);
    QVERIFY(luma);
    QVERIFY(chroma);
    QCOMPARE(luma->pixelSize(), frameSize);
    QCOMPARE(chroma->pixelSize(), QSize(frameSize.width() / 2, frameSize.height() / 2));
}

void tst_QWindowsD3D11RhiTextureImporter::
        importTexture_keepsEachFramesContent_whenNV12AndRhiIsOpenGL()
{
    q23::expected<OffscreenGlRhi, QString> glRhi = createOffscreenGlRhi();
    if (!glRhi)
        QSKIP("Could not create an OpenGL QRhi on this machine");

    if (!QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*glRhi->rhi))
        QSKIP("OpenGL import needs GL_ARB_copy_image or OpenGL 4.3");

    verifyNV12FramesKeepTheirContent(*glRhi->rhi);
}
#endif // QT_CONFIG(opengl)

QTEST_MAIN(tst_QWindowsD3D11RhiTextureImporter)

#include "tst_qwindowsd3d11rhitextureimporter.moc"
