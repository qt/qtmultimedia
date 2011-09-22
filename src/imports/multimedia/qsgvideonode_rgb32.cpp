/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qsgvideonode_rgb32.h"

#include <QtDeclarative/qsgtexture.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_RGB32::supportedPixelFormats(
                                    QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> formats;

    if (handleType == QAbstractVideoBuffer::NoHandle)
        formats.append(QVideoFrame::Format_RGB32);

    return formats;
}

QSGVideoNode *QSGVideoNodeFactory_RGB32::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QSGVideoNode_RGB32();

    return 0;
}


class QSGVideoTexture_RGB32 : public QSGTexture
{
public:
    QSGVideoTexture_RGB32();

    int textureId() const { return m_textureId; }
    QSize textureSize() const { return m_size; }
    bool hasAlphaChannel() const { return false; }
    bool hasMipmaps() const { return false; }

    void setCurrentFrame(const QVideoFrame &frame) { m_frame = frame; }

    //QRectF textureSubRect() const;

    void bind();

private:
    QVideoFrame m_frame;
    GLuint m_textureId;
    QSize m_size;
};

QSGVideoTexture_RGB32::QSGVideoTexture_RGB32()
    : QSGTexture()
    , m_textureId(0)
{
}

void QSGVideoTexture_RGB32::bind()
{
    if (m_frame.isValid()) {
        if (m_size != m_frame.size()) {
            if (m_textureId)
                glDeleteTextures(1, &m_textureId);
            glGenTextures(1, &m_textureId);
            m_size = m_frame.size();
        }

        if (m_frame.map(QAbstractVideoBuffer::ReadOnly)) {
            QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
            const uchar *bits = m_frame.bits();
            functions->glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, m_textureId);
#ifdef QT_OPENGL_ES
            qWarning() << "RGB video doesn't work on GL ES\n";
#else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                         m_size.width(), m_size.height(),
                         0, GL_BGRA, GL_UNSIGNED_BYTE, bits);
#endif
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            m_frame.unmap();
        }
        m_frame = QVideoFrame();
        updateBindOptions(true);
    } else {
        glBindTexture(GL_TEXTURE_2D, m_textureId);
        updateBindOptions(false);
    }
}

QSGVideoNode_RGB32::QSGVideoNode_RGB32()
{
    setMaterial(&m_material);
    m_texture = new QSGVideoTexture_RGB32();

    m_material.setTexture(m_texture);
    m_material.setFiltering(QSGTexture::Linear);
}


void QSGVideoNode_RGB32::setCurrentFrame(const QVideoFrame &frame)
{
    m_texture->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}
