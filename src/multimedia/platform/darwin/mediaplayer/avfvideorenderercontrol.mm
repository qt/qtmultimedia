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

#include "avfvideorenderercontrol_p.h"
#include "avfdisplaylink_p.h"

#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
#include "avfvideoframerenderer_ios_p.h"
#else
#include "avfvideoframerenderer_p.h"
#endif

#include <private/qabstractvideobuffer_p.h>
#include <QtMultimedia/qabstractvideosurface.h>
#include <QtMultimedia/qvideosurfaceformat.h>

#include <private/qimagevideobuffer_p.h>
#include <private/avfvideosink_p.h>

#include <QtCore/qdebug.h>

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

class TextureVideoBuffer : public QAbstractVideoBuffer
{
public:
    TextureVideoBuffer(QVideoFrame::HandleType type, quint64 tex)
        : QAbstractVideoBuffer(type)
        , m_texture(tex)
    {}

    virtual ~TextureVideoBuffer()
    {
    }

    QVideoFrame::MapMode mapMode() const override { return QVideoFrame::NotMapped; }
    MapData map(QVideoFrame::MapMode /*mode*/) override { return {}; }
    void unmap() override {}

    QVariant handle() const override
    {
        return QVariant::fromValue<unsigned long long>(m_texture);
    }

private:
    quint64 m_texture;
};

AVFVideoRendererControl::AVFVideoRendererControl(QObject *parent)
    : QObject(parent)
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
}

void AVFVideoRendererControl::reconfigure()
{
#ifdef QT_DEBUG_AVF
    qDebug() << "reconfigure";
#endif
    if (!m_layer) {
        m_displayLink->stop();
        return;
    }

    QMutexLocker locker(&m_mutex);

    renderToNativeView(shouldRenderToWindow());

    if (rendersToWindow()) {
        if (m_frameRenderer) {
            delete m_frameRenderer;
            m_frameRenderer = nullptr;
        }
        m_displayLink->stop();
    } else {
        if (!m_frameRenderer)
            m_frameRenderer = new AVFVideoFrameRenderer(this);
#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
        if (m_layer)
            m_frameRenderer->setPlayerLayer(static_cast<AVPlayerLayer*>(m_layer));
#endif
        m_displayLink->start();
    }

    updateAspectRatio();
    nativeSizeChanged();
}

void AVFVideoRendererControl::updateVideoFrame(const CVTimeStamp &ts)
{
    Q_UNUSED(ts);

    auto type = graphicsType();
    Q_ASSERT(type != QVideoSink::NativeWindow);

    if (!m_sink)
        return;

    if (!m_layer) {
        qWarning("updateVideoFrame called without AVPlayerLayer (which shouldn't happen");
        return;
    }

    auto *layer = playerLayer();
    if (!layer.readyForDisplay || !m_frameRenderer)
        return;
    nativeSizeChanged();

    QVideoFrame frame;
    if (type == QVideoSink::Metal || type == QVideoSink::NativeTexture) {
        quint64 tex = m_frameRenderer->renderLayerToMTLTexture(layer);
        if (tex == 0)
            return;

        auto buffer = new TextureVideoBuffer(QVideoFrame::MTLTextureHandle, tex);
        frame = QVideoFrame(buffer, nativeSize(), QVideoSurfaceFormat::Format_BGR32);
        if (!frame.isValid())
            return;

        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), QVideoFrame::MTLTextureHandle);
#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
        format.setScanLineDirection(QVideoSurfaceFormat::TopToBottom);
#else
        format.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);
#endif
        // #### QVideoFrame needs the surfaceformat!
    } else if (type == QVideoSink::OpenGL) {
        quint64 tex = m_frameRenderer->renderLayerToTexture(layer);
        //Make sure we got a valid texture
        if (tex == 0)
            return;

        QAbstractVideoBuffer *buffer = new TextureVideoBuffer(QVideoFrame::GLTextureHandle, tex);
        frame = QVideoFrame(buffer, nativeSize(), QVideoSurfaceFormat::Format_BGR32);
        if (!frame.isValid())
            return;

        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), QVideoFrame::GLTextureHandle);
#if defined(Q_OS_IOS) || defined(Q_OS_TVOS)
        format.setScanLineDirection(QVideoSurfaceFormat::TopToBottom);
#else
        format.setScanLineDirection(QVideoSurfaceFormat::BottomToTop);
#endif
    } else {
        Q_ASSERT(type == QVideoSink::Memory);
        //fallback to rendering frames to QImages
        QImage frameData = m_frameRenderer->renderLayerToImage(layer);

        if (frameData.isNull())
            return;

        QAbstractVideoBuffer *buffer = new QImageVideoBuffer(frameData);
        frame = QVideoFrame(buffer, nativeSize(), QVideoSurfaceFormat::Format_ARGB32_Premultiplied);
        QVideoSurfaceFormat format(frame.size(), frame.pixelFormat(), QVideoFrame::NoHandle);
    }

    m_sink->videoSink()->newVideoFrame(frame);
}

void AVFVideoRendererControl::updateAspectRatio()
{
    if (!m_layer)
        return;

    AVLayerVideoGravity gravity = AVLayerVideoGravityResizeAspect;

    switch (aspectRatioMode()) {
    case Qt::IgnoreAspectRatio:
        gravity = AVLayerVideoGravityResize;
        break;
    case Qt::KeepAspectRatio:
        gravity = AVLayerVideoGravityResizeAspect;
        break;
    case Qt::KeepAspectRatioByExpanding:
        gravity = AVLayerVideoGravityResizeAspectFill;
        break;
    default:
        break;
    }
    playerLayer().videoGravity = gravity;
}

#include "moc_avfvideorenderercontrol_p.cpp"
