/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Multimedia module.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef RGBFRAMEHELPER_H
#define RGBFRAMEHELPER_H

#include <QImage>
#include <QAbstractVideoBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>

/*
  Returns a QImage that wraps the given video frame.

  This is suitable only for QAbstractVideoBuffer::NoHandle frames with RGB (or BGR)
  data. YUV is not supported here.

  The QVideoFrame must be mapped and kept mapped as long as the wrapping QImage
  exists.

  As a convenience the function also supports frames with a handle type of
  QAbstractVideoBuffer::GLTextureHandle. This allows creating a system memory backed
  QVideoFrame containing the image data from an OpenGL texture. However, readback is a
  slow operation and may stall the GPU pipeline and should be avoided in production code.
*/
QImage imageWrapper(const QVideoFrame &frame)
{
#ifndef QT_NO_OPENGL
    if (frame.handleType() == QAbstractVideoBuffer::GLTextureHandle) {
        // Slow and inefficient path. Ideally what's on the GPU should remain on the GPU, instead of readbacks like this.
        QImage img(frame.width(), frame.height(), QImage::Format_RGBA8888);
        GLuint textureId = frame.handle().toUInt();
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        QOpenGLFunctions *f = ctx->functions();
        GLuint fbo;
        f->glGenFramebuffers(1, &fbo);
        GLuint prevFbo;
        f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint *) &prevFbo);
        f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
        f->glReadPixels(0, 0, frame.width(), frame.height(), GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
        f->glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
        return img;
    } else
#endif // QT_NO_OPENGL
    {
        if (!frame.isReadable()) {
            qWarning("imageFromVideoFrame: No mapped image data available for read");
            return QImage();
        }

        QImage::Format fmt = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
        if (fmt != QImage::Format_Invalid)
            return QImage(frame.bits(), frame.width(), frame.height(), fmt);

        qWarning("imageFromVideoFrame: No matching QImage format");
    }

    return QImage();
}

#ifndef QT_NO_OPENGL
class TextureBuffer : public QAbstractVideoBuffer
{
public:
    TextureBuffer(uint id) : QAbstractVideoBuffer(GLTextureHandle), m_id(id) { }
    MapMode mapMode() const { return NotMapped; }
    uchar *map(MapMode, int *, int *) { return 0; }
    void unmap() { }
    QVariant handle() const { return QVariant::fromValue<uint>(m_id); }

private:
    GLuint m_id;
};
#endif // QT_NO_OPENGL

/*
  Creates and returns a new video frame wrapping the OpenGL texture textureId. The size
  must be passed in size, together with the format of the underlying image data in
  format. When the texture originates from a QImage, use
  QVideoFrame::imageFormatFromPixelFormat() to get a suitable format. Ownership is not
  altered, the new QVideoFrame will not destroy the texture.
*/
QVideoFrame frameFromTexture(uint textureId, const QSize &size, QVideoFrame::PixelFormat format)
{
#ifndef QT_NO_OPENGL
    return QVideoFrame(new TextureBuffer(textureId), size, format);
#else
    return QVideoFrame();
#endif // QT_NO_OPENGL
}

#endif
