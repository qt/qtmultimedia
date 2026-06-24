// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMGLTEXTUREVIDEOBUFFER_H
#define QWASMGLTEXTUREVIDEOBUFFER_H

#include <QtGui/rhi/qrhi.h>
#include <QSize>
#include <QThread>

#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include "qvideoframe.h"

#include <QtCore/private/quniquehandle_p.h>

#include <emscripten/val.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

struct QGlTextureHandleTraits
{
    using Type = GLuint;
    static Type invalidValue() { return 0; }
    static bool close(Type handle)
    {
        glDeleteTextures(1, &handle);
        return true;
    }
};
using QGlTextureHandle = QUniqueHandle<QGlTextureHandleTraits>;

class QWasmGLTextureVideoBuffer : public QHwVideoBuffer
{
public:
    explicit QWasmGLTextureVideoBuffer(QGlTextureHandle textureHandle, const QSize &size,
                                       EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext, QRhi *rhi);

    QWasmGLTextureVideoBuffer::MapData map(QVideoFrame::MapMode) override;
    void unmap() override;

    QVideoFrameFormat format() const override { return m_videoFrameFormat; }
    quint64 textureHandle(QRhi &, int plane) override;

    // Must return the RHI that owns the WebGL context the texture lives in.
    // Without this, qImageFromVideoFrame() calls qEnsureThreadLocalRhi()
    // which creates a second WebGL context, corrupting the GL state for the
    // QQuick render loop that resumes after toImage() returns.
    QRhi *associatedCurrentThreadRhi() const override
    {
        return (QThread::currentThread() == m_rhiThread) ? m_rhi : nullptr;
    }

private:
    QGlTextureHandle m_glTextureHandle;
    QSize m_size;
    QVideoFrameFormat m_videoFrameFormat;
    // WebGL context the texture was created in
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_glContext;
    QRhi *m_rhi;
    QThread *m_rhiThread;
};

QT_END_NAMESPACE

#endif // QWASMGLTEXTUREVIDEOBUFFER_H
