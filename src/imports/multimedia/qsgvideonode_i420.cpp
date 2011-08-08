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
#include "qsgvideonode_i420.h"
#include <QtDeclarative/qsgtexturematerial.h>
#include <QtDeclarative/qsgmaterial.h>

#include <QtOpenGL/qglshaderprogram.h>

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
        "attribute highp vec4 qt_VertexPosition;            \n"
        "attribute highp vec2 qt_VertexTexCoord;            \n"
        "varying highp vec2 qt_TexCoord;                    \n"
        "void main() {                                      \n"
        "    qt_TexCoord = qt_VertexTexCoord;               \n"
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
        "varying highp vec2 qt_TexCoord;"
        ""
        "void main()"
        "{"
        "    mediump float Y = texture2D(yTexture, qt_TexCoord).r;"
        "    mediump float U = texture2D(uTexture, qt_TexCoord).r;"
        "    mediump float V = texture2D(vTexture, qt_TexCoord).r;"
        "    mediump vec4 color = vec4(Y, U, V, 1.);"
        "    gl_FragColor = colorMatrix * color * opacity;"
        "}";
        return shader;
    }

    virtual void initialize() {
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_yTexture = program()->uniformLocation("yTexture");
        m_id_uTexture = program()->uniformLocation("uTexture");
        m_id_vTexture = program()->uniformLocation("vTexture");
        m_id_colorMatrix = program()->uniformLocation("colorMatrix");
        m_id_opacity = program()->uniformLocation("opacity");
    }

    int m_id_matrix;
    int m_id_yTexture;
    int m_id_uTexture;
    int m_id_vTexture;
    int m_id_colorMatrix;
    int m_id_opacity;
};


class QSGVideoMaterial_YUV420 : public QSGMaterial
{
public:
    QSGVideoMaterial_YUV420(const QVideoSurfaceFormat &format)
    {
        switch (format.yCbCrColorSpace()) {
        case QVideoSurfaceFormat::YCbCr_JPEG:
            colorMatrix = QMatrix4x4(
                        1.0,  0.000,  1.402, -0.701,
                        1.0, -0.344, -0.714,  0.529,
                        1.0,  1.772,  0.000, -0.886,
                        0.0,  0.000,  0.000,  1.0000);
            break;
        case QVideoSurfaceFormat::YCbCr_BT709:
        case QVideoSurfaceFormat::YCbCr_xvYCC709:
            colorMatrix = QMatrix4x4(
                        1.164,  0.000,  1.793, -0.5727,
                        1.164, -0.534, -0.213,  0.3007,
                        1.164,  2.115,  0.000, -1.1302,
                        0.0,    0.000,  0.000,  1.0000);
            break;
        default: //BT 601:
            colorMatrix = QMatrix4x4(
                        1.164,  0.000,  1.596, -0.8708,
                        1.164, -0.392, -0.813,  0.5296,
                        1.164,  2.017,  0.000, -1.081,
                        0.0,    0.000,  0.000,  1.0000);
        }

        setFlag(Blending, false);
    }

    virtual QSGMaterialType *type() const {
        static QSGMaterialType theType;
        return &theType;
    }

    virtual QSGMaterialShader *createShader() const {
        return new QSGVideoMaterialShader_YUV420;
    }

    virtual int compare(const QSGMaterial *other) const {
        const QSGVideoMaterial_YUV420 *m = static_cast<const QSGVideoMaterial_YUV420 *>(other);
        int d = idY - m->idY;
        if (d)
            return d;
        else if ((d = idU - m->idU) != 0)
            return d;
        else
            return idV - m->idV;
    }

    void updateBlending() {
        setFlag(Blending, qFuzzyCompare(opacity, 1.0) ? false : true);
    }

    GLuint idY;
    GLuint idU;
    GLuint idV;
    qreal opacity;
    QMatrix4x4 colorMatrix;
};


QSGVideoNode_I420::QSGVideoNode_I420(const QVideoSurfaceFormat &format) :
    m_width(0),
    m_height(0),
    m_format(format)
{
    m_material = new QSGVideoMaterial_YUV420(format);
    setMaterial(m_material);
    m_material->opacity = 1;
}

QSGVideoNode_I420::~QSGVideoNode_I420()
{
    if (m_width != 0 && m_height != 0)
        glDeleteTextures(3, m_id);
}

void QSGVideoNode_I420::setCurrentFrame(const QVideoFrame &frame)
{
    m_frame = frame;

    m_frame.map(QAbstractVideoBuffer::ReadOnly);

    int fw = frame.width();
    int fh = frame.height();

    // Frame has changed size, recreate textures...
    if (fw != m_width || fh != m_height) {
        if (m_width != 0 && m_height != 0)
            glDeleteTextures(3, m_id);
        glGenTextures(3, m_id);
        m_width = fw;
        m_height = fh;

        m_material->idY = m_id[0];
        m_material->idU = m_id[1];
        m_material->idV = m_id[2];
    }

    const uchar *bits = frame.bits();
    int bpl = frame.bytesPerLine();
    int bpl2 = (bpl / 2 + 3) & ~3;
    int offsetU = bpl * fh;
    int offsetV = bpl * fh + bpl2 * fh / 2;

    if (m_frame.pixelFormat() == QVideoFrame::Format_YV12)
        qSwap(offsetU, offsetV);

    bindTexture(m_id[0], GL_TEXTURE0, fw, fh, bits);
    bindTexture(m_id[1], GL_TEXTURE1, fw/2, fh / 2, bits + offsetU);
    bindTexture(m_id[2], GL_TEXTURE2, fw/2, fh / 2, bits + offsetV);

    m_frame.unmap();

    markDirty(DirtyMaterial);
}

void QSGVideoNode_I420::bindTexture(int id, int unit, int w, int h, const uchar *bits)
{
    QGLFunctions *functions = QGLContext::currentContext()->functions();
    functions->glActiveTexture(unit);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, w, h, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, bits);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}


void QSGVideoMaterialShader_YUV420::updateState(const RenderState &state,
                                                QSGMaterial *newMaterial,
                                                QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    QGLFunctions *functions = state.context()->functions();
    QSGVideoMaterial_YUV420 *mat = static_cast<QSGVideoMaterial_YUV420 *>(newMaterial);
    program()->setUniformValue(m_id_yTexture, 0);
    program()->setUniformValue(m_id_uTexture, 1);
    program()->setUniformValue(m_id_vTexture, 2);

    functions->glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mat->idY);
    functions->glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mat->idU);
    functions->glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mat->idV);

    program()->setUniformValue(m_id_colorMatrix, mat->colorMatrix);
    if (state.isOpacityDirty()) {
        mat->opacity = state.opacity();
        program()->setUniformValue(m_id_opacity, GLfloat(mat->opacity));
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
}
