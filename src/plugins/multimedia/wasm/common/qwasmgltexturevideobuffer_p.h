// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMGLTEXTUREVIDEOBUFFER_H
#define QWASMGLTEXTUREVIDEOBUFFER_H

#include <QtGui/rhi/qrhi.h>
#include <QSize>

#include <QtMultimedia/private/qhwvideobuffer_p.h>
#include "qvideoframe.h"

#include <QtCore/private/quniquehandle_p.h>

#include <emscripten/val.h>
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
    explicit QWasmGLTextureVideoBuffer(QGlTextureHandle textureHandle, const QSize &size);

    QWasmGLTextureVideoBuffer::MapData map(QVideoFrame::MapMode) override;
    void unmap() override;

    QVideoFrameFormat format() const override { return m_videoFrameFormat; }
    quint64 textureHandle(QRhi &, int plane) override;

private:
    QGlTextureHandle m_glTextureHandle;
    QSize m_size;
    QVideoFrameFormat m_videoFrameFormat;
};

QT_END_NAMESPACE

#endif // QWASMGLTEXTUREVIDEOBUFFER_H
