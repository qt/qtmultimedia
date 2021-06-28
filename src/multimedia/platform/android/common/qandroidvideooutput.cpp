/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qandroidvideooutput_p.h"

#include "androidsurfacetexture_p.h"
#include <qvideosink.h>
#include "private/qabstractvideobuffer_p.h"
#include "private/qrhi_p.h"
#include "private/qrhigles2_p.h"
#include <QVideoFrameFormat>
#include <qvideosink.h>
#include <QtCore/qcoreapplication.h>

#include <qevent.h>
#include <qcoreapplication.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>
#include <qopenglshaderprogram.h>
#include <qopenglframebufferobject.h>
#include <QtGui/QWindow>
#include <QtGui/QOffscreenSurface>

QT_BEGIN_NAMESPACE

static const GLfloat g_vertex_data[] = {
    -1.f, 1.f,
    1.f, 1.f,
    1.f, -1.f,
    -1.f, -1.f
};

static const GLfloat g_texture_data[] = {
    0.f, 0.f,
    1.f, 0.f,
    1.f, 1.f,
    0.f, 1.f
};

void OpenGLResourcesDeleter::deleteTextureHelper(quint32 id)
{
    if (id != 0)
        glDeleteTextures(1, &id);
}

void OpenGLResourcesDeleter::deleteFboHelper(void *fbo)
{
    delete reinterpret_cast<QOpenGLFramebufferObject *>(fbo);
}

void OpenGLResourcesDeleter::deleteShaderProgramHelper(void *prog)
{
    delete reinterpret_cast<QOpenGLShaderProgram *>(prog);
}

void OpenGLResourcesDeleter::deleteThisHelper()
{
    delete this;
}

class AndroidTextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    AndroidTextureVideoBuffer(QAndroidTextureVideoOutput *output, const QSize &size)
        : QAbstractVideoBuffer(QVideoFrame::RhiTextureHandle)
        , m_output(output)
        , m_size(size)
    {
    }

    virtual ~AndroidTextureVideoBuffer() {}

    QVideoFrame::MapMode mapMode() const override { return m_mapMode; }

    MapData map(QVideoFrame::MapMode mode) override
    {
        MapData mapData;
        if (m_mapMode == QVideoFrame::NotMapped && mode == QVideoFrame::ReadOnly && updateFrame()) {
            m_mapMode = mode;
            m_image = m_output->m_fbo->toImage();

            mapData.nBytes = static_cast<int>(m_image.sizeInBytes());
            mapData.nPlanes = 1;
            mapData.bytesPerLine[0] = m_image.bytesPerLine();
            mapData.data[0] = m_image.bits();
        }

        return mapData;
    }

    void unmap() override
    {
        m_image = QImage();
        m_mapMode = QVideoFrame::NotMapped;
    }

    quint64 textureHandle(int plane) const override
    {
        if (plane != 0)
            return 0;
        AndroidTextureVideoBuffer *that = const_cast<AndroidTextureVideoBuffer*>(this);
        if (!that->updateFrame())
            return 0;

        return m_output->m_fbo->texture();
    }

private:
    bool updateFrame()
    {
        // Even though the texture was updated in a previous call, we need to re-check
        // that this has not become a stale buffer, e.g., if the output size changed or
        // has since became invalid.
        if (!m_output->m_nativeSize.isValid())
            return false;

        // Size changed
        if (m_output->m_nativeSize != m_size)
            return false;

        // In the unlikely event that we don't have a valid fbo, but have a valid size,
        // force an update.
        const bool forceUpdate = !m_output->m_fbo;

        if (m_textureUpdated && !forceUpdate)
            return true;

        // update the video texture (called from the render thread)
        return (m_textureUpdated = m_output->renderFrameToFbo());
    }

    QVideoFrame::MapMode m_mapMode = QVideoFrame::NotMapped;
    QAndroidTextureVideoOutput *m_output = nullptr;
    QImage m_image;
    QSize m_size;
    bool m_textureUpdated = false;
};

QAndroidTextureVideoOutput::QAndroidTextureVideoOutput(QObject *parent)
    : QAndroidVideoOutput(parent)
{

}

QAndroidTextureVideoOutput::~QAndroidTextureVideoOutput()
{
    delete m_offscreenSurface;
    delete m_glContext;
    clearSurfaceTexture();

    if (m_glDeleter) { // Make sure all of these are deleted on the render thread.
        m_glDeleter->deleteFbo(m_fbo);
        m_glDeleter->deleteShaderProgram(m_program);
        m_glDeleter->deleteTexture(m_externalTex);
        m_glDeleter->deleteThis();
    }
}

QVideoSink *QAndroidTextureVideoOutput::surface() const
{
    return m_sink;
}

void QAndroidTextureVideoOutput::setSurface(QVideoSink *surface)
{
    if (surface == m_sink)
        return;

    m_sink = surface;
}

bool QAndroidTextureVideoOutput::isReady()
{
    return QOpenGLContext::currentContext() || m_externalTex;
}

void QAndroidTextureVideoOutput::initSurfaceTexture()
{
    if (m_surfaceTexture)
        return;

    if (!m_sink)
        return;

    QMutexLocker locker(&m_mutex);

    m_surfaceTexture = new AndroidSurfaceTexture(m_externalTex);

    if (m_surfaceTexture->surfaceTexture() != 0) {
        connect(m_surfaceTexture, SIGNAL(frameAvailable()), this, SLOT(onFrameAvailable()));
    } else {
        delete m_surfaceTexture;
        m_surfaceTexture = nullptr;
        if (m_glDeleter)
            m_glDeleter->deleteTexture(m_externalTex);
        m_externalTex = 0;
    }
}

void QAndroidTextureVideoOutput::clearSurfaceTexture()
{
    QMutexLocker locker(&m_mutex);
    if (m_surfaceTexture) {
        delete m_surfaceTexture;
        m_surfaceTexture = nullptr;
    }

    // Also reset the attached OpenGL texture
    if (m_glDeleter)
        m_glDeleter->deleteTexture(m_externalTex);
    m_externalTex = 0;
}

AndroidSurfaceTexture *QAndroidTextureVideoOutput::surfaceTexture()
{
    initSurfaceTexture();
    return m_surfaceTexture;
}

void QAndroidTextureVideoOutput::setVideoSize(const QSize &size)
{
     QMutexLocker locker(&m_mutex);
    if (m_nativeSize == size)
        return;

    stop();

    m_nativeSize = size;
}

void QAndroidTextureVideoOutput::stop()
{
    m_nativeSize = QSize();
}

void QAndroidTextureVideoOutput::reset()
{
    // flush pending frame
    if (m_sink)
        m_sink->newVideoFrame(QVideoFrame());

    clearSurfaceTexture();
}

void QAndroidTextureVideoOutput::onFrameAvailable()
{
    if (!m_nativeSize.isValid() || !m_sink)
        return;

    QAbstractVideoBuffer *buffer = new AndroidTextureVideoBuffer(this, m_nativeSize);
    QVideoFrame frame(buffer, QVideoFrameFormat(m_nativeSize, QVideoFrameFormat::Format_ARGB32_Premultiplied));

    m_sink->newVideoFrame(frame);
}

bool QAndroidTextureVideoOutput::renderFrameToFbo()
{
    QMutexLocker locker(&m_mutex);

    if (!m_nativeSize.isValid() || !m_surfaceTexture)
        return false;

    QOpenGLContext *shareContext = nullptr;
    auto rhi = m_sink ? m_sink->rhi() : nullptr;
    if (rhi && rhi->backend() == QRhi::OpenGLES2) {
        auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
        shareContext = nativeHandles->context;
    }

    // Make sure we have an OpenGL context to make current.
    if (shareContext || (!QOpenGLContext::currentContext() && !m_glContext)) {
        // Create Hidden QWindow surface to create context in this thread.
        m_offscreenSurface = new QWindow();
        m_offscreenSurface->setSurfaceType(QWindow::OpenGLSurface);
        // Needs geometry to be a valid surface, but size is not important.
        m_offscreenSurface->setGeometry(0, 0, 1, 1);
        m_offscreenSurface->create();
        m_offscreenSurface->moveToThread(m_sink->thread());

        // Create OpenGL context and set share context from surface.
        m_glContext = new QOpenGLContext();
        m_glContext->setFormat(m_offscreenSurface->requestedFormat());

        if (shareContext)
            m_glContext->setShareContext(shareContext);

        if (!m_glContext->create()) {
            qWarning("Failed to create QOpenGLContext");
            return false;
        }
    }

    if (m_glContext)
        m_glContext->makeCurrent(m_offscreenSurface);

    createGLResources();

    m_surfaceTexture->updateTexImage();

    // save current render states
    GLboolean stencilTestEnabled;
    GLboolean depthTestEnabled;
    GLboolean scissorTestEnabled;
    GLboolean blendEnabled;
    glGetBooleanv(GL_STENCIL_TEST, &stencilTestEnabled);
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glGetBooleanv(GL_SCISSOR_TEST, &scissorTestEnabled);
    glGetBooleanv(GL_BLEND, &blendEnabled);

    if (stencilTestEnabled)
        glDisable(GL_STENCIL_TEST);
    if (depthTestEnabled)
        glDisable(GL_DEPTH_TEST);
    if (scissorTestEnabled)
        glDisable(GL_SCISSOR_TEST);
    if (blendEnabled)
        glDisable(GL_BLEND);

    m_fbo->bind();

    glViewport(0, 0, m_nativeSize.width(), m_nativeSize.height());

    m_program->bind();
    m_program->enableAttributeArray(0);
    m_program->enableAttributeArray(1);
    m_program->setUniformValue("frameTexture", GLuint(0));
    m_program->setUniformValue("texMatrix", m_surfaceTexture->getTransformMatrix());

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, g_vertex_data);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, g_texture_data);

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    m_program->disableAttributeArray(0);
    m_program->disableAttributeArray(1);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    m_fbo->release();

    // restore render states
    if (stencilTestEnabled)
        glEnable(GL_STENCIL_TEST);
    if (depthTestEnabled)
        glEnable(GL_DEPTH_TEST);
    if (scissorTestEnabled)
        glEnable(GL_SCISSOR_TEST);
    if (blendEnabled)
        glEnable(GL_BLEND);

    return true;
}

void QAndroidTextureVideoOutput::createGLResources()
{
    Q_ASSERT(QOpenGLContext::currentContext() != NULL);

    if (!m_glDeleter)
        m_glDeleter = new OpenGLResourcesDeleter;

    if (!m_externalTex) {
        m_surfaceTexture->detachFromGLContext();
        glGenTextures(1, &m_externalTex);
        m_surfaceTexture->attachToGLContext(m_externalTex);
    }

    if (!m_fbo || m_fbo->size() != m_nativeSize) {
        delete m_fbo;
        m_fbo = new QOpenGLFramebufferObject(m_nativeSize);
    }

    if (!m_program) {
        m_program = new QOpenGLShaderProgram;

        QOpenGLShader *vertexShader = new QOpenGLShader(QOpenGLShader::Vertex, m_program);
        vertexShader->compileSourceCode("attribute highp vec4 vertexCoordsArray; \n" \
                                        "attribute highp vec2 textureCoordArray; \n" \
                                        "uniform   highp mat4 texMatrix; \n" \
                                        "varying   highp vec2 textureCoords; \n" \
                                        "void main(void) \n" \
                                        "{ \n" \
                                        "    gl_Position = vertexCoordsArray; \n" \
                                        "    textureCoords = (texMatrix * vec4(textureCoordArray, 0.0, 1.0)).xy; \n" \
                                        "}\n");
        m_program->addShader(vertexShader);

        QOpenGLShader *fragmentShader = new QOpenGLShader(QOpenGLShader::Fragment, m_program);
        fragmentShader->compileSourceCode("#extension GL_OES_EGL_image_external : require \n" \
                                          "varying highp vec2         textureCoords; \n" \
                                          "uniform samplerExternalOES frameTexture; \n" \
                                          "void main() \n" \
                                          "{ \n" \
                                          "    gl_FragColor = texture2D(frameTexture, textureCoords); \n" \
                                          "}\n");
        m_program->addShader(fragmentShader);

        m_program->bindAttributeLocation("vertexCoordsArray", 0);
        m_program->bindAttributeLocation("textureCoordArray", 1);
        m_program->link();
    }
}

QT_END_NAMESPACE
