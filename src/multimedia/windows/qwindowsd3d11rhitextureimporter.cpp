// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11rhitextureimporter_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/private/qexpected_p.h>

#include <d3d11on12.h>
#include <d3d12.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {

Q_LOGGING_CATEGORY(lcD3D11RhiTextureImporter, "qt.multimedia.windows.d3d11rhitextureimporter");

class D3D11TextureHandles : public QVideoFrameTexturesHandles
{
public:
    explicit D3D11TextureHandles(ComPtr<ID3D11Texture2D> tex) : m_tex(std::move(tex)) { }

    quint64 textureHandle(QRhi &rhi, int /*plane*/) override
    {
        if (rhi.backend() != QRhi::D3D11)
            return 0;
        return quint64(m_tex.Get());
    }

private:
    const ComPtr<ID3D11Texture2D> m_tex;
};

const QRhiD3D11NativeHandles *resolveD3D11NativeHandles(QRhi &rhi)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D11);
    return static_cast<const QRhiD3D11NativeHandles *>(rhi.nativeHandles());
}

q23::expected<ComPtr<ID3D11Device1>, HRESULT>
resolveD3D11Device(const QRhiD3D11NativeHandles *nativeHandles)
{
    ComPtr<ID3D11Device> rhiDevice = static_cast<ID3D11Device *>(nativeHandles->dev);
    ComPtr<ID3D11Device1> rhiDevice1;
    if (const HRESULT hr = rhiDevice.As(&rhiDevice1); FAILED(hr))
        return q23::unexpected{ hr };
    return rhiDevice1;
}

// Exposes the D3D12 view of a D3D11 texture, unwrapped via D3D11On12
class D3D12TextureHandles : public QVideoFrameTexturesHandles
{
public:
    D3D12TextureHandles(ComPtr<ID3D12Resource> resource, ComPtr<ID3D11On12Device2> on12Device,
                        ComPtr<ID3D11Texture2D> wrappedResource)
        : m_resource(std::move(resource)),
          m_on12Device(std::move(on12Device)),
          m_wrappedResource(std::move(wrappedResource))
    {
    }

    ~D3D12TextureHandles() override
    {
        m_resource.Reset();
        if (m_on12Device && m_wrappedResource)
            m_on12Device->ReturnUnderlyingResource(m_wrappedResource.Get(), 0, nullptr, nullptr);
    }

    quint64 textureHandle(QRhi &rhi, int /*plane*/) override
    {
        switch (rhi.backend()) {
        case QRhi::D3D12:
            return quint64(m_resource.Get());
        default:
            return 0;
        }
    }

private:
    ComPtr<ID3D12Resource> m_resource;
    const ComPtr<ID3D11On12Device2> m_on12Device;
    const ComPtr<ID3D11Texture2D> m_wrappedResource;
};

} // namespace

QWindowsD3D11RhiTextureImporter::QWindowsD3D11RhiTextureImporter() = default;
QWindowsD3D11RhiTextureImporter::~QWindowsD3D11RhiTextureImporter() = default;

bool QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(QRhi &rhi)
{
    switch (rhi.backend()) {
    case QRhi::D3D11: {
        // A software (WARP) D3D11 device lives on a different adapter than the source device,
        // so opening a shared handle across them would either fail or, worse, block on the
        // keyed mutex until the finite timeout expires. Treat it the same as unsupported.
        if (rhi.driverInfo().deviceType == QRhiDriverInfo::CpuDevice)
            return false;

        const QRhiD3D11NativeHandles *nh = resolveD3D11NativeHandles(rhi);
        return nh && nh->dev;
    }
    case QRhi::D3D12:
        return true;
    default:
        return false;
    }
}

bool QWindowsD3D11RhiTextureImporter::isPixelFormatSupportedByRhiBackend(
        QRhi &rhi, QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (rhi.backend()) {
    case QRhi::D3D12: {
        const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
        return desc && desc->nplanes == 1;
    }
    default:
        return true;
    }
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTexture(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize, UINT subresourceIndex)
{
    switch (rhi.backend()) {
    case QRhi::D3D11:
        return importTextureD3D11(rhi, srcDevice, srcContext, texture, frameSize, subresourceIndex);
    case QRhi::D3D12:
        return importTextureD3D12(rhi, srcDevice, srcContext, texture, frameSize, subresourceIndex);
    default:
        return nullptr;
    }
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTextureD3D11(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize, UINT subresourceIndex)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D11);
    const QRhiD3D11NativeHandles *nh = resolveD3D11NativeHandles(rhi);
    Q_ASSERT(nh && nh->dev);

    Q_ASSERT(srcDevice);
    Q_ASSERT(srcContext);
    Q_ASSERT(texture);

    auto device = resolveD3D11Device(nh);
    if (!device) {
        qCWarning(lcD3D11RhiTextureImporter) << "QueryInterface(ID3D11Device1) failed:"
                                             << QSystemError::windowsComString(device.error());
        return nullptr;
    }
    ComPtr<ID3D11Device1> rhiDevice = std::move(*device);

    ComPtr<ID3D11DeviceContext> rhiCtx;
    rhiDevice->GetImmediateContext(&rhiCtx);

    if (!m_bridge.copyToSharedTex(srcDevice, srcContext, texture, frameSize, subresourceIndex))
        return nullptr;

    ComPtr<ID3D11Texture2D> rhiTex = m_bridge.copyFromSharedTex(rhiDevice, rhiCtx);
    if (!rhiTex)
        return nullptr;

    return std::make_unique<D3D11TextureHandles>(std::move(rhiTex));
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTextureD3D12(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize, UINT subresourceIndex)
{
    Q_ASSERT(rhi.backend() == QRhi::D3D12);
    Q_ASSERT(srcDevice);
    Q_ASSERT(srcContext);
    Q_ASSERT(texture);

    const auto *nh = static_cast<const QRhiD3D12NativeHandles *>(rhi.nativeHandles());
    if (!nh || !nh->dev || !nh->commandQueue)
        return nullptr;

    auto *d3d12Device = static_cast<ID3D12Device *>(nh->dev);
    auto *cmdQueue = static_cast<ID3D12CommandQueue *>(nh->commandQueue);

    if (!m_d3d11on12Device) {
        // D3D11On12 needs a queue of its own: UnwrapUnderlyingResource() makes cmdQueue wait
        // for a fence that D3D11On12 only signals in its next submission. If that submission
        // goes to cmdQueue as well, it is queued behind the wait and cmdQueue deadlocks.
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        ComPtr<ID3D12CommandQueue> on12Queue;
        if (const HRESULT hr = d3d12Device->CreateCommandQueue(&queueDesc,
                                                               IID_PPV_ARGS(&on12Queue));
            FAILED(hr)) {
            qCWarning(lcD3D11RhiTextureImporter)
                    << "CreateCommandQueue:" << QSystemError::windowsComString(hr);
            return nullptr;
        }

        IUnknown *queues[] = { on12Queue.Get() };
        HRESULT hr = D3D11On12CreateDevice(d3d12Device, 0, nullptr, 0, queues, 1, 0,
                                           &m_d3d11on12Device, nullptr, nullptr);
        if (FAILED(hr)) {
            qCWarning(lcD3D11RhiTextureImporter)
                    << "D3D11On12CreateDevice:" << QSystemError::windowsComString(hr);
            return nullptr;
        }
    }

    ComPtr<ID3D11Device1> d3d11on12Device1;
    if (const HRESULT hr = m_d3d11on12Device.As(&d3d11on12Device1); FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter)
                << "QueryInterface(ID3D11Device1) failed:" << QSystemError::windowsComString(hr);
        return nullptr;
    }

    ComPtr<ID3D11DeviceContext> d3d11on12Ctx;
    m_d3d11on12Device->GetImmediateContext(&d3d11on12Ctx);

    if (!m_bridge.copyToSharedTex(srcDevice, srcContext, texture, frameSize, subresourceIndex))
        return nullptr;

    ComPtr<ID3D11Texture2D> d3d11Tex = m_bridge.copyFromSharedTex(d3d11on12Device1, d3d11on12Ctx);
    if (!d3d11Tex)
        return nullptr;

    ComPtr<ID3D11On12Device2> on12Device2;
    if (const HRESULT hr = m_d3d11on12Device.As(&on12Device2); FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter) << "QueryInterface(ID3D11On12Device2) failed:"
                                             << QSystemError::windowsComString(hr);
        return nullptr;
    }

    ComPtr<ID3D12Resource> resource;
    if (const HRESULT hr = on12Device2->UnwrapUnderlyingResource(d3d11Tex.Get(), cmdQueue,
                                                                 IID_PPV_ARGS(&resource));
        FAILED(hr)) {
        qCWarning(lcD3D11RhiTextureImporter)
                << "UnwrapUnderlyingResource:" << QSystemError::windowsComString(hr);
        return nullptr;
    }

    // UnwrapUnderlyingResource() doesn't flush: submit the copy and the fence signal that
    // cmdQueue now waits for.
    d3d11on12Ctx->Flush();

    return std::make_unique<D3D12TextureHandles>(std::move(resource), std::move(on12Device2),
                                                 std::move(d3d11Tex));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
