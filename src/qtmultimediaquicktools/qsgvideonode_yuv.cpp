/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
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
#include "qsgvideonode_yuv.h"
#include <QtCore/qmutex.h>
#include <QtQuick/qsgtexturematerial.h>
#include <QtQuick/qsgmaterial.h>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>

QT_BEGIN_NAMESPACE

QList<QVideoFrame::PixelFormat> QSGVideoNodeFactory_YUV::supportedPixelFormats(
                                        QAbstractVideoBuffer::HandleType handleType) const
{
    QList<QVideoFrame::PixelFormat> formats;

    if (handleType == QAbstractVideoBuffer::NoHandle) {
        formats << QVideoFrame::Format_YUV420P << QVideoFrame::Format_YV12
                << QVideoFrame::Format_NV12 << QVideoFrame::Format_NV21;
    }

    return formats;
}

QSGVideoNode *QSGVideoNodeFactory_YUV::createNode(const QVideoSurfaceFormat &format)
{
    if (supportedPixelFormats(format.handleType()).contains(format.pixelFormat()))
        return new QSGVideoNode_YUV(format);

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

class QSGVideoMaterialShader_NV_12_21 : public QSGVideoMaterialShader_YUV420
{
public:
    QSGVideoMaterialShader_NV_12_21(bool isNV21) : m_isNV21(isNV21) {
    }

    virtual void updateState(const RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial);

protected:

    virtual const char *vertexShader() const {
        const char *shader =
        "uniform highp mat4 qt_Matrix;                         \n"
        "uniform highp float yWidth;                           \n"
        "attribute highp vec4 qt_VertexPosition;               \n"
        "attribute highp vec2 qt_VertexTexCoord;               \n"
        "varying highp vec2 yTexCoord;                         \n"
        "void main() {                                         \n"
        "    yTexCoord   = qt_VertexTexCoord * vec2(yWidth, 1);\n"
        "    gl_Position = qt_Matrix * qt_VertexPosition;      \n"
        "}";
        return shader;
    }

    virtual const char *fragmentShader() const {
        static const char *shaderNV12 =
            "uniform sampler2D yTexture;                                 \n"
            "uniform sampler2D uvTexture;                                \n"
            "uniform mediump mat4 colorMatrix;                           \n"
            "uniform lowp float opacity;                                 \n"
            "varying highp vec2 yTexCoord;                               \n"
            "void main()                                                 \n"
            "{                                                           \n"
            "    mediump float Y = texture2D(yTexture, yTexCoord).r;     \n"
            "    mediump vec2 UV = texture2D(uvTexture, yTexCoord).ra;   \n"
            "    mediump vec4 color = vec4(Y, UV.x, UV.y, 1.);           \n"
            "    gl_FragColor = colorMatrix * color * opacity;           \n"
            "}";

        static const char *shaderNV21 =
            "uniform sampler2D yTexture;                                 \n"
            "uniform sampler2D uvTexture;                                \n"
            "uniform mediump mat4 colorMatrix;                           \n"
            "uniform lowp float opacity;                                 \n"
            "varying highp vec2 yTexCoord;                               \n"
            "void main()                                                 \n"
            "{                                                           \n"
            "    mediump float Y = texture2D(yTexture, yTexCoord).r;     \n"
            "    mediump vec2 UV = texture2D(uvTexture, yTexCoord).ar;   \n"
            "    mediump vec4 color = vec4(Y, UV.x, UV.y, 1.);           \n"
            "    gl_FragColor = colorMatrix * color * opacity;           \n"
            "}";
        return m_isNV21 ? shaderNV21 : shaderNV12;
    }

    virtual void initialize() {
        m_id_yTexture = program()->uniformLocation("yTexture");
        m_id_uTexture = program()->uniformLocation("uvTexture");
        m_id_matrix = program()->uniformLocation("qt_Matrix");
        m_id_yWidth = program()->uniformLocation("yWidth");
        m_id_colorMatrix = program()->uniformLocation("colorMatrix");
        m_id_opacity = program()->uniformLocation("opacity");
    }

private:
    bool m_isNV21;
};


class QSGVideoMaterial_YUV : public QSGMaterial
{
public:
    QSGVideoMaterial_YUV(const QVideoSurfaceFormat &format);
    ~QSGVideoMaterial_YUV();

    bool isNV12_21() const {
        const QVideoFrame::PixelFormat pf = m_format.pixelFormat();
        return pf == QVideoFrame::Format_NV12 || pf == QVideoFrame::Format_NV21;
    }

    virtual QSGMaterialType *type() const {
        static QSGMaterialType theType;
        return &theType;
    }

    virtual QSGMaterialShader *createShader() const {
        const QVideoFrame::PixelFormat pf = m_format.pixelFormat();
        if (isNV12_21())
            return new QSGVideoMaterialShader_NV_12_21(pf == QVideoFrame::Format_NV21);

        return new QSGVideoMaterialShader_YUV420;
    }

    virtual int compare(const QSGMaterial *other) const {
        const QSGVideoMaterial_YUV *m = static_cast<const QSGVideoMaterial_YUV *>(other);
        int d = m_textureIds[0] - m->m_textureIds[0];
        if (d)
            return d;

        d = m_textureIds[1] - m->m_textureIds[1];

        if (m_textureIds.size() == 2 || d != 0)
            return d;

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
    void bindTexture(int id, int w, int h, const uchar *bits, GLenum format);

    QVideoSurfaceFormat m_format;
    QSize m_textureSize;

    QVector<GLuint> m_textureIds;

    qreal m_opacity;
    GLfloat m_yWidth;
    GLfloat m_uvWidth;
    QMatrix4x4 m_colorMatrix;

    QVideoFrame m_frame;
    QMutex m_frameMutex;
};

QSGVideoMaterial_YUV::QSGVideoMaterial_YUV(const QVideoSurfaceFormat &format) :
    m_format(format),
    m_opacity(1.0),
    m_yWidth(1.0),
    m_uvWidth(1.0)
{
    m_textureIds.resize(isNV12_21() ? 2 : 3);

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

QSGVideoMaterial_YUV::~QSGVideoMaterial_YUV()
{
    if (!m_textureSize.isEmpty()) {
        if (QOpenGLContext *current = QOpenGLContext::currentContext())
            current->functions()->glDeleteTextures(m_textureIds.size(), &m_textureIds[0]);
        else
            qWarning() << "QSGVideoMaterial_YUV: Cannot obtain GL context, unable to delete textures";
    }
}

void QSGVideoMaterial_YUV::bind()
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
                    functions->glDeleteTextures(m_textureIds.size(), &m_textureIds[0]);
                functions->glGenTextures(m_textureIds.size(), &m_textureIds[0]);
                m_textureSize = m_frame.size();
            }

            GLint previousAlignment;
            functions->glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
            functions->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            if (isNV12_21()) {
                const int y = 0;
                const int uv = 1;

                m_yWidth = qreal(fw) / m_frame.bytesPerLine(y);
                m_uvWidth = m_yWidth;

                functions->glActiveTexture(GL_TEXTURE1);
                bindTexture(m_textureIds[1], m_frame.bytesPerLine(uv) / 2, fh / 2, m_frame.bits(uv), GL_LUMINANCE_ALPHA);
                functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
                bindTexture(m_textureIds[0], m_frame.bytesPerLine(y), fh, m_frame.bits(y), GL_LUMINANCE);
            } else {
                const int y = 0;
                const int u = m_frame.pixelFormat() == QVideoFrame::Format_YUV420P ? 1 : 2;
                const int v = m_frame.pixelFormat() == QVideoFrame::Format_YUV420P ? 2 : 1;

                m_yWidth = qreal(fw) / m_frame.bytesPerLine(y);
                m_uvWidth = qreal(fw) / (2 * m_frame.bytesPerLine(u));

                functions->glActiveTexture(GL_TEXTURE1);
                bindTexture(m_textureIds[1], m_frame.bytesPerLine(u), fh / 2, m_frame.bits(u), GL_LUMINANCE);
                functions->glActiveTexture(GL_TEXTURE2);
                bindTexture(m_textureIds[2], m_frame.bytesPerLine(v), fh / 2, m_frame.bits(v), GL_LUMINANCE);
                functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
                bindTexture(m_textureIds[0], m_frame.bytesPerLine(y), fh, m_frame.bits(y), GL_LUMINANCE);
            }

            functions->glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
            m_frame.unmap();
        }

        m_frame = QVideoFrame();
    } else {
        functions->glActiveTexture(GL_TEXTURE1);
        functions->glBindTexture(GL_TEXTURE_2D, m_textureIds[1]);
        if (!isNV12_21()) {
            functions->glActiveTexture(GL_TEXTURE2);
            functions->glBindTexture(GL_TEXTURE_2D, m_textureIds[2]);
        }
        functions->glActiveTexture(GL_TEXTURE0); // Finish with 0 as default texture unit
        functions->glBindTexture(GL_TEXTURE_2D, m_textureIds[0]);
    }
}

void QSGVideoMaterial_YUV::bindTexture(int id, int w, int h, const uchar *bits, GLenum format)
{
    QOpenGLFunctions *functions = QOpenGLContext::currentContext()->functions();

    functions->glBindTexture(GL_TEXTURE_2D, id);
    functions->glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, bits);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    functions->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

QSGVideoNode_YUV::QSGVideoNode_YUV(const QVideoSurfaceFormat &format) :
    m_format(format)
{
    setFlag(QSGNode::OwnsMaterial);
    m_material = new QSGVideoMaterial_YUV(format);
    setMaterial(m_material);
}

QSGVideoNode_YUV::~QSGVideoNode_YUV()
{
}

void QSGVideoNode_YUV::setCurrentFrame(const QVideoFrame &frame, FrameFlags)
{
    m_material->setCurrentFrame(frame);
    markDirty(DirtyMaterial);
}

void QSGVideoMaterialShader_YUV420::updateState(const RenderState &state,
                                                QSGMaterial *newMaterial,
                                                QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    QSGVideoMaterial_YUV *mat = static_cast<QSGVideoMaterial_YUV *>(newMaterial);
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

void QSGVideoMaterialShader_NV_12_21::updateState(const RenderState &state,
                                                 QSGMaterial *newMaterial,
                                                 QSGMaterial *oldMaterial)
{
    Q_UNUSED(oldMaterial);

    QSGVideoMaterial_YUV *mat = static_cast<QSGVideoMaterial_YUV *>(newMaterial);
    program()->setUniformValue(m_id_yTexture, 0);
    program()->setUniformValue(m_id_uTexture, 1);

    mat->bind();

    program()->setUniformValue(m_id_colorMatrix, mat->m_colorMatrix);
    program()->setUniformValue(m_id_yWidth, mat->m_yWidth);
    if (state.isOpacityDirty()) {
        mat->m_opacity = state.opacity();
        program()->setUniformValue(m_id_opacity, GLfloat(mat->m_opacity));
    }
    if (state.isMatrixDirty())
        program()->setUniformValue(m_id_matrix, state.combinedMatrix());
}

QT_END_NAMESPACE
