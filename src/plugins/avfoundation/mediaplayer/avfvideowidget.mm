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

#include "avfvideowidget.h"
#include <QtCore/QDebug>
#include <QtGui/QOpenGLShaderProgram>

QT_USE_NAMESPACE

AVFVideoWidget::AVFVideoWidget(QWidget *parent, const QGLFormat &format)
    : QGLWidget(format, parent)
    , m_textureId(0)
    , m_aspectRatioMode(Qt::KeepAspectRatio)
    , m_shaderProgram(0)
{
    setAutoFillBackground(false);
}

AVFVideoWidget::~AVFVideoWidget()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    delete m_shaderProgram;
}

void AVFVideoWidget::initializeGL()
{
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    m_shaderProgram = new QOpenGLShaderProgram;

    static const char *textureVertexProgram =
            "uniform highp mat4 matrix;\n"
            "attribute highp vec3 vertexCoordEntry;\n"
            "attribute highp vec2 textureCoordEntry;\n"
            "varying highp vec2 textureCoord;\n"
            "void main() {\n"
            "   textureCoord = textureCoordEntry;\n"
            "   gl_Position = matrix * vec4(vertexCoordEntry, 1);\n"
            "}\n";

    static const char *textureFragmentProgram =
            "uniform sampler2D texture;\n"
            "varying highp vec2 textureCoord;\n"
            "void main() {\n"
            "   gl_FragColor = texture2D(texture, textureCoord);\n"
            "}\n";

    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, textureVertexProgram);
    m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, textureFragmentProgram);
    m_shaderProgram->link();
}

void AVFVideoWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, GLsizei(w), GLsizei(h));
    updateGL();
}

void AVFVideoWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_textureId)
        return;

    QRect targetRect = displayRect();
    GLfloat x1 = targetRect.left();
    GLfloat x2 = targetRect.right();
    GLfloat y1 = targetRect.bottom();
    GLfloat y2 = targetRect.top();
    GLfloat zValue = 0;

    const GLfloat textureCoordinates[] = {
        0, 0,
        1, 0,
        1, 1,
        0, 1
    };

    const GLfloat vertexCoordinates[] = {
        x1, y1, zValue,
        x2, y1, zValue,
        x2, y2, zValue,
        x1, y2, zValue
    };

    //Set matrix to transfrom geometry values into gl coordinate space.
    m_transformMatrix.setToIdentity();
    m_transformMatrix.scale( 2.0f / size().width(), 2.0f / size().height() );
    m_transformMatrix.translate(-size().width() / 2.0f, -size().height() / 2.0f);

    m_shaderProgram->bind();

    m_vertexCoordEntry = m_shaderProgram->attributeLocation("vertexCoordEntry");
    m_textureCoordEntry = m_shaderProgram->attributeLocation("textureCoordEntry");
    m_matrixLocation = m_shaderProgram->uniformLocation("matrix");

    //attach the data!
    glEnableVertexAttribArray(m_vertexCoordEntry);
    glEnableVertexAttribArray(m_textureCoordEntry);

    glVertexAttribPointer(m_vertexCoordEntry, 3, GL_FLOAT, GL_FALSE, 0, vertexCoordinates);
    glVertexAttribPointer(m_textureCoordEntry, 2, GL_FLOAT, GL_FALSE, 0, textureCoordinates);
    m_shaderProgram->setUniformValue(m_matrixLocation, m_transformMatrix);

    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);

    glDisableVertexAttribArray(m_vertexCoordEntry);
    glDisableVertexAttribArray(m_textureCoordEntry);

    m_shaderProgram->release();
}

void AVFVideoWidget::setTexture(GLuint texture)
{
    m_textureId = texture;

    if (isVisible()) {
        makeCurrent();
        updateGL();
    }
}

QSize AVFVideoWidget::sizeHint() const
{
    return m_nativeSize;
}

void AVFVideoWidget::setNativeSize(const QSize &size)
{
    m_nativeSize = size;
}

void AVFVideoWidget::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (m_aspectRatioMode != mode) {
        m_aspectRatioMode = mode;
        update();
    }
}

QRect AVFVideoWidget::displayRect() const
{
    QRect displayRect = rect();

    if (m_aspectRatioMode == Qt::KeepAspectRatio) {
        QSize size = m_nativeSize;
        size.scale(displayRect.size(), Qt::KeepAspectRatio);

        displayRect = QRect(QPoint(0, 0), size);
        displayRect.moveCenter(rect().center());
    }
    return displayRect;
}
