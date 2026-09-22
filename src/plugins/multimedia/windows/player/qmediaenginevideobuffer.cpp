// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaenginevideobuffer_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimedia/qvideoframeformat.h>

QT_BEGIN_NAMESPACE

QMediaEngineVideoBuffer::QMediaEngineVideoBuffer(
        ComPtr<ID3D11Texture2D> texture, ComPtr<ID3D11Device> engineDevice, QSize frameSize,
        std::shared_ptr<QtMultimediaPrivate::QWindowsD3D11RhiTextureImporter> importer)
    : m_texture(std::move(texture)),
      m_engineDevice(std::move(engineDevice)),
      m_frameSize(frameSize),
      m_importer(std::move(importer))
{
}

QMediaEngineVideoBuffer::~QMediaEngineVideoBuffer() = default;

QVideoFrameTexturesUPtr
QMediaEngineVideoBuffer::mapTextures(QRhi &rhi, QVideoFrameTexturesUPtr & /*oldTextures*/)
{
    if (!m_importer)
        return nullptr;

    ComPtr<ID3D11DeviceContext> engineCtx;
    m_engineDevice->GetImmediateContext(&engineCtx);

    QVideoFrameTexturesHandlesUPtr handles = m_importer->importTexture(
            rhi, m_engineDevice.Get(), engineCtx.Get(), m_texture, m_frameSize,
            QVideoFrameFormat::Format_BGRA8888);
    if (!handles)
        return nullptr;

    return QVideoTextureHelper::createTexturesFromHandles(
            std::move(handles), rhi, QVideoFrameFormat::Format_BGRA8888, m_frameSize);
}

QAbstractVideoBuffer::MapData QMediaEngineVideoBuffer::map(QVideoFrame::MapMode mode)
{
    if (mode != QVideoFrame::ReadOnly)
        return {};

    return m_readback.map(m_engineDevice, m_texture, m_frameSize);
}

void QMediaEngineVideoBuffer::unmap()
{
    m_readback.unmap();
}

QT_END_NAMESPACE
