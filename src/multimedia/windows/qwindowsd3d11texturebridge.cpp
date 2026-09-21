// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11texturebridge_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>

#include <dxgi1_2.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {
Q_LOGGING_CATEGORY(lcD3D11TextureBridge, "qt.multimedia.windows.d3d11texturebridge");
}

// Timeout for keyed mutex acquisition. Using a finite timeout prevents
// deadlocks when the D3D device is lost between AcquireSync and ReleaseSync.
static constexpr DWORD kMutexTimeoutMs = 5000;

bool QWindowsD3D11TextureBridge::copyToSharedTex(ID3D11Device *dev, ID3D11DeviceContext *ctx,
                                                 const ComPtr<ID3D11Texture2D> &tex,
                                                 const QSize &frameSize, UINT index)
{
    Q_ASSERT(dev);
    Q_ASSERT(ctx);
    Q_ASSERT(tex);

    if (!ensureSrcTex(dev, tex, frameSize))
        return false;

    // Flush to ensure that texture is fully updated before we share it.
    ctx->Flush();

    if (const HRESULT hr = m_srcMutex->AcquireSync(kSrcKey, kMutexTimeoutMs); hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "AcquireSync failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());

    // A crop box is needed because the source texture may be bigger than the
    // frame size, e.g. to account for a decoder's surface alignment requirements.
    const D3D11_BOX crop{ 0, 0, 0, width, height, 1 };
    ctx->CopySubresourceRegion(m_srcTex.Get(), 0, 0, 0, 0, tex.Get(), index, &crop);

    m_srcMutex->ReleaseSync(kDestKey);
    return true;
}

bool QWindowsD3D11TextureBridge::copyToSharedTex(const ComPtr<ID3D11Device> &device,
                                                 const ComPtr<ID3D11DeviceContext> &deviceContext,
                                                 const ComPtr<ID3D11Texture2D> &texture,
                                                 const QSize &frameSize, UINT index)
{
    return copyToSharedTex(device.Get(), deviceContext.Get(), texture, frameSize, index);
}

ComPtr<ID3D11Texture2D>
QWindowsD3D11TextureBridge::copyFromSharedTex(const ComPtr<ID3D11Device1> &dev,
                                              const ComPtr<ID3D11DeviceContext> &ctx)
{
    if (!ensureDestTex(dev))
        return {};

    if (const HRESULT hr = m_destMutex->AcquireSync(kDestKey, kMutexTimeoutMs); hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "AcquireSync failed:" << QSystemError::windowsComString(hr);
        return {};
    }

    ctx->CopySubresourceRegion(m_outputTex.Get(), 0, 0, 0, 0, m_destTex.Get(), 0, nullptr);

    m_destMutex->ReleaseSync(kSrcKey);

    return m_outputTex;
}

bool QWindowsD3D11TextureBridge::ensureDestTex(const ComPtr<ID3D11Device1> &dev)
{
    if (m_destDevice != dev) {
        // Destination device changed. Recreate texture.
        m_destTex = nullptr;
        m_destDevice = dev;
    }

    if (m_destTex)
        return true;

    if (const HRESULT hr =
                m_destDevice->OpenSharedResource1(m_sharedHandle.get(), IID_PPV_ARGS(&m_destTex));
        hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "OpenSharedResource1 failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    CD3D11_TEXTURE2D_DESC desc{};
    m_destTex->GetDesc(&desc);

    desc.MiscFlags = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    if (const HRESULT hr =
                m_destDevice->CreateTexture2D(&desc, nullptr, m_outputTex.ReleaseAndGetAddressOf());
        hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "CreateTexture2D failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    if (const HRESULT hr = m_destTex.As(&m_destMutex); hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "QueryInterface(IDXGIKeyedMutex) failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    return true;
}

bool QWindowsD3D11TextureBridge::ensureSrcTex(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex,
                                              const QSize &frameSize)
{
    if (!isSrcInitialized(dev, tex, frameSize))
        return recreateSrc(dev, tex, frameSize);

    return true;
}

bool QWindowsD3D11TextureBridge::isSrcInitialized(const ID3D11Device *dev,
                                                  const ComPtr<ID3D11Texture2D> &tex,
                                                  const QSize &frameSize) const
{
    if (!m_srcTex)
        return false;

    // Check if device has changed
    ComPtr<ID3D11Device> texDevice;
    m_srcTex->GetDevice(texDevice.GetAddressOf());
    if (dev != texDevice.Get())
        return false;

    // Check if shared texture has correct size and format
    CD3D11_TEXTURE2D_DESC inputDesc{};
    tex->GetDesc(&inputDesc);

    CD3D11_TEXTURE2D_DESC currentDesc{};
    m_srcTex->GetDesc(&currentDesc);

    if (inputDesc.Format != currentDesc.Format)
        return false;

    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());

    if (currentDesc.Width != width || currentDesc.Height != height)
        return false;

    return true;
}

bool QWindowsD3D11TextureBridge::recreateSrc(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex,
                                             const QSize &frameSize)
{
    m_sharedHandle.close();

    CD3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);

    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());

    CD3D11_TEXTURE2D_DESC texDesc{ desc.Format, width, height };
    texDesc.MipLevels = 1;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    if (const HRESULT hr =
                dev->CreateTexture2D(&texDesc, nullptr, m_srcTex.ReleaseAndGetAddressOf());
        hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "CreateTexture2D failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    ComPtr<IDXGIResource1> res;
    if (const HRESULT hr = m_srcTex.As(&res); hr != S_OK) {
        qCWarning(lcD3D11TextureBridge)
                << "QueryInterface(IDXGIResource1) failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    const HRESULT hr =
            res->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &m_sharedHandle);

    if (hr != S_OK || !m_sharedHandle) {
        qCWarning(lcD3D11TextureBridge)
                << "CreateSharedHandle failed:" << QSystemError::windowsComString(hr);
        return false;
    }

    if (const HRESULT mutexHr = m_srcTex.As(&m_srcMutex); mutexHr != S_OK || !m_srcMutex) {
        qCWarning(lcD3D11TextureBridge) << "QueryInterface(IDXGIKeyedMutex) failed:"
                                        << QSystemError::windowsComString(mutexHr);
        return false;
    }

    m_destTex = nullptr;
    m_destMutex = nullptr;
    return true;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
