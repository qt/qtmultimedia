// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "devicecontext.h"

ComResult<QColor> DeviceContext::getFirstPixelColor(const ComPtr<ID3D11Texture2D> &texture) const
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
    if (hr != S_OK)
        return q23::unexpected{ hr };

    // Copy the texture data to the staging texture
    context->CopyResource(stagingTexture.Get(), texture.Get());

    // Map the staging texture to access its data
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource);
    if (hr != S_OK)
        return q23::unexpected{ hr };

    // Get the value of the first pixel (top-left corner)
    unsigned char *data = static_cast<unsigned char *>(mappedResource.pData);

    QColor firstPixel{ data[0], data[1], data[2], data[3] };

    // Unmap the staging texture
    context->Unmap(stagingTexture.Get(), 0);

    return firstPixel;
}

ComResult<ComPtr<ID3D11Texture2D>>
DeviceContext::createTextureArray(QSize size, const std::vector<QColor> &colors) const
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

    if (hr != S_OK)
        return q23::unexpected{ hr };

    hr = fillTextureWithColors(tex, colors);
    if (hr != S_OK)
        return q23::unexpected{ hr };

    return tex;
}

HRESULT DeviceContext::fillTextureWithColors(const ComPtr<ID3D11Texture2D> &texture,
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

    if (hr != S_OK)
        return hr;

    // Map the staging texture to access its data
    D3D11_MAPPED_SUBRESOURCE mappedResource{};
    context->CopyResource(stagingTexture.Get(), texture.Get());
    hr = context->Map(stagingTexture.Get(), 0, D3D11_MAP_WRITE, 0, &mappedResource);
    if (hr != S_OK)
        return hr;

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

ComResult<DeviceContext> createDeviceContext()
{
    DeviceContext devContext;
    ComPtr<ID3D11Device> srcDev;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                   D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT, nullptr, 0,
                                   D3D11_SDK_VERSION, srcDev.GetAddressOf(), nullptr,
                                   devContext.context.GetAddressOf());
    if (hr != S_OK)
        return q23::unexpected{ hr };

    hr = srcDev.As(&devContext.device);
    if (hr != S_OK)
        return q23::unexpected{ hr };

    return devContext;
}
