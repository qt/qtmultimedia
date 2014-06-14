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
#include "qsgvideonode_i420.h"
#include <QtCore/qmutex.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgmaterial.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_I420::supportedPixelFormats(
                                        QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> formats;

    if (handleType == QAbstractVideoBuffer::NoHandle)
        formats << QVideoFrame::Format_YUV420P << QVideoFrame::Format_YV12;

    return formats;
}

QSGVideoNode *QSGVideoNodeFactory_I420::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QSGVideoNode_I420(format);

    return 0;
}


class QSGVideoMaterialShader_YUV420 : public QSGMaterialShader
{
public:
    void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

    virtual char const *const *attributeNames() const {
        static const char *names[] = {
            "qt_VertexPosition",
            "qt_VertexTexCoord",
            0
        };
        return names;
    }

protected:

    virtual const char *vertexShader() const {
        const char *shader =
        "uniform highp mat4 qt_Matrix;                      \n"
        "uniform highp float yWidth;                        \n"
        "uniform highp float uvWidth;                       \n"
        "attribute highp vec4 qt_VertexPosition;            \n"
        "attribute highp vec2 qt_VertexTexCoord;            \n"
        "varying highp vec2 yTexCoord;                      \n"
        "varying highp vec2 uvTexCoord;                     \n"
        "void main() {                                      \n"
        "    yTexCoord   = qt_VertexTexCoord * vec2(yWidth, 1);\n"
        "    uvTexCoord  = qt_VertexTexCoord * vec2(uvWidth, 1);\n"
        "    gl_Position = qt_Matrix * qt_VertexPosition;   \n"
        "}";
        return shader;
    }

    virtual const char *fragmentShader() const {
        static const char *shader =
        "uniform sampler2D yTexture;"
        "uniform sampler2D uTexture;"
        "uniform sampler2D vTexture;"
        "uniform mediump mat4 colorMatrix;"
        "uniform lowp float opacity;"
        ""
        "varying highp vec2 yTexCoord;"
        "varying highp vec2 uvTexCoord;"
        ""
        "void main()"
        "{"
        "    mediump float Y = texture2D(yTexture, yTexCoord).r;"
        "    mediump float U = texture2D(uTexture, uvTexCoord).r;"
        "    mediump float V = texture2D(vTexture, uvTexCoord).r;"
        "    mediump vec4 color = vec4(Y, U, V, 1.);"
        "    gl_FragColor = colorMatrix * color * opacity;"
        "}";
        return shader;
    }

    virtual void initialize() {
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_yWidth = program()->uniformLocation("yWidth");
        m_id_uvWidth = program()->uniformLocation("uvWidth");
        m_id_yTexture = program()->uniformLocation("yTexture");
        m_id_uTexture = program()->uniformLocation("uTexture");
        m_id_vTexture = program()->uniformLocation("vTexture");
        m_id_colorMatrix = program()->uniformLocation("colorMatrix");
        m_id_opacity = program()->uniformLocation("opacity");
    }

    int m_id_matrix;
    int m_id_yWidth;
    int m_id_uvWidth;
    int m_id_yTexture;
    int m_id_uTexture;
    int m_id_vTexture;
    int m_id_colorMatrix;
    int m_id_opacity;
};


class QSGVideoMaterial_YUV420 : public QSGMaterial
{
public:
    QSGVideoMaterial_YUV420(const QVideoSurfaceFormat &format);
    ~QSGVideoMaterial_YUV420();

    virtual QSGMaterialType *type() const {
        static QSGMaterialType theType;
        return &theType;
    }

    virtual QSGMaterialShader *createShader() const {
        return new QSGVideoMaterialShader_YUV420;
    }

    virtual int compare(const QSGMaterial *other) const {
        const QSGVideoMaterial_YUV420 *m = static_cast<const QSGVideoMaterial_YUV420 *>(other);
        int d = m_textureIds[0] - m->m_textureIds[0];
        if (d)
            return d;
        else if ((d = m_textureIds[1] - m->m_textureIds[1]) != 0)
            return d;
        else
            return m_textureIds[2] - m->m_textureIds[2];
    }

    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(m_opacity, qreal(1.0)) ? false : true);
    }

    void setCurrentFrame(const QVideoFrame &frame) {
        QMutexLocker lock(&m_frameMutex);
        m_frame = frame;
    }

    void bind();
    void bindTexture(int id, int w, int h, const uchar *bits);

    QVideoSurfaceFormat m_format;
    QSize m_textureSize;

    static const uint Num_Texture_IDs = 3;
    GLuint m_textureIds[Num_Texture_IDs];

    qreal m_opacity;
    GLfloat m_yWidth;
    GLfloat m_uvWidth;
    QMatrix4x4 m_colorMatrix;

    QVideoFrame m_frame;
    QMutex m_frameMutex;
};

QSGVideoMaterial_YUV420::QSGVideoMaterial_YUV420(const QVideoSurfaceFormat &format) :
    m_format(format),
    m_opacity(1.0),
    m_yWidth(1.0),
    m_uvWidth(1.0)
{
    memset(m_textureIds, 0, sizeof(m_textureIds));

    switch (format.yCbCrColorSpace()) {
    case QVideoSurfaceFormat::YCbCr_JPEG:
        m_colorMatrix = QMatrix4x4(
                    1.0f,  0.000f,  1.402f, -0.701f,
                    1.0f, -0.344f, -0.714f,  0.529f,
                    1.0f,  1.772f,  0.000f, -0.886f,
                    0.0f,  0.000f,  0.000f,  1.0000f);
        break;
    case QVideoSurfaceFormat::YCbCr_BT709:
    case QVideoSurfaceFormat::YCbCr_xvYCC709:
        m_colorMatrix = QMatrix4x4(
                    1.164f,  0.000f,  1.793f, -0.5727f,
                    1.164f, -0.534f, -0.213f,  0.3007f,
                    1.164f,  2.115f,  0.000f, -1.1302f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
        break;
    default: //BT 601:
        m_colorMatrix = QMatrix4x4(
                    1.164f,  0.000f,  1.596f, -0.8708f,
                    1.164f, -0.392f, -0.813f,  0.5296f,
                    1.164f,  2.017f,  0.000f, -1.081f,
                    0.0f,    0.000f,  0.000f,  1.0000f);
    }

    setFlag(Blending, false);
}

QSGVideoMaterial_YUV420::~QSGVideoMaterial_YUV420()
{
    if (!m_textureSize.isEmpty())
        glDeleteTextures(Num_Texture_IDs, m_textureIds);
}

void QSGVideoMaterial_YUV420::bind()
{
    QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();

    QMutexLocker lock(&m_frameMutex);
    if (m_frame.isValid()) {
        if (m_frame.map(QAbstractVideoBuffer::ReadOnly)) {
            int fw = m_frame.width();
            int fh = m_frame.height();

            // Frame has changed size, recreate textures...
            if (m_textureSize != m_frame.size()) {
                if (!m_textureSize.isEmpty())
                    glDeleteTextures(Num_Texture_IDs, m_textureIds);
                glGenTextures(Num_Texture_IDs, m_textureIds);
                m_textureSize = m_frame.size();
            }

            const uchar *bits = m_frame.bits();
            int yStride = m_frame.bytesPerLine();
            // The UV stride is usually half the Y stride and is 32-bit aligned.
            // However it's not always the case, at least on Windows where the
            // UV planes are sometimes not aligned.
            // We calculate the stride using the UV byte count to always
            // have a correct stride.
            int uvStride = (m_frame.mappedBytes() - yStride * fh) / fh;
            int offsetU = yStride * fh;
            int offsetV = yStride * fh + uvStride * fh / 2;

            m_yWidth = qreal(fw) / yStride;
            m_uvWidth = qreal(fw) /  (2 * uvStride);

            if (m_frame.pixelFormat() == QVideoFrame::Format_YV12)
                qSwap(offsetU, offsetV);

            GLint previousAlignment;
            glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            functions->glActiveTexture(GL_TEXTURE1);
            bindTexture(m_textureIds[1], uvStride, fh / 2, bits + offsetU);
            functions->glActiveTexture(GL_TEXTURE2);
            bindTexture(m_textureIds[2], uvStride, fh / 2, bits + offsetV);
            functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
            bindTexture(m_textureIds[0], yStride, fh, bits);

            glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);

            m_frame.unmap();
        }

        m_frame = QVideoFrame();
    } else {
        functions->glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
        functions->glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
        functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
        glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
    }
}

void QSGVideoMaterial_YUV420::bindTexture(int id, int w, int h, const uchar *bits)
{
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bits);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

QSGVideoNode_I420::QSGVideoNode_I420(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QSGVideoMaterial_YUV420(format);
    setMaterial(m_material);
}

QSGVideoNode_I420::~QSGVideoNode_I420()
{
}

void QSGVideoNode_I420::setCurrentFrame(const QVideoFrame &frame)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}


void QSGVideoMaterialShader_YUV420::updateState(const RenderState &state,
                                                QSGMaterial *newMaterial,
                                                QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    QSGVideoMaterial_YUV420 *mat = static_cast<QSGVideoMaterial_YUV420 *>(newMaterial);
    program()->setUniformValue(m_id_yTexture, 0);
    program()->setUniformValue(m_id_uTexture, 1);
    program()->setUniformValue(m_id_vTexture, 2);

    mat->bind();

    program()->setUniformValue(m_id_colorMatrix, mat->m_colorMatrix);
    program()->setUniformValue(m_id_yWidth, mat->m_yWidth);
    program()->setUniformValue(m_id_uvWidth, mat->m_uvWidth);
    if (state.isOpacityDirty()) {
        mat->m_opacity = state.opacity();
        program()->setUniformValue(m_id_opacity, GLfloat(mat->m_opacity));
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
}

QT_END_NAMESPACE
