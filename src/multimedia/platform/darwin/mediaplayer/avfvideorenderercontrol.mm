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
#include <private/avfvideobuffer_p.h>

#include <private/qabstractvideobuffer_p.h>
#include <QtMultimedia/qvideosurfaceformat.h>

#include <private/qimagevideobuffer_p.h>
#include <private/avfvideosink_p.h>
#include <QtGui/private/qrhi_p.h>

#include <QtCore/qdebug.h>

#import <AVFoundation/AVFoundation.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>

QT_USE_NAMESPACE

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
        m_displayLink->stop();
    } else {
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
    if (!layer.readyForDisplay)
        return;
    nativeSizeChanged();

    QVideoFrame frame;
    size_t width, height;
    CVPixelBufferRef pixelBuffer = copyPixelBufferFromLayer(width, height);
    if (!pixelBuffer)
        return;
    AVFVideoBuffer *buffer = new AVFVideoBuffer(m_rhi, pixelBuffer);
    auto fmt = AVFVideoBuffer::fromCVPixelFormat(CVPixelBufferGetPixelFormatType(pixelBuffer));
//    qDebug() << "Got pixelbuffer with format" << fmt;
    CVPixelBufferRelease(pixelBuffer);

    QVideoSurfaceFormat format(QSize(width, height), fmt);

    frame = QVideoFrame(buffer, format);
    m_sink->videoSink()->newVideoFrame(frame);
}

static NSDictionary* const AVF_OUTPUT_SETTINGS = @{
        (NSString *)kCVPixelBufferPixelFormatTypeKey: @[
            @(kCVPixelFormatType_32BGRA),
// ### Add supported YUV formats
// This has problems when trying to get the textures from the pixel buffer in AVFVideoBuffer::textureHandle.
// Somehow this doesn't work correctly for at least the HDR format (even though it works when mapping the buffers
// and downloading to a metal texture through RHI). Needs more investigation.
//            @(kCVPixelFormatType_420YpCbCr8Planar),
//            @(kCVPixelFormatType_420YpCbCr8PlanarFullRange),
//            @(kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange),
//            @(kCVPixelFormatType_420YpCbCr10BiPlanarFullRange)
        ],
        (NSString *)kCVPixelBufferMetalCompatibilityKey: @true
};

CVPixelBufferRef AVFVideoRendererControl::copyPixelBufferFromLayer(size_t& width, size_t& height)
{
    AVPlayerLayer *layer = playerLayer();
    //Is layer valid
    if (!layer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferFromLayer: invalid layer");
#endif
        return nullptr;
    }

    if (!m_videoOutput) {
        m_videoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:AVF_OUTPUT_SETTINGS];
        [m_videoOutput setDelegate:nil queue:nil];
        AVPlayerItem * item = [[layer player] currentItem];
        [item addOutput:m_videoOutput];
    }

    CFTimeInterval currentCAFrameTime =  CACurrentMediaTime();
    CMTime currentCMFrameTime =  [m_videoOutput itemTimeForHostTime:currentCAFrameTime];
    // happens when buffering / loading
    if (CMTimeCompare(currentCMFrameTime, kCMTimeZero) < 0) {
        return nullptr;
    }

    CVPixelBufferRef pixelBuffer = [m_videoOutput copyPixelBufferForItemTime:currentCMFrameTime
                                                   itemTimeForDisplay:nil];
    if (!pixelBuffer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferForItemTime returned nil");
        CMTimeShow(currentCMFrameTime);
#endif
        return nullptr;
    }

    width = CVPixelBufferGetWidth(pixelBuffer);
    height = CVPixelBufferGetHeight(pixelBuffer);
//    auto f = CVPixelBufferGetPixelFormatType(pixelBuffer);
//    char fmt[5];
//    memcpy(fmt, &f, 4);
//    fmt[4] = 0;
//    qDebug() << "copyPixelBuffer" << f << fmt << width << height;
    return pixelBuffer;
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

void AVFVideoRendererControl::setRhi(QRhi *rhi)
{
    m_rhi = rhi;
}

#include "moc_avfvideorenderercontrol_p.cpp"
