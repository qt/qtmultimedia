// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>

#include <QtMultimediaTestLib/private/qwindowsd3d11testdevicecontext_p.h>

#include <QtMultimedia/private/qwindowsd3d11planesplitter_p.h>

#include <QtCore/private/qsystemerror_p.h>

using QWindowsD3D11PlaneSplitter = QtMultimediaPrivate::QWindowsD3D11PlaneSplitter;

#define QVERIFYCOMRESULT(comresult) \
    QVERIFY2(comresult,             \
             comresult ? "no error" \
                       : qPrintable(QSystemError::windowsComString((comresult).error())))

namespace {

// Mimics FFmpeg's D3D11VA decoder pool: always a texture array, with shader-resource access.
ComResult<ComPtr<ID3D11Texture2D>> createNV12TextureArray(const QWindowsD3D11TestDeviceContext &dev,
                                                          QSize size, UINT arraySize)
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = static_cast<UINT>(size.width());
    texDesc.Height = static_cast<UINT>(size.height());
    texDesc.Format = DXGI_FORMAT_NV12;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = arraySize;
    texDesc.SampleDesc = { 1, 0 };

    // Some drivers reject a multi-slice planar array without D3D11_BIND_DECODER (see
    // SetupDecoderTextures() in qffmpeghwaccel_d3d11.cpp). Software (WARP) drivers reject the
    // flag itself, so fall back to a shader-resource-only array there: the splitter only needs
    // the plane views.
    ComResult<ComPtr<ID3D11Texture2D>> tex = q23::unexpected{ E_FAIL };
    for (const UINT bindFlags : { UINT(D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DECODER),
                                  UINT(D3D11_BIND_SHADER_RESOURCE) }) {
        texDesc.BindFlags = bindFlags;
        ComPtr<ID3D11Texture2D> created;
        const HRESULT hr = dev.device->CreateTexture2D(&texDesc, nullptr, &created);
        if (hr == S_OK)
            return created;
        tex = q23::unexpected{ hr };
    }
    return tex;
}

struct SubresourceBytes
{
    std::vector<uint8_t> data;
    UINT rowPitch;
};

ComResult<SubresourceBytes> readSubresourceBytes(const QWindowsD3D11TestDeviceContext &dev,
                                                 const ComPtr<ID3D11Texture2D> &texture)
{
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> staging;
    if (const HRESULT hr = dev.device->CreateTexture2D(&stagingDesc, nullptr, &staging); hr != S_OK)
        return q23::unexpected{ hr };

    dev.context->CopyResource(staging.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (const HRESULT hr = dev.context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        hr != S_OK)
        return q23::unexpected{ hr };

    SubresourceBytes result;
    result.rowPitch = mapped.RowPitch;
    const auto *base = static_cast<const uint8_t *>(mapped.pData);
    result.data.assign(base, base + size_t(mapped.RowPitch) * desc.Height);

    dev.context->Unmap(staging.Get(), 0);
    return result;
}

quint8 byteAt(const SubresourceBytes &bytes, UINT x, UINT y, UINT stride, UINT offsetInPixel)
{
    return bytes.data[size_t(y) * bytes.rowPitch + size_t(x) * stride + offsetInPixel];
}

} // namespace

class tst_QWindowsD3D11PlaneSplitter : public QObject
{
    Q_OBJECT

private slots:
    void init();

    void isPixelFormatSupported_returnsTrue_onlyForNV12AndP010();
    void split_returnsCorrectPlaneShapesAndContent_forNV12();
    void split_returnsCorrectContent_whenTextureLacksShaderResourceBinding();
    void split_returnsCorrectPlaneShapesAndContent_forP010();

private:
    ComResult<QWindowsD3D11TestDeviceContext> m_device = q23::unexpected{ E_FAIL };
};

void tst_QWindowsD3D11PlaneSplitter::init()
{
    m_device = createD3D11TestDeviceContext();
    QVERIFYCOMRESULT(m_device);
}

void tst_QWindowsD3D11PlaneSplitter::isPixelFormatSupported_returnsTrue_onlyForNV12AndP010()
{
    QVERIFY(QWindowsD3D11PlaneSplitter::isPixelFormatSupported(QVideoFrameFormat::Format_NV12));
    QVERIFY(QWindowsD3D11PlaneSplitter::isPixelFormatSupported(QVideoFrameFormat::Format_P010));
    QVERIFY(!QWindowsD3D11PlaneSplitter::isPixelFormatSupported(
            QVideoFrameFormat::Format_RGBA8888));
    QVERIFY(!QWindowsD3D11PlaneSplitter::isPixelFormatSupported(
            QVideoFrameFormat::Format_BGRA8888));
    QVERIFY(!QWindowsD3D11PlaneSplitter::isPixelFormatSupported(QVideoFrameFormat::Format_YUYV));
}

void tst_QWindowsD3D11PlaneSplitter::split_returnsCorrectPlaneShapesAndContent_forNV12()
{
    constexpr QSize frameSize{ 16, 8 };
    // fillNV12Texture() copies through a staging texture, where drivers reject >2-slice planar
    // arrays without D3D11_BIND_DECODER; 2 slices still prove a non-zero slice is addressed.
    constexpr UINT arraySize = 2;
    constexpr UINT testSlice = 1;

    const ComResult<ComPtr<ID3D11Texture2D>> pool =
            createNV12TextureArray(*m_device, frameSize, arraySize);
    QVERIFYCOMRESULT(pool);

    // Distinct values per slice make cross-slice bleed visible.
    for (UINT slice = 0; slice < arraySize; ++slice)
        QCOMPARE(m_device->fillNV12Texture(*pool, frameSize, quint8(0x10 + slice), 0, 0, slice),
                 S_OK);
    constexpr quint8 lumaValue = 0x42;
    constexpr quint8 chromaU = 0x11;
    constexpr quint8 chromaV = 0xEE;
    QCOMPARE(m_device->fillNV12Texture(*pool, frameSize, lumaValue, chromaU, chromaV, testSlice),
             S_OK);

    auto splitter = QWindowsD3D11PlaneSplitter::create(m_device->device.Get());
    QVERIFY(splitter);
    const auto splitPlanes = splitter->split(m_device->device.Get(), m_device->context.Get(), *pool,
                                             testSlice, frameSize, QVideoFrameFormat::Format_NV12);
    QVERIFY(splitPlanes.has_value());
    const auto &planes = *splitPlanes;
    QCOMPARE(planes.size(), 2u);

    QCOMPARE(planes[0].size, frameSize);
    QCOMPARE(planes[1].size, QSize(frameSize.width() / 2, frameSize.height() / 2));

    D3D11_TEXTURE2D_DESC lumaDesc{};
    planes[0].texture->GetDesc(&lumaDesc);
    QCOMPARE(lumaDesc.Format, DXGI_FORMAT_R8_UNORM);
    D3D11_TEXTURE2D_DESC chromaDesc{};
    planes[1].texture->GetDesc(&chromaDesc);
    QCOMPARE(chromaDesc.Format, DXGI_FORMAT_R8G8_UNORM);

    const ComResult<SubresourceBytes> lumaBytes =
            readSubresourceBytes(*m_device, planes[0].texture);
    QVERIFYCOMRESULT(lumaBytes);
    for (int y = 0; y < frameSize.height(); ++y)
        for (int x = 0; x < frameSize.width(); ++x)
            QCOMPARE(byteAt(*lumaBytes, UINT(x), UINT(y), 1, 0), lumaValue);

    const ComResult<SubresourceBytes> chromaBytes =
            readSubresourceBytes(*m_device, planes[1].texture);
    QVERIFYCOMRESULT(chromaBytes);
    for (int y = 0; y < frameSize.height() / 2; ++y) {
        for (int x = 0; x < frameSize.width() / 2; ++x) {
            QCOMPARE(byteAt(*chromaBytes, UINT(x), UINT(y), 2, 0), chromaU);
            QCOMPARE(byteAt(*chromaBytes, UINT(x), UINT(y), 2, 1), chromaV);
        }
    }
}

void tst_QWindowsD3D11PlaneSplitter::
        split_returnsCorrectContent_whenTextureLacksShaderResourceBinding()
{
    // Mirrors the FFmpeg backend's frame copies, created without D3D11_BIND_SHADER_RESOURCE.
    constexpr QSize frameSize{ 16, 8 };
    const ComResult<ComPtr<ID3D11Texture2D>> tex = m_device->createNV12Texture(frameSize);
    QVERIFYCOMRESULT(tex);

    constexpr quint8 lumaValue = 0x42;
    constexpr quint8 chromaU = 0x11;
    constexpr quint8 chromaV = 0xEE;
    QCOMPARE(m_device->fillNV12Texture(*tex, frameSize, lumaValue, chromaU, chromaV), S_OK);

    auto splitter = QWindowsD3D11PlaneSplitter::create(m_device->device.Get());
    QVERIFY(splitter);
    const auto splitPlanes = splitter->split(m_device->device.Get(), m_device->context.Get(), *tex,
                                             0, frameSize, QVideoFrameFormat::Format_NV12);
    QVERIFY(splitPlanes.has_value());
    const auto &planes = *splitPlanes;
    QCOMPARE(planes.size(), 2u);

    const ComResult<SubresourceBytes> lumaBytes =
            readSubresourceBytes(*m_device, planes[0].texture);
    QVERIFYCOMRESULT(lumaBytes);
    QCOMPARE(byteAt(*lumaBytes, 0, 0, 1, 0), lumaValue);
    QCOMPARE(byteAt(*lumaBytes, UINT(frameSize.width() - 1), UINT(frameSize.height() - 1), 1, 0),
             lumaValue);

    const ComResult<SubresourceBytes> chromaBytes =
            readSubresourceBytes(*m_device, planes[1].texture);
    QVERIFYCOMRESULT(chromaBytes);
    QCOMPARE(byteAt(*chromaBytes, 0, 0, 2, 0), chromaU);
    QCOMPARE(byteAt(*chromaBytes, 0, 0, 2, 1), chromaV);
}

void tst_QWindowsD3D11PlaneSplitter::split_returnsCorrectPlaneShapesAndContent_forP010()
{
    // No 16-bit fill helper exists; checks plane shapes/formats only, as the texel-fetch path
    // is already covered byte-exactly by the NV12 case.
    constexpr QSize frameSize{ 16, 8 };

    // Software (WARP) devices can't create P010 textures; they report that via E_NOTIMPL.
    UINT formatSupport = 0;
    if (FAILED(m_device->device->CheckFormatSupport(DXGI_FORMAT_P010, &formatSupport))
        || !(formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D)) {
        QSKIP("Device can't create P010 textures (software adapter)");
    }

    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = UINT(frameSize.width());
    texDesc.Height = UINT(frameSize.height());
    texDesc.Format = DXGI_FORMAT_P010;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> tex;
    QCOMPARE(m_device->device->CreateTexture2D(&texDesc, nullptr, &tex), S_OK);

    auto splitter = QWindowsD3D11PlaneSplitter::create(m_device->device.Get());
    QVERIFY(splitter);
    const auto splitPlanes = splitter->split(m_device->device.Get(), m_device->context.Get(), tex,
                                             0, frameSize, QVideoFrameFormat::Format_P010);
    QVERIFY(splitPlanes.has_value());
    const auto &planes = *splitPlanes;
    QCOMPARE(planes.size(), 2u);
    QCOMPARE(planes[0].size, frameSize);
    QCOMPARE(planes[1].size, QSize(frameSize.width() / 2, frameSize.height() / 2));

    D3D11_TEXTURE2D_DESC lumaDesc{};
    planes[0].texture->GetDesc(&lumaDesc);
    QCOMPARE(lumaDesc.Format, DXGI_FORMAT_R16_UNORM);
    D3D11_TEXTURE2D_DESC chromaDesc{};
    planes[1].texture->GetDesc(&chromaDesc);
    QCOMPARE(chromaDesc.Format, DXGI_FORMAT_R16G16_UNORM);
}

QTEST_MAIN(tst_QWindowsD3D11PlaneSplitter)

#include "tst_qwindowsd3d11planesplitter.moc"
