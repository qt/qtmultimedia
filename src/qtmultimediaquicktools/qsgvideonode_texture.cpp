/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qsgvideonode_texture_p.h"
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgmaterial.h>
#include <QtCore/qmutex.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtMultimedia/private/qmediaopenglhelper_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_Texture::supportedPixelFormats(
                                        QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> pixelFormats;

#ifdef Q_OS_MACOS
    if (handleType == QAbstractVideoBuffer::GLTextureRectangleHandle) {
        pixelFormats.append(QVideoFrame::Format_BGR32);
        pixelFormats.append(QVideoFrame::Format_BGRA32);
    }
#endif

    if (handleType == QAbstractVideoBuffer::GLTextureHandle) {
        pixelFormats.append(QVideoFrame::Format_RGB565);
        pixelFormats.append(QVideoFrame::Format_RGB32);
        pixelFormats.append(QVideoFrame::Format_ARGB32);
        pixelFormats.append(QVideoFrame::Format_BGR32);
        pixelFormats.append(QVideoFrame::Format_BGRA32);
#if !QT_CONFIG(gpu_vivante)
        pixelFormats.append(QVideoFrame::Format_ABGR32);
#endif
    }

    return pixelFormats;
}

QSGVideoNode *QSGVideoNodeFactory_Texture::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QSGVideoNode_Texture(format);

    return 0;
}


class QSGVideoMaterialShader_Texture : public QSGMaterialShader
{
public:
    QSGVideoMaterialShader_Texture()
        : QSGMaterialShader()
    {
    }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    char const *const *attributeNames() const override {
        static const char *names[] = {
            "qt_VertexPosition",
            "qt_VertexTexCoord",
            0
        };
        return names;
    }

protected:
    void initialize() override {
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_Texture = program()->uniformLocation("rgbTexture");
        m_id_opacity = program()->uniformLocation("opacity");
    }

    int m_id_matrix;
    int m_id_Texture;
    int m_id_opacity;
};

class QSGVideoMaterialShader_Texture_2D : public QSGVideoMaterialShader_Texture
{
public:
    QSGVideoMaterialShader_Texture_2D()
    {
        setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qtmultimediaquicktools/shaders/monoplanarvideo.vert"));
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qtmultimediaquicktools/shaders/rgbvideo.frag"));
    }
};

class QSGVideoMaterialShader_Texture_2D_swizzle : public QSGVideoMaterialShader_Texture_2D
{
public:
    QSGVideoMaterialShader_Texture_2D_swizzle(bool hasAlpha)
        : m_hasAlpha(hasAlpha)
    {
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qtmultimediaquicktools/shaders/rgbvideo_swizzle.frag"));
    }

protected:
    void initialize() override {
        QSGVideoMaterialShader_Texture_2D::initialize();
        program()->setUniformValue(program()->uniformLocation("hasAlpha"), GLboolean(m_hasAlpha));
    }

    int m_hasAlpha;
};

class QSGVideoMaterial_Texture : public QSGMaterial
{
public:
    QSGVideoMaterial_Texture(const QVideoSurfaceFormat &format) :
        m_format(format),
        m_textureId(0),
        m_opacity(1.0)
    {
        setFlag(Blending, false);
    }

    ~QSGVideoMaterial_Texture()
    {
        m_frame = QVideoFrame();
    }

    QSGMaterialType *type() const override {
        static QSGMaterialType normalType, swizzleType;
        return needsSwizzling() ? &swizzleType : &normalType;
    }

    int compare(const QSGMaterial *other) const override {
        const QSGVideoMaterial_Texture *m = static_cast<const QSGVideoMaterial_Texture *>(other);

        if (!m_textureId)
            return 1;

        int diff = m_textureId - m->m_textureId;
        if (diff)
            return diff;

        diff = m_format.pixelFormat() - m->m_format.pixelFormat();
        if (diff)
            return diff;

        return (m_opacity > m->m_opacity) ? 1 : -1;
    }

    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(m_opacity, qreal(1.0)) ? false : true);
    }

    void setVideoFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
        m_textureSize = frame.size();
    }

    virtual void bind() = 0;

    QVideoFrame m_frame;
    QMutex m_frameMutex;
    QSize m_textureSize;
    QVideoSurfaceFormat m_format;
    GLuint m_textureId;
    qreal m_opacity;

protected:
    bool needsSwizzling() const {
        return !QMediaOpenGLHelper::isANGLE()
                && (m_format.pixelFormat() == QVideoFrame::Format_RGB32
                    || m_format.pixelFormat() == QVideoFrame::Format_ARGB32);
    }
};

class QSGVideoMaterial_Texture_2D : public QSGVideoMaterial_Texture
{
public:
    QSGVideoMaterial_Texture_2D(const QVideoSurfaceFormat &format) :
        QSGVideoMaterial_Texture(format)
    {
    }

    QSGMaterialShader *createShader() const override
    {
        const bool hasAlpha = m_format.pixelFormat() == QVideoFrame::Format_ARGB32;
        return needsSwizzling() ? new QSGVideoMaterialShader_Texture_2D_swizzle(hasAlpha)
                                : new QSGVideoMaterialShader_Texture_2D;
    }

    void bind() override
    {
        QMutexLocker lock(&m_frameMutex);
        if (m_frame.isValid()) {
            m_textureId = m_frame.handle().toUInt();
            QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
            functions->glBindTexture(GL_TEXTURE_2D, m_textureId);

            functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            m_textureId = 0;
        }
    }
};

#ifdef Q_OS_MACOS
class QSGVideoMaterialShader_Texture_Rectangle : public QSGVideoMaterialShader_Texture
{
public:
    QSGVideoMaterialShader_Texture_Rectangle()
    {
        setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qtmultimediaquicktools/shaders/rectsampler.vert"));
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qtmultimediaquicktools/shaders/rectsampler_rgb.frag"));
    }

    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override
    {
        QSGVideoMaterial_Texture *mat = static_cast<QSGVideoMaterial_Texture *>(newMaterial);
        QVector2D size(mat->m_textureSize.width(), mat->m_textureSize.height());
        program()->setUniformValue(m_id_videoSize, size);

        QSGVideoMaterialShader_Texture::updateState(state, newMaterial, oldMaterial);
    }

protected:
    void initialize() override
    {
        QSGVideoMaterialShader_Texture::initialize();
        m_id_videoSize = program()->uniformLocation("qt_videoSize");
    }

    int m_id_videoSize;
};

class QSGVideoMaterial_Texture_Rectangle : public QSGVideoMaterial_Texture
{
public:
    QSGVideoMaterial_Texture_Rectangle(const QVideoSurfaceFormat &format) :
        QSGVideoMaterial_Texture(format)
    {
    }

    QSGMaterialShader *createShader() const override
    {
        Q_ASSERT(!needsSwizzling());
        return new QSGVideoMaterialShader_Texture_Rectangle;
    }

    void bind() override
    {
        QMutexLocker lock(&m_frameMutex);
        if (m_frame.isValid()) {
            m_textureId = m_frame.handle().toUInt();
            QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();
            functions->glActiveTexture(GL_TEXTURE0);
            functions->glBindTexture(GL_TEXTURE_RECTANGLE, m_textureId);

            functions->glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            functions->glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            functions->glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            functions->glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            m_textureId = 0;
        }
    }
};
#endif

QSGVideoNode_Texture::QSGVideoNode_Texture(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);

#ifdef Q_OS_MACOS
    if (format.handleType() == QAbstractVideoBuffer::GLTextureRectangleHandle)
        m_material = new QSGVideoMaterial_Texture_Rectangle(format);
#endif

    if (!m_material)
        m_material = new QSGVideoMaterial_Texture_2D(format);

    setMaterial(m_material);
}

QSGVideoNode_Texture::~QSGVideoNode_Texture()
{
}

void QSGVideoNode_Texture::setCurrentFrame(const QVideoFrame &frame, FrameFlags)
{
    m_material->setVideoFrame(frame);
    markDirty(DirtyMaterial);
}

void QSGVideoMaterialShader_Texture::updateState(const RenderState &state,
                                                QSGMaterial *newMaterial,
                                                QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    QSGVideoMaterial_Texture *mat = static_cast<QSGVideoMaterial_Texture *>(newMaterial);
    program()->setUniformValue(m_id_Texture, 0);

    mat->bind();

    if (state.isOpacityDirty()) {
        mat->m_opacity = state.opacity();
        mat->updateBlending();
        program()->setUniformValue(m_id_opacity, GLfloat(mat->m_opacity));
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
}

QT_END_NAMESPACE
