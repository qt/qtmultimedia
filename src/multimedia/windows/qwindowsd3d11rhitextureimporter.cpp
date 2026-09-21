// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11rhitextureimporter_p.h"

#include <QtGui/rhi/qrhi.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>
#include <QtCore/private/qexpected_p.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {

Q_LOGGING_CATEGORY(lcD3D11RhiTextureImporter, "qt.multimedia.windows.d3d11rhitextureimporter");

// Exposes a plain D3D11 texture to a D3D11 QRhi. The same native handle is valid for every
// plane of a multi-planar format (e.g. NV12): QRhi selects the plane-specific view internally
// based on the QRhiTexture::Format requested for that plane.
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

} // namespace

QWindowsD3D11RhiTextureImporter::QWindowsD3D11RhiTextureImporter() = default;
QWindowsD3D11RhiTextureImporter::~QWindowsD3D11RhiTextureImporter() = default;

bool QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(QRhi &rhi)
{
    if (rhi.backend() != QRhi::D3D11)
        return false;

    // A software (WARP) D3D11 device lives on a different adapter than the source device, so
    // opening a shared handle across them would either fail or, worse, block on the keyed mutex
    // until the finite timeout expires. Treat it the same as an unsupported backend.
    if (rhi.driverInfo().deviceType == QRhiDriverInfo::CpuDevice)
        return false;

    // Without a native D3D11 device, there's nothing to import into.
    // QRhi::nativeHandles() isn't declared const, even though it doesn't mutate observable state.
    const QRhiD3D11NativeHandles *nh = resolveD3D11NativeHandles(rhi);
    return nh && nh->dev;
}

QVideoFrameTexturesHandlesUPtr QWindowsD3D11RhiTextureImporter::importTexture(
        QRhi &rhi, ID3D11Device *srcDevice, ID3D11DeviceContext *srcContext,
        const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize, UINT subresourceIndex)
{
    Q_ASSERT(isRhiBackendSupported(rhi));

    return importTextureD3D11(rhi, srcDevice, srcContext, texture, frameSize, subresourceIndex);
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

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
