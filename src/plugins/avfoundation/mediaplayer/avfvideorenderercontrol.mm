/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfvideorenderercontrol.h"
#include "avfdisplaylink.h"

#include "avfvideoframerenderer.h"

#include <QtMultimedia/qabstractvideobuffer.h>
#include <QtMultimedia/qabstractvideosurface.h>
#include <QtMultimedia/qvideosurfaceformat.h>

#include <private/qimagevideobuffer_p.h>

#include <QtCore/qdebug.h>

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

class TextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    TextureVideoBuffer(GLuint texture, QAbstractVideoBuffer::HandleType type)
        : QAbstractVideoBuffer(type)
        , m_texture(texture)
    {}

    MapMode mapMode() const { return NotMapped; }
    uchar *map(MapMode, int*, int*) { return nullptr; }
    void unmap() {}

    QVariant handle() const
    {
        return QVariant::fromValue<unsigned int>(m_texture);
    }

private:
    GLuint m_texture;
};

class CoreVideoTextureVideoBuffer : public TextureVideoBuffer
{
public:
    CoreVideoTextureVideoBuffer(CVOGLTextureRef texture, QAbstractVideoBuffer::HandleType type)
        : TextureVideoBuffer(CVOGLTextureGetName(texture), type)
        , m_coreVideoTexture(texture)
    {}

    virtual ~CoreVideoTextureVideoBuffer()
    {
        // absolutely critical that we drop this
        // reference of textures will stay in the cache
        CFRelease(m_coreVideoTexture);
    }

private:
    CVOGLTextureRef m_coreVideoTexture;
};


AVFVideoRendererControl::AVFVideoRendererControl(QObject *parent)
    : QVideoRendererControl(parent)
    , m_surface(nullptr)
    , m_playerLayer(nullptr)
    , m_frameRenderer(nullptr)
{
    m_displayLink = new AVFDisplayLink(this);
    connect(m_displayLink, SIGNAL(tick(CVTimeStamp)), SLOT(updateVideoFrame(CVTimeStamp)));
}

AVFVideoRendererControl::~AVFVideoRendererControl()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    m_displayLink->stop();
    [static_cast<AVPlayerLayer*>(m_playerLayer) release];
}

QAbstractVideoSurface *AVFVideoRendererControl::surface() const
{
    return m_surface;
}

void AVFVideoRendererControl::setSurface(QAbstractVideoSurface *surface)
{
#ifdef QT_DEBUG_AVF
    qDebug() << "Set video surface" << surface;
#endif

    //When we have a valid surface, we can setup a frame renderer
    //and schedule surface updates with the display link.
    if (surface == m_surface)
        return;

    QMutexLocker locker(&m_mutex);

    if (m_surface && m_surface->isActive())
        m_surface->stop();

    m_surface = surface;

    //If the surface changed, then the current frame renderer is no longer valid
    delete m_frameRenderer;
    m_frameRenderer = nullptr;

    //If there is now no surface to render too
    if (m_surface == nullptr) {
        m_displayLink->stop();
        return;
    }

    //Surface changed, so we need a new frame renderer
    m_frameRenderer = new AVFVideoFrameRenderer(m_surface, this);

    auto updateSurfaceType = [this] {
        auto preferredOpenGLSurfaceTypes = {
#ifdef Q_OS_MACOS
            QAbstractVideoBuffer::GLTextureRectangleHandle, // GL_TEXTURE_RECTANGLE
#endif
            QAbstractVideoBuffer::GLTextureHandle // GL_TEXTURE_2D
        };

        for (auto surfaceType : preferredOpenGLSurfaceTypes) {
            auto supportedFormats = m_surface->supportedPixelFormats(surfaceType);
            if (supportedFormats.contains(QVideoFrame::Format_BGR32)) {
                m_surfaceType = surfaceType;
                return;
            }
            m_surfaceType = QAbstractVideoBuffer::NoHandle; // QImage
        }
    };
    updateSurfaceType();
    connect(m_surface, &QAbstractVideoSurface::supportedFormatsChanged, this, updateSurfaceType);

    //If we already have a layer, but changed surfaces start rendering again
    if (m_playerLayer && !m_displayLink->isActive()) {
        m_displayLink->start();
    }

}

void AVFVideoRendererControl::setLayer(void *playerLayer)
{
    if (m_playerLayer == playerLayer)
        return;

    [static_cast<AVPlayerLayer*>(m_playerLayer) release];

    m_playerLayer = [static_cast<AVPlayerLayer*>(playerLayer) retain];

    //If there is an active surface, make sure it has been stopped so that
    //we can update it's state with the new content.
    if (m_surface && m_surface->isActive())
        m_surface->stop();

    //If there is no layer to render, stop scheduling updates
    if (m_playerLayer == nullptr) {
        m_displayLink->stop();
        return;
    }

    setupVideoOutput();

    //If we now have both a valid surface and layer, start scheduling updates
    if (m_surface && !m_displayLink->isActive()) {
        m_displayLink->start();
    }
}

void AVFVideoRendererControl::updateVideoFrame(const CVTimeStamp &ts)
{
    Q_UNUSED(ts)

    AVPlayerLayer *playerLayer = static_cast<AVPlayerLayer*>(m_playerLayer);

    if (!playerLayer) {
        qWarning("updateVideoFrame called without AVPlayerLayer (which shouldn't happen");
        return;
    }

    if (!playerLayer.readyForDisplay)
        return;

    if (m_surfaceType == QAbstractVideoBuffer::GLTextureHandle
     || m_surfaceType == QAbstractVideoBuffer::GLTextureRectangleHandle) {
        QSize size;
        QAbstractVideoBuffer *buffer = nullptr;

#ifdef Q_OS_MACOS
        if (m_surfaceType == QAbstractVideoBuffer::GLTextureRectangleHandle) {
            // Render to GL_TEXTURE_RECTANGLE directly
            if (CVOGLTextureRef tex = m_frameRenderer->renderLayerToTexture(playerLayer, &size))
                buffer = new CoreVideoTextureVideoBuffer(tex, m_surfaceType);
        } else {
            // Render to GL_TEXTURE_2D via FBO
            if (GLuint tex = m_frameRenderer->renderLayerToFBO(playerLayer, &size))
                buffer = new TextureVideoBuffer(tex, m_surfaceType);
        }
#else
        Q_ASSERT(m_surfaceType != QAbstractVideoBuffer::GLTextureRectangleHandle);
        // Render to GL_TEXTURE_2D directly
        if (CVOGLTextureRef tex = m_frameRenderer->renderLayerToTexture(playerLayer, &size))
            buffer = new CoreVideoTextureVideoBuffer(tex, m_surfaceType);
#endif
        if (!buffer)
            return;

        QVideoFrame frame = QVideoFrame(buffer, size, QVideoFrame::Format_BGR32);

        if (m_surface && frame.isValid()) {
            if (m_surface->isActive() && m_surface->surfaceFormat().pixelFormat() != frame.pixelFormat())
                m_surface->stop();

            if (!m_surface->isActive()) {
                QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), m_surfaceType);
                format.setScanLineDirection(QVideoSurfaceFormat::TopToBottom);
                if (!m_surface->start(format)) {
                    //Surface doesn't support GLTextureHandle
                    qWarning("Failed to activate video surface");
                }
            }

            if (m_surface->isActive())
                m_surface->present(frame);
        }
    } else {
        //fallback to rendering frames to QImages
        QSize size;
        QImage frameData = m_frameRenderer->renderLayerToImage(playerLayer, &size);

        if (frameData.isNull()) {
            return;
        }

        QAbstractVideoBuffer *buffer = new QImageVideoBuffer(frameData);
        QVideoFrame frame = QVideoFrame(buffer, size, QVideoFrame::Format_ARGB32);
        if (m_surface && frame.isValid()) {
            if (m_surface->isActive() && m_surface->surfaceFormat().pixelFormat() != frame.pixelFormat())
                m_surface->stop();

            if (!m_surface->isActive()) {
                QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), m_surfaceType);

                if (!m_surface->start(format)) {
                    qWarning("Failed to activate video surface");
                }
            }

            if (m_surface->isActive())
                m_surface->present(frame);
        }

    }
}

void AVFVideoRendererControl::setupVideoOutput()
{
}
