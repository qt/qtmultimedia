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

#include <QtCore/private/qjni_p.h>
#include "jsurfacetextureholder.h"
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>
#include <qevent.h>
#include <qcoreapplication.h>
#include <qopenglcontext.h>
#include <qopenglfunctions.h>

QT_BEGIN_NAMESPACE

#define ExternalGLTextureHandle QAbstractVideoBuffer::HandleType(QAbstractVideoBuffer::UserHandle + 1)

TextureDeleter::~TextureDeleter()
{
    glDeleteTextures(1, &m_id);
}

class AndroidTextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    AndroidTextureVideoBuffer(JSurfaceTexture *surface)
        : QAbstractVideoBuffer(ExternalGLTextureHandle)
        , m_surfaceTexture(surface)
    {
    }

    virtual ~AndroidTextureVideoBuffer() {}

    MapMode mapMode() const { return NotMapped; }
    uchar *map(MapMode, int*, int*) { return 0; }
    void unmap() {}

    QVariant handle() const
    {
        if (m_data.isEmpty()) {
            // update the video texture (called from the render thread)
            m_surfaceTexture->updateTexImage();
            m_data << (uint)m_surfaceTexture->textureID() << m_surfaceTexture->getTransformMatrix();
        }

        return m_data;
    }

private:
    mutable JSurfaceTexture *m_surfaceTexture;
    mutable QVariantList m_data;
};

QAndroidVideoRendererControl::QAndroidVideoRendererControl(QObject *parent)
    : QVideoRendererControl(parent)
    , m_surface(0)
    , m_androidSurface(0)
    , m_surfaceTexture(0)
    , m_surfaceHolder(0)
    , m_externalTex(0)
    , m_textureDeleter(0)
{
}

QAndroidVideoRendererControl::~QAndroidVideoRendererControl()
{
    if (m_surfaceTexture) {
        m_surfaceTexture->callMethod<void>("release");
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
    if (m_textureDeleter)
        m_textureDeleter->deleteLater();
}

QAbstractVideoSurface *QAndroidVideoRendererControl::surface() const
{
    return m_surface;
}

void QAndroidVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
    if (surface == m_surface)
        return;

    if (m_surface) {
        if (m_surface->isActive())
            m_surface->stop();
        m_surface->setProperty("_q_GLThreadCallback", QVariant());
    }

    m_surface = surface;

    if (m_surface) {
        m_surface->setProperty("_q_GLThreadCallback",
                               QVariant::fromValue<QObject*>(this));
    }
}

bool QAndroidVideoRendererControl::isReady()
{
    return QOpenGLContext::currentContext() || m_externalTex;
}

bool QAndroidVideoRendererControl::initSurfaceTexture()
{
    if (m_surfaceTexture)
        return true;

    if (!m_surface)
        return false;

    // if we have an OpenGL context in the current thread, create a texture. Otherwise, wait
    // for the GL render thread to call us back to do it.
    if (QOpenGLContext::currentContext()) {
        glGenTextures(1, &m_externalTex);
        m_textureDeleter = new TextureDeleter(m_externalTex);
    } else if (!m_externalTex) {
        return false;
    }

    m_surfaceTexture = new JSurfaceTexture(m_externalTex);

    if (m_surfaceTexture->isValid()) {
        connect(m_surfaceTexture, SIGNAL(frameAvailable()), this, SLOT(onFrameAvailable()));
    } else {
        delete m_surfaceTexture;
        m_surfaceTexture = 0;
        m_textureDeleter->deleteLater();
        m_externalTex = 0;
        m_textureDeleter = 0;
    }

    return m_surfaceTexture != 0;
}

jobject QAndroidVideoRendererControl::surfaceHolder()
{
    if (!initSurfaceTexture())
        return 0;

    if (!m_surfaceHolder) {
        m_androidSurface = new QJNIObjectPrivate("android/view/Surface",
                                          "(Landroid/graphics/SurfaceTexture;)V",
                                          m_surfaceTexture->object());

        m_surfaceHolder = new JSurfaceTextureHolder(m_androidSurface->object());
    }

    return m_surfaceHolder->object();
}

jobject QAndroidVideoRendererControl::surfaceTexture()
{
    if (!initSurfaceTexture())
        return 0;

    return m_surfaceTexture->object();
}

void QAndroidVideoRendererControl::setVideoSize(const QSize &size)
{
    if (m_nativeSize == size)
        return;

    stop();

    m_nativeSize = size;
}

void QAndroidVideoRendererControl::stop()
{
    if (m_surface && m_surface->isActive())
        m_surface->stop();
    m_nativeSize = QSize();
}

void QAndroidVideoRendererControl::onFrameAvailable()
{
    if (!m_nativeSize.isValid() || !m_surface)
        return;

    QAbstractVideoBuffer *buffer = new AndroidTextureVideoBuffer(m_surfaceTexture);
    QVideoFrame frame(buffer, m_nativeSize, QVideoFrame::Format_BGR32);

    if (m_surface->isActive() && (m_surface->surfaceFormat().pixelFormat() != frame.pixelFormat()
                                  || m_surface->nativeResolution() != frame.size())) {
        m_surface->stop();
    }

    if (!m_surface->isActive()) {
        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), ExternalGLTextureHandle);
        format.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);

        m_surface->start(format);
    }

    if (m_surface->isActive())
        m_surface->present(frame);
}

void QAndroidVideoRendererControl::customEvent(QEvent *e)
{
    if (e->type() == QEvent::User) {
        // This is running in the render thread (OpenGL enabled)
        if (!m_externalTex) {
            glGenTextures(1, &m_externalTex);
            m_textureDeleter = new TextureDeleter(m_externalTex); // will be deleted in the correct thread
            emit readyChanged(true);
        }
    }
}

QT_END_NAMESPACE
