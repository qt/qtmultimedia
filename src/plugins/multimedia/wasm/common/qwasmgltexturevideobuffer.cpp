// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmgltexturevideobuffer_p.h"
#include <private/qhwvideobuffer_p.h>

QT_BEGIN_NAMESPACE


QWasmGLTextureVideoBuffer::QWasmGLTextureVideoBuffer(QGlTextureHandle textureHandle,
                                                     const QSize &size)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle),
      m_videoFrameFormat(size, QVideoFrameFormat::Format_RGBA8888),
      m_glTextureHandle(std::move(textureHandle)),
      m_size(size)
{
}

QAbstractVideoBuffer::MapData QWasmGLTextureVideoBuffer::map(QVideoFrame::MapMode)
{
    return {};
}

void QWasmGLTextureVideoBuffer::unmap()
{
}

quint64 QWasmGLTextureVideoBuffer::textureHandle(QRhi &, int plane)
{
    if (plane != 0 || !m_glTextureHandle)
        return 0;
    return static_cast<quint64>(m_glTextureHandle.get());
}

QT_END_NAMESPACE
