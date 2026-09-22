// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11planesplitter_p.h"
#include "qwindowsd3d11planesplitter_shaders_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {

Q_LOGGING_CATEGORY(lcD3D11PlaneSplitter, "qt.multimedia.windows.d3d11planesplitter");

DXGI_FORMAT toPlaneViewFormat(QVideoTextureHelper::TextureDescription::TextureFormat format)
{
    switch (format) {
    case QVideoTextureHelper::TextureDescription::Red_8:
        return DXGI_FORMAT_R8_UNORM;
    case QVideoTextureHelper::TextureDescription::RG_8:
        return DXGI_FORMAT_R8G8_UNORM;
    case QVideoTextureHelper::TextureDescription::Red_16:
        return DXGI_FORMAT_R16_UNORM;
    case QVideoTextureHelper::TextureDescription::RG_16:
        return DXGI_FORMAT_R16G16_UNORM;
    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

} // namespace

QWindowsD3D11PlaneSplitter::QWindowsD3D11PlaneSplitter() = default;
QWindowsD3D11PlaneSplitter::~QWindowsD3D11PlaneSplitter() = default;

bool QWindowsD3D11PlaneSplitter::isPixelFormatSupported(QVideoFrameFormat::PixelFormat pixelFormat)
{
    // Matches the D3D11VA surface formats in FFmpeg's hwcontext_d3d11va supported_formats[].
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_P010:
        return true;
    default:
        return false;
    }
}

std::unique_ptr<QWindowsD3D11PlaneSplitter> QWindowsD3D11PlaneSplitter::create(ID3D11Device *device)
{
    Q_ASSERT(device);
    auto splitter = std::unique_ptr<QWindowsD3D11PlaneSplitter>(new QWindowsD3D11PlaneSplitter());
    if (!splitter->createPipeline(device))
        return nullptr;
    return splitter;
}

bool QWindowsD3D11PlaneSplitter::createPipeline(ID3D11Device *device)
{
    Q_ASSERT(device);
    Q_ASSERT(!m_shaderDevice);

    if (const HRESULT hr =
                device->CreateVertexShader(g_planeSplitVS, sizeof(g_planeSplitVS), nullptr, &m_vs);
        FAILED(hr)) {
        qCWarning(lcD3D11PlaneSplitter)
                << "CreateVertexShader failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    if (const HRESULT hr =
                device->CreatePixelShader(g_planeSplitPS, sizeof(g_planeSplitPS), nullptr, &m_ps);
        FAILED(hr)) {
        qCWarning(lcD3D11PlaneSplitter)
                << "CreatePixelShader failed:" << QSystemError::windowsComString(hr);
        m_vs.Reset();
        return false;
    }

    m_shaderDevice = device;
    return true;
}

std::pair<ComPtr<ID3D11Texture2D>, UINT> QWindowsD3D11PlaneSplitter::shaderReadableTexture(
        ID3D11Device *device, ID3D11DeviceContext *context, const ComPtr<ID3D11Texture2D> &texture,
        UINT subresourceIndex)
{
    D3D11_TEXTURE2D_DESC srcDesc{};
    texture->GetDesc(&srcDesc);
    if (srcDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
        return { texture, subresourceIndex };

    bool needRecreate = !m_shaderReadableCopy;
    if (m_shaderReadableCopy) {
        ComPtr<ID3D11Device> copyDevice;
        m_shaderReadableCopy->GetDevice(&copyDevice);
        D3D11_TEXTURE2D_DESC copyDesc{};
        m_shaderReadableCopy->GetDesc(&copyDesc);
        needRecreate = copyDevice.Get() != device || copyDesc.Format != srcDesc.Format
                || copyDesc.Width != srcDesc.Width || copyDesc.Height != srcDesc.Height;
    }

    if (needRecreate) {
        D3D11_TEXTURE2D_DESC copyDesc{};
        copyDesc.Width = srcDesc.Width;
        copyDesc.Height = srcDesc.Height;
        copyDesc.MipLevels = 1;
        copyDesc.ArraySize = 1;
        copyDesc.Format = srcDesc.Format;
        copyDesc.SampleDesc.Count = 1;
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        copyDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        if (const HRESULT hr = device->CreateTexture2D(
                    &copyDesc, nullptr, m_shaderReadableCopy.ReleaseAndGetAddressOf());
            FAILED(hr)) {
            qCWarning(lcD3D11PlaneSplitter) << "CreateTexture2D failed for shader-readable copy:"
                                            << QSystemError::windowsComString(hr);
            return {};
        }
    }

    context->CopySubresourceRegion(m_shaderReadableCopy.Get(), 0, 0, 0, 0, texture.Get(),
                                   subresourceIndex, nullptr);
    return { m_shaderReadableCopy, 0 };
}

std::optional<std::vector<QWindowsD3D11PlaneSplitter::Plane>> QWindowsD3D11PlaneSplitter::split(
        ID3D11Device *device, ID3D11DeviceContext *context, const ComPtr<ID3D11Texture2D> &texture,
        UINT subresourceIndex, const QSize &frameSize, QVideoFrameFormat::PixelFormat pixelFormat)
{
    Q_ASSERT(device);
    Q_ASSERT(context);
    Q_ASSERT(texture);
    Q_ASSERT(m_shaderDevice.Get() == device);
    Q_ASSERT(m_vs && m_ps);
    Q_ASSERT(isPixelFormatSupported(pixelFormat));

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    Q_ASSERT(desc && desc->nplanes == 2);

    const auto [sourceTexture, sourceSlice] =
            shaderReadableTexture(device, context, texture, subresourceIndex);
    if (!sourceTexture)
        return std::nullopt;

    std::vector<Plane> result;
    result.reserve(desc->nplanes);

    context->IASetInputLayout(nullptr);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(m_vs.Get(), nullptr, 0);
    context->PSSetShader(m_ps.Get(), nullptr, 0);

    for (int plane = 0; plane < desc->nplanes; ++plane) {
        const DXGI_FORMAT viewFormat = toPlaneViewFormat(desc->textureFormat[plane]);
        if (viewFormat == DXGI_FORMAT_UNKNOWN) {
            qCWarning(lcD3D11PlaneSplitter) << "Unsupported plane texture format for pixel format"
                                            << pixelFormat << "plane" << plane;
            return std::nullopt;
        }

        const QSize planeSize{ desc->widthForPlane(frameSize.width(), plane),
                               desc->heightForPlane(frameSize.height(), plane) };

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = viewFormat;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
        srvDesc.Texture2DArray.MostDetailedMip = 0;
        srvDesc.Texture2DArray.MipLevels = 1;
        srvDesc.Texture2DArray.FirstArraySlice = sourceSlice;
        srvDesc.Texture2DArray.ArraySize = 1;

        ComPtr<ID3D11ShaderResourceView> srv;
        if (const HRESULT hr =
                    device->CreateShaderResourceView(sourceTexture.Get(), &srvDesc, &srv);
            FAILED(hr)) {
            qCWarning(lcD3D11PlaneSplitter) << "CreateShaderResourceView failed for plane" << plane
                                            << ":" << QSystemError::windowsComString(hr);
            return std::nullopt;
        }

        D3D11_TEXTURE2D_DESC rtDesc{};
        rtDesc.Width = static_cast<UINT>(planeSize.width());
        rtDesc.Height = static_cast<UINT>(planeSize.height());
        rtDesc.MipLevels = 1;
        rtDesc.ArraySize = 1;
        rtDesc.Format = viewFormat;
        rtDesc.SampleDesc.Count = 1;
        rtDesc.Usage = D3D11_USAGE_DEFAULT;
        rtDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

        ComPtr<ID3D11Texture2D> planeTex;
        if (const HRESULT hr = device->CreateTexture2D(&rtDesc, nullptr, &planeTex); FAILED(hr)) {
            qCWarning(lcD3D11PlaneSplitter) << "CreateTexture2D failed for plane" << plane << ":"
                                            << QSystemError::windowsComString(hr);
            return std::nullopt;
        }

        ComPtr<ID3D11RenderTargetView> rtv;
        if (const HRESULT hr = device->CreateRenderTargetView(planeTex.Get(), nullptr, &rtv);
            FAILED(hr)) {
            qCWarning(lcD3D11PlaneSplitter) << "CreateRenderTargetView failed for plane" << plane
                                            << ":" << QSystemError::windowsComString(hr);
            return std::nullopt;
        }

        const D3D11_VIEWPORT viewport{
            0.0f, 0.0f, float(planeSize.width()), float(planeSize.height()), 0.0f, 1.0f
        };
        context->RSSetViewports(1, &viewport);
        ID3D11RenderTargetView *rtvs[] = { rtv.Get() };
        context->OMSetRenderTargets(1, rtvs, nullptr);
        ID3D11ShaderResourceView *srvs[] = { srv.Get() };
        context->PSSetShaderResources(0, 1, srvs);
        context->Draw(3, 0);

        result.push_back({ std::move(planeTex), planeSize });
    }

    ID3D11ShaderResourceView *nullSrvs[] = { nullptr };
    context->PSSetShaderResources(0, 1, nullSrvs);
    ID3D11RenderTargetView *nullRtvs[] = { nullptr };
    context->OMSetRenderTargets(1, nullRtvs, nullptr);

    return result;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
