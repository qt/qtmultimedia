// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmgltexturevideobuffer_p.h"
#include <private/qhwvideobuffer_p.h>

QT_BEGIN_NAMESPACE


QWasmGLTextureVideoBuffer::QWasmGLTextureVideoBuffer(QGlTextureHandle textureHandle,
                                                     const QSize &size,
                                                     EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext,
                                                     QRhi *rhi)
    : QHwVideoBuffer(QVideoFrame::RhiTextureHandle),
      m_glTextureHandle(std::move(textureHandle)),
      m_size(size),
      m_videoFrameFormat(size, QVideoFrameFormat::Format_RGBA8888),
      m_glContext(glContext),
      m_rhi(rhi),
      m_rhiThread(QThread::currentThread())
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
    // Ensure the context that owns this texture is current before
    // the caller binds or samples it.
    emscripten_webgl_make_context_current(m_glContext);
    return static_cast<quint64>(m_glTextureHandle.get());
}

QT_END_NAMESPACE
