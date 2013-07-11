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

#include "qandroidvideorendercontrol.h"

#include <QtPlatformSupport/private/qjnihelpers_p.h>
#include "jsurfacetextureholder.h"
#include <QAbstractVideoSurface>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLFramebufferObject>
#include <QVideoSurfaceFormat>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <qevent.h>

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

class TextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    TextureVideoBuffer(GLuint textureId)
        : QAbstractVideoBuffer(GLTextureHandle)
        , m_textureId(textureId)
    {}

    virtual ~TextureVideoBuffer() {}

    MapMode mapMode() const { return NotMapped; }
    uchar *map(MapMode, int*, int*) { return 0; }
    void unmap() {}

    QVariant handle() const
    {
        return QVariant::fromValue<unsigned int>(m_textureId);
    }

private:
    GLuint m_textureId;
};

class ImageVideoBuffer : public QAbstractVideoBuffer
{
public:
    ImageVideoBuffer(const QImage &image)
        : QAbstractVideoBuffer(NoHandle)
        , m_image(image)
        , m_mode(NotMapped)
    {

    }

    MapMode mapMode() const { return m_mode; }
    uchar *map(MapMode mode, int *, int *)
    {
        if (mode != NotMapped && m_mode == NotMapped) {
            m_mode = mode;
            return m_image.bits();
        }

        return 0;
    }

    void unmap()
    {
        m_mode = NotMapped;
    }

private:
    QImage m_image;
    MapMode m_mode;
};

QAndroidVideoRendererControl::QAndroidVideoRendererControl(QObject *parent)
    : QVideoRendererControl(parent)
    , m_surface(0)
    , m_offscreenSurface(0)
    , m_glContext(0)
    , m_fbo(0)
    , m_program(0)
    , m_useImage(false)
    , m_androidSurface(0)
    , m_surfaceTexture(0)
    , m_surfaceHolder(0)
    , m_externalTex(0)
    , m_textureReadyCallback(0)
    , m_textureReadyContext(0)
{
}

QAndroidVideoRendererControl::~QAndroidVideoRendererControl()
{
    if (m_glContext)
        m_glContext->makeCurrent(m_offscreenSurface);

    if (m_surfaceTexture) {
        QJNILocalRef<jobject> surfaceTex = m_surfaceTexture->surfaceTexture();
        QJNIObject obj(surfaceTex.object());
        obj.callMethod<void>("release");
        delete m_surfaceTexture;
        m_surfaceTexture = 0;
    }
    if (m_androidSurface) {
        m_androidSurface->callMethod<void>("release");
        delete m_androidSurface;
        m_androidSurface = 0;
    }
    if (m_surfaceHolder) {
        delete m_surfaceHolder;
        m_surfaceHolder = 0;
    }
    if (m_externalTex)
        glDeleteTextures(1, &m_externalTex);

    delete m_fbo;
    delete m_program;
    delete m_glContext;
    delete m_offscreenSurface;
}

QAbstractVideoSurface *QAndroidVideoRendererControl::surface() const
{
    return m_surface;
}

void QAndroidVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    if (surface == m_surface)
        return;

    if (m_surface && m_surface->isActive()) {
        m_surface->stop();
        m_surface->removeEventFilter(this);
    }

    m_surface = surface;

    if (m_surface) {
        m_useImage = !m_surface->supportedPixelFormats(QAbstractVideoBuffer::GLTextureHandle).contains(QVideoFrame::Format_BGR32);
        m_surface->installEventFilter(this);
    }
}

bool QAndroidVideoRendererControl::isTextureReady()
{
    return QOpenGLContext::currentContext() || (m_surface && m_surface->property("GLContext").isValid());
}

void QAndroidVideoRendererControl::setTextureReadyCallback(TextureReadyCallback cb, void *context)
{
    m_textureReadyCallback = cb;
    m_textureReadyContext = context;
}

bool QAndroidVideoRendererControl::initSurfaceTexture()
{
    if (m_surfaceTexture)
        return true;

    if (!m_surface)
        return false;

    QOpenGLContext *currContext = QOpenGLContext::currentContext();

    // If we don't have a GL context in the current thread, create one and share it
    // with the render thread GL context
    if (!currContext && !m_glContext) {
        QOpenGLContext *shareContext = qobject_cast<QOpenGLContext*>(m_surface->property("GLContext").value<QObject*>());
        if (!shareContext)
            return false;

        m_offscreenSurface = new QOffscreenSurface;
        QSurfaceFormat format;
        format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
        m_offscreenSurface->setFormat(format);
        m_offscreenSurface->create();

        m_glContext = new QOpenGLContext;
        m_glContext->setFormat(m_offscreenSurface->requestedFormat());

        if (shareContext)
            m_glContext->setShareContext(shareContext);

        if (!m_glContext->create()) {
            delete m_glContext;
            m_glContext = 0;
            delete m_offscreenSurface;
            m_offscreenSurface = 0;
            return false;
        }

        // if sharing contexts is not supported, fallback to image rendering and send the bits
        // to the video surface
        if (!m_glContext->shareContext())
            m_useImage = true;
    }

    if (m_glContext)
        m_glContext->makeCurrent(m_offscreenSurface);

    glGenTextures(1, &m_externalTex);
    m_surfaceTexture = new JSurfaceTexture(m_externalTex);

    if (m_surfaceTexture->isValid()) {
        connect(m_surfaceTexture, SIGNAL(frameAvailable()), this, SLOT(onFrameAvailable()));
    } else {
        delete m_surfaceTexture;
        m_surfaceTexture = 0;
        glDeleteTextures(1, &m_externalTex);
    }

    return m_surfaceTexture != 0;
}

jobject QAndroidVideoRendererControl::surfaceHolder()
{
    if (!initSurfaceTexture())
        return 0;

    if (!m_surfaceHolder) {
        QJNILocalRef<jobject> surfaceTex = m_surfaceTexture->surfaceTexture();

        m_androidSurface = new QJNIObject("android/view/Surface",
                                          "(Landroid/graphics/SurfaceTexture;)V",
                                          surfaceTex.object());

        m_surfaceHolder = new JSurfaceTextureHolder(m_androidSurface->object());
    }

    return m_surfaceHolder->object();
}

void QAndroidVideoRendererControl::setVideoSize(const QSize &size)
{
    if (m_nativeSize == size)
        return;

    m_nativeSize = size;

    delete m_fbo;
    m_fbo = 0;
}

void QAndroidVideoRendererControl::stop()
{
    if (m_surface && m_surface->isActive())
        m_surface->stop();
    m_nativeSize = QSize();
}

void QAndroidVideoRendererControl::onFrameAvailable()
{
    if (m_glContext)
        m_glContext->makeCurrent(m_offscreenSurface);

    m_surfaceTexture->updateTexImage();

    if (!m_nativeSize.isValid())
        return;

    renderFrameToFbo();

    QAbstractVideoBuffer *buffer = 0;
    QVideoFrame frame;

    if (m_useImage) {
        buffer = new ImageVideoBuffer(m_fbo->toImage().mirrored());
        frame = QVideoFrame(buffer, m_nativeSize, QVideoFrame::Format_RGB32);
    } else {
        buffer = new TextureVideoBuffer(m_fbo->texture());
        frame = QVideoFrame(buffer, m_nativeSize, QVideoFrame::Format_BGR32);
    }

    if (m_surface && frame.isValid()) {
        if (m_surface->isActive() && (m_surface->surfaceFormat().pixelFormat() != frame.pixelFormat()
                                      || m_surface->nativeResolution() != frame.size())) {
            m_surface->stop();
        }

        if (!m_surface->isActive()) {
            QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(),
                                       m_useImage ? QAbstractVideoBuffer::NoHandle
                                                  : QAbstractVideoBuffer::GLTextureHandle);

            m_surface->start(format);
        }

        if (m_surface->isActive())
            m_surface->present(frame);
    }
}

void QAndroidVideoRendererControl::renderFrameToFbo()
{
    createGLResources();

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
    m_program->release();

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
    m_fbo->release();

    glFinish();
}

void QAndroidVideoRendererControl::createGLResources()
{
    if (!m_fbo)
        m_fbo = new QOpenGLFramebufferObject(m_nativeSize);

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

bool QAndroidVideoRendererControl::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::DynamicPropertyChange) {
        QDynamicPropertyChangeEvent *event = static_cast<QDynamicPropertyChangeEvent*>(e);
        if (event->propertyName() == "GLContext" && m_textureReadyCallback) {
            m_textureReadyCallback(m_textureReadyContext);
            m_textureReadyCallback = 0;
            m_textureReadyContext = 0;
        }
    }

    return false;
}

QT_END_NAMESPACE
