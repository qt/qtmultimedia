// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwindowsd3d11testdevicecontext_p.h"

#include <QtMultimediaTestLib/private/qvideoframetestutils_p.h>

#include <QtGui/rhi/qrhi.h>

#include <QtCore/private/qsystemerror_p.h>

#include <algorithm>

ComResult<QColor>
QWindowsD3D11TestDeviceContext::getFirstPixelColor(const ComPtr<ID3D11Texture2D> &texture) const
{
    // Get the texture description
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    // Create a staging texture to map the data
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
    if (FAILED(hr)) {
        qWarning() << "Failed to create staging texture for first pixel read"
                   << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    // Copy the texture data to the staging texture
    context->CopyResource(stagingTexture.Get(), texture.Get());

    // Map the staging texture to access its data
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        qWarning() << "Failed to map staging texture for first pixel read"
                   << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    // Get the value of the first pixel (top-left corner)
    unsigned char *data = static_cast<unsigned char *>(mappedResource.pData);

    QColor firstPixel{ data[0], data[1], data[2], data[3] };

    // Unmap the staging texture
    context->Unmap(stagingTexture.Get(), 0);

    return firstPixel;
}

ComResult<ComPtr<ID3D11Texture2D>>
QWindowsD3D11TestDeviceContext::createTextureArray(QSize size,
                                                   const std::vector<QColor> &colors) const
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = size.width();
    texDesc.Height = size.height();
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.BindFlags = 0;
    texDesc.ArraySize = static_cast<UINT>(colors.size());
    texDesc.MipLevels = 1;
    texDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());

    if (FAILED(hr)) {
        qWarning() << "Failed to create texture array" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    hr = fillTextureWithColors(tex, colors);
    if (FAILED(hr)) {
        qWarning() << "Failed to fill texture array" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    return tex;
}

bool QWindowsD3D11TestDeviceContext::isTextureFormatSupported(DXGI_FORMAT format) const
{
    if (!device)
        return false;

    UINT formatSupport = 0;
    const HRESULT hr = device->CheckFormatSupport(format, &formatSupport);
    if (FAILED(hr))
        return false;

    return (formatSupport & D3D11_FORMAT_SUPPORT_TEXTURE2D) != 0;
}

ComResult<ComPtr<ID3D11Texture2D>>
QWindowsD3D11TestDeviceContext::createNV12Texture(QSize size) const
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = size.width();
    texDesc.Height = size.height();
    texDesc.Format = DXGI_FORMAT_NV12;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.BindFlags = 0;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create NV12 texture" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    return tex;
}

HRESULT QWindowsD3D11TestDeviceContext::fillNV12Texture(const ComPtr<ID3D11Texture2D> &texture,
                                                        QSize size, quint8 lumaValue,
                                                        quint8 chromaUValue, quint8 chromaVValue,
                                                        UINT arraySlice) const
{
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create staging texture for NV12 fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    // texture may be a texture array (e.g. a pooled decoder surface); operate on the specific
    // array slice this frame actually occupies, not just subresource 0.
    const UINT subresource = D3D11CalcSubresource(0, arraySlice, desc.MipLevels);

    context->CopyResource(stagingTexture.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    hr = context->Map(stagingTexture.Get(), subresource, D3D11_MAP_WRITE, 0, &mappedResource);
    if (FAILED(hr)) {
        qWarning() << "Failed to map staging texture for NV12 fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    unsigned char *data = static_cast<unsigned char *>(mappedResource.pData);
    const int width = size.width();
    const int height = size.height();

    // Luma plane: one byte per pixel.
    for (int row = 0; row < height; ++row) {
        unsigned char *rowData = data + row * mappedResource.RowPitch;
        std::fill(rowData, rowData + width, lumaValue);
    }

    // Chroma plane: interleaved U/V, half the width and height of the luma plane, laid out
    // immediately below the luma plane's rows in the same subresource.
    unsigned char *chromaBase = data + height * mappedResource.RowPitch;
    for (int row = 0; row < height / 2; ++row) {
        unsigned char *rowData = chromaBase + row * mappedResource.RowPitch;
        for (int col = 0; col < width / 2; ++col) {
            rowData[2 * col] = chromaUValue;
            rowData[2 * col + 1] = chromaVValue;
        }
    }

    context->Unmap(stagingTexture.Get(), subresource);
    context->CopyResource(texture.Get(), stagingTexture.Get());

    return S_OK;
}

ComResult<ComPtr<ID3D11Texture2D>>
QWindowsD3D11TestDeviceContext::createP010Texture(QSize size) const
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = size.width();
    texDesc.Height = size.height();
    texDesc.Format = DXGI_FORMAT_P010;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.BindFlags = 0;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create P010 texture" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    return tex;
}

HRESULT QWindowsD3D11TestDeviceContext::fillP010Texture(const ComPtr<ID3D11Texture2D> &texture,
                                                        QSize size, quint16 lumaValue,
                                                        quint16 chromaUValue, quint16 chromaVValue,
                                                        UINT arraySlice) const
{
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create staging texture for P010 fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    const UINT subresource = D3D11CalcSubresource(0, arraySlice, desc.MipLevels);

    context->CopyResource(stagingTexture.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    hr = context->Map(stagingTexture.Get(), subresource, D3D11_MAP_WRITE, 0, &mappedResource);
    if (FAILED(hr)) {
        qWarning() << "Failed to map staging texture for P010 fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    unsigned char *data = static_cast<unsigned char *>(mappedResource.pData);

    // P010 stores each 10-bit sample left-shifted into the high bits of a 16-bit little-endian
    // word.
    const quint16 shiftedLuma = quint16(lumaValue << 6);
    const char lumaTexel[2] = { char(shiftedLuma & 0xff), char(shiftedLuma >> 8) };
    QtMultimediaTest::fillPlaneRows(data, mappedResource.RowPitch, size.width(), size.height(),
                                    QByteArrayView(lumaTexel, sizeof(lumaTexel)));

    const quint16 shiftedU = quint16(chromaUValue << 6);
    const quint16 shiftedV = quint16(chromaVValue << 6);
    const char chromaTexel[4] = {
        char(shiftedU & 0xff),
        char(shiftedU >> 8),
        char(shiftedV & 0xff),
        char(shiftedV >> 8),
    };
    unsigned char *chromaBase = data + size.height() * mappedResource.RowPitch;
    QtMultimediaTest::fillPlaneRows(chromaBase, mappedResource.RowPitch, size.width() / 2,
                                    size.height() / 2,
                                    QByteArrayView(chromaTexel, sizeof(chromaTexel)));

    context->Unmap(stagingTexture.Get(), subresource);
    context->CopyResource(texture.Get(), stagingTexture.Get());

    return S_OK;
}

ComResult<ComPtr<ID3D11Texture2D>>
QWindowsD3D11TestDeviceContext::createSinglePlaneTexture(QSize size, DXGI_FORMAT format) const
{
    D3D11_TEXTURE2D_DESC texDesc{};
    texDesc.Width = size.width();
    texDesc.Height = size.height();
    texDesc.Format = format;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    texDesc.BindFlags = 0;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.SampleDesc = { 1, 0 };

    ComPtr<ID3D11Texture2D> tex;
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, tex.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create single-plane texture" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    return tex;
}

HRESULT
QWindowsD3D11TestDeviceContext::fillSinglePlaneTexture(const ComPtr<ID3D11Texture2D> &texture,
                                                       QSize size, QByteArrayView texel,
                                                       UINT arraySlice) const
{
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create staging texture for single-plane fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    const UINT subresource = D3D11CalcSubresource(0, arraySlice, desc.MipLevels);

    context->CopyResource(stagingTexture.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    hr = context->Map(stagingTexture.Get(), subresource, D3D11_MAP_WRITE, 0, &mappedResource);
    if (FAILED(hr)) {
        qWarning() << "Failed to map staging texture for single-plane fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    unsigned char *data = static_cast<unsigned char *>(mappedResource.pData);
    QtMultimediaTest::fillPlaneRows(data, mappedResource.RowPitch, size.width(), size.height(),
                                    texel);

    context->Unmap(stagingTexture.Get(), subresource);
    context->CopyResource(texture.Get(), stagingTexture.Get());

    return S_OK;
}

ComResult<QByteArray>
QWindowsD3D11TestDeviceContext::readPlaneRows(const ComPtr<ID3D11Texture2D> &texture, int rowOffset,
                                              int width, int height, int texelSize,
                                              UINT arraySlice) const
{
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create staging texture for plane read"
                   << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    const UINT subresource = D3D11CalcSubresource(0, arraySlice, desc.MipLevels);

    context->CopyResource(stagingTexture.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    hr = context->Map(stagingTexture.Get(), subresource, D3D11_MAP_READ, 0, &mappedResource);
    if (FAILED(hr)) {
        qWarning() << "Failed to map staging texture for plane read"
                   << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    const auto *base = static_cast<const unsigned char *>(mappedResource.pData);
    const qsizetype rowBytes = qsizetype(width) * texelSize;
    QByteArray result(rowBytes * height, Qt::Uninitialized);
    for (int row = 0; row < height; ++row) {
        const unsigned char *rowData = base + (rowOffset + row) * mappedResource.RowPitch;
        std::copy(rowData, rowData + rowBytes, result.data() + row * rowBytes);
    }

    context->Unmap(stagingTexture.Get(), subresource);

    return result;
}

HRESULT
QWindowsD3D11TestDeviceContext::fillTextureWithColors(const ComPtr<ID3D11Texture2D> &texture,
                                                      const std::vector<QColor> &colors) const
{
    // Get the texture description
    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    // Create a staging texture to map the data
    D3D11_TEXTURE2D_DESC stagingDesc = desc;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    stagingDesc.MiscFlags = 0;

    ComPtr<ID3D11Texture2D> stagingTexture;
    HRESULT hr = device->CreateTexture2D(&stagingDesc, nullptr, stagingTexture.GetAddressOf());

    if (FAILED(hr)) {
        qWarning() << "Failed to create staging texture for color fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    // Map the staging texture to access its data
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    context->CopyResource(stagingTexture.Get(), texture.Get());
    hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_WRITE, 0, &mappedResource);
    if (FAILED(hr)) {
        qWarning() << "Failed to map staging texture for color fill"
                   << QSystemError::windowsComString(hr);
        return hr;
    }

    unsigned char *data = static_cast<unsigned char *>(mappedResource.pData);
    for (UINT plane = 0; plane < desc.ArraySize; ++plane) {
        const QColor color = colors[plane];
        for (UINT row = 0; row < desc.Height; ++row) {
            for (UINT col = 0; col < desc.Width; ++col) {
                unsigned char *pixel = data + row * mappedResource.RowPitch
                        + plane * mappedResource.DepthPitch + col * 4;
                pixel[0] = static_cast<unsigned char>(color.red());
                pixel[1] = static_cast<unsigned char>(color.green());
                pixel[2] = static_cast<unsigned char>(color.blue());
                pixel[3] = static_cast<unsigned char>(color.alpha());
            }
        }
    }

    // Unmap the staging texture and copy it back to the original texture
    context->Unmap(stagingTexture.Get(), 0);
    context->CopyResource(texture.Get(), stagingTexture.Get());

    return S_OK;
}

static ComResult<QWindowsD3D11TestDeviceContext>
createD3D11TestDeviceContextImpl(D3D_DRIVER_TYPE driverType, UINT flags)
{
    QWindowsD3D11TestDeviceContext devContext;
    ComPtr<ID3D11Device> srcDev;
    HRESULT hr =
            D3D11CreateDevice(nullptr, driverType, nullptr, flags, nullptr, 0, D3D11_SDK_VERSION,
                              srcDev.GetAddressOf(), nullptr, devContext.context.GetAddressOf());
    if (FAILED(hr)) {
        qWarning() << "Failed to create D3D11 device" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    hr = srcDev.As(&devContext.device);
    if (FAILED(hr)) {
        qWarning() << "Failed to query ID3D11Device1" << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    return devContext;
}

ComResult<QWindowsD3D11TestDeviceContext> createD3D11TestDeviceContext()
{
    // VIDEO_SUPPORT is required for the device to expose ID3D11VideoDevice/ID3D11VideoContext,
    // which FFmpeg's D3D11VA hwdevice queries for on init (see d3d11va_device_init in
    // hwcontext_d3d11va.c).
    return createD3D11TestDeviceContextImpl(D3D_DRIVER_TYPE_HARDWARE,
                                            D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT
                                                    | D3D11_CREATE_DEVICE_VIDEO_SUPPORT);
}

ComResult<QWindowsD3D11TestDeviceContext> createWarpD3D11TestDeviceContext()
{
    // WARP's software rasterizer doesn't implement ID3D11VideoDevice/ID3D11VideoContext at all:
    // requesting VIDEO_SUPPORT here makes D3D11CreateDevice fail outright with
    // DXGI_ERROR_UNSUPPORTED. Tests using a WARP device only exercise the "different adapter /
    // CpuDevice" rejection paths, which don't need it.
    return createD3D11TestDeviceContextImpl(D3D_DRIVER_TYPE_WARP, 0);
}

ComResult<QWindowsD3D11TestDeviceContext> wrapRhiD3D11Device(QRhi &rhi)
{
    const auto *native = static_cast<const QRhiD3D11NativeHandles *>(rhi.nativeHandles());
    if (!native || !native->dev) {
        qWarning() << "Failed to wrap RHI D3D11 device: invalid native handles";
        return q23::unexpected{ E_FAIL };
    }

    QWindowsD3D11TestDeviceContext ctx;
    const ComPtr<ID3D11Device> device = static_cast<ID3D11Device *>(native->dev);
    HRESULT hr = device.As(&ctx.device);
    if (FAILED(hr)) {
        qWarning() << "Failed to query ID3D11Device1 from RHI device"
                   << QSystemError::windowsComString(hr);
        return q23::unexpected{ hr };
    }

    ctx.device->GetImmediateContext(ctx.context.GetAddressOf());
    if (!ctx.context) {
        qWarning() << "Failed to get immediate context from RHI device";
        return q23::unexpected{ E_FAIL };
    }

    return ctx;
}
