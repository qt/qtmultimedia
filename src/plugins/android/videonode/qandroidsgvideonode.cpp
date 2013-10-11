/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidsgvideonode.h"

#include <qsgmaterial.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QAndroidSGVideoNodeMaterialShader : public QSGMaterialShader
{
public:
    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    char const *const *attributeNames() const {
        static const char *names[] = {
            "qt_VertexPosition",
            "qt_VertexTexCoord",
            0
        };
        return names;
    }

protected:
    const char *vertexShader() const {
        return
                "uniform highp mat4 qt_Matrix;                                          \n"
                "uniform highp mat4 texMatrix;                                          \n"
                "attribute highp vec4 qt_VertexPosition;                                \n"
                "attribute highp vec2 qt_VertexTexCoord;                                \n"
                "varying highp vec2 qt_TexCoord;                                        \n"
                "void main() {                                                          \n"
                "    qt_TexCoord = (texMatrix * vec4(qt_VertexTexCoord, 0.0, 1.0)).xy;  \n"
                "    gl_Position = qt_Matrix * qt_VertexPosition;                       \n"
                "}";
    }

    const char *fragmentShader() const {
        return
                "#extension GL_OES_EGL_image_external : require                      \n"
                "uniform samplerExternalOES videoTexture;                            \n"
                "uniform lowp float opacity;                                         \n"
                "varying highp vec2 qt_TexCoord;                                     \n"
                "void main()                                                         \n"
                "{                                                                   \n"
                "    gl_FragColor = texture2D(videoTexture, qt_TexCoord) * opacity;  \n"
                "}";
    }

    void initialize() {
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_texMatrix = program()->uniformLocation("texMatrix");
        m_id_texture = program()->uniformLocation("videoTexture");
        m_id_opacity = program()->uniformLocation("opacity");
    }

    int m_id_matrix;
    int m_id_texMatrix;
    int m_id_texture;
    int m_id_opacity;
};

class QAndroidSGVideoNodeMaterial : public QSGMaterial
{
public:
    QAndroidSGVideoNodeMaterial()
        : m_textureId(0)
    {
        setFlag(Blending, false);
    }

    ~QAndroidSGVideoNodeMaterial()
    {
        m_frame = QVideoFrame();
    }

    QSGMaterialType *type() const {
        static QSGMaterialType theType;
        return &theType;
    }

    QSGMaterialShader *createShader() const {
        return new QAndroidSGVideoNodeMaterialShader;
    }

    int compare(const QSGMaterial *other) const {
        const QAndroidSGVideoNodeMaterial *m = static_cast<const QAndroidSGVideoNodeMaterial *>(other);
        return m_textureId - m->m_textureId;
    }

    void setVideoFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    bool updateTexture()
    {
        QMutexLocker lock(&m_frameMutex);
        bool texMatrixDirty = false;

        if (m_frame.isValid()) {
            QVariantList list = m_frame.handle().toList();

            GLuint texId = list.at(0).toUInt();
            QMatrix4x4 mat = qvariant_cast<QMatrix4x4>(list.at(1));

            texMatrixDirty = texId != m_textureId || mat != m_texMatrix;

            m_textureId = texId;
            m_texMatrix = mat;

            // the texture is already bound and initialized at this point,
            // no need to call glTexParams

        } else {
            m_textureId = 0;
        }

        return texMatrixDirty;
    }

    QVideoFrame m_frame;
    QMutex m_frameMutex;
    GLuint m_textureId;
    QMatrix4x4 m_texMatrix;
};

void QAndroidSGVideoNodeMaterialShader::updateState(const RenderState &state,
                                                    QSGMaterial *newMaterial,
                                                    QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);
    QAndroidSGVideoNodeMaterial *mat = static_cast<QAndroidSGVideoNodeMaterial *>(newMaterial);
    program()->setUniformValue(m_id_texture, 0);

    if (mat->updateTexture())
        program()->setUniformValue(m_id_texMatrix, mat->m_texMatrix);

    if (state.isOpacityDirty())
        program()->setUniformValue(m_id_opacity, state.opacity());

    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
}

QAndroidSGVideoNode::QAndroidSGVideoNode(const QVideoSurfaceFormat &format)
    : m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QAndroidSGVideoNodeMaterial;
    setMaterial(m_material);
}

void QAndroidSGVideoNode::setCurrentFrame(const QVideoFrame &frame)
{
    m_material->setVideoFrame(frame);
    markDirty(DirtyMaterial);
}

QVideoFrame::PixelFormat QAndroidSGVideoNode::pixelFormat() const
{
    return m_format.pixelFormat();
}

QT_END_NAMESPACE
