// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11texturereadback_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {
Q_LOGGING_CATEGORY(lcD3D11TextureReadback, "qt.multimedia.windows.d3d11texturereadback");
}

QAbstractVideoBuffer::MapData
QWindowsD3D11TextureReadback::map(const ComPtr<ID3D11Device> &device,
                                  const ComPtr<ID3D11Texture2D> &texture, const QSize &frameSize)
{
    Q_ASSERT(device);
    Q_ASSERT(texture);

    if (m_mapped)
        return {};

    device->GetImmediateContext(&m_stagingCtx);

    CD3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    if (const HRESULT hr = device->CreateTexture2D(&desc, nullptr, &m_stagingTex); FAILED(hr)) {
        qCWarning(lcD3D11TextureReadback)
                << "CreateTexture2D failed:" << QSystemError::windowsComString(hr);
        return {};
    }

    m_stagingCtx->CopyResource(m_stagingTex.Get(), texture.Get());

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (const HRESULT hr = m_stagingCtx->Map(m_stagingTex.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        FAILED(hr)) {
        qCWarning(lcD3D11TextureReadback) << "Map failed:" << QSystemError::windowsComString(hr);
        m_stagingTex.Reset();
        m_stagingCtx.Reset();
        return {};
    }

    m_mapped = true;

    QAbstractVideoBuffer::MapData data;
    data.planeCount = 1;
    data.bytesPerLine[0] = static_cast<int>(mapped.RowPitch);
    data.data[0] = static_cast<uchar *>(mapped.pData);
    data.dataSize[0] = static_cast<int>(mapped.RowPitch) * frameSize.height();
    return data;
}

void QWindowsD3D11TextureReadback::unmap()
{
    if (!m_mapped)
        return;

    m_stagingCtx->Unmap(m_stagingTex.Get(), 0);
    m_stagingTex.Reset();
    m_stagingCtx.Reset();
    m_mapped = false;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
