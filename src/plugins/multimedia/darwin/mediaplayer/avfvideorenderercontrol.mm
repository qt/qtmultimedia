// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "avfvideorenderercontrol_p.h"
#include "avfdisplaylink_p.h"
#include <avfvideobuffer_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include "private/qvideoframe_p.h"

#include <QtMultimedia/qvideoframeformat.h>

#include <avfvideosink_p.h>
#include <rhi/qrhi.h>

#include <QtCore/qdebug.h>

#import <AVFoundation/AVFoundation.h>
#include <CoreVideo/CVPixelBuffer.h>
#include <CoreVideo/CVImageBuffer.h>

QT_USE_NAMESPACE

@interface SubtitleDelegate : NSObject <AVPlayerItemLegibleOutputPushDelegate>
{
    AVFVideoRendererControl *m_renderer;
}

- (void)legibleOutput:(AVPlayerItemLegibleOutput *)output
        didOutputAttributedStrings:(NSArray<NSAttributedString *> *)strings
        nativeSampleBuffers:(NSArray *)nativeSamples
        forItemTime:(CMTime)itemTime;

@end

@implementation SubtitleDelegate

-(id)initWithRenderer: (AVFVideoRendererControl *)renderer
{
    if (!(self = [super init]))
        return nil;

    m_renderer = renderer;

    return self;
}

- (void)legibleOutput:(AVPlayerItemLegibleOutput *)output
        didOutputAttributedStrings:(NSArray<NSAttributedString *> *)strings
        nativeSampleBuffers:(NSArray *)nativeSamples
        forItemTime:(CMTime)itemTime
{
    QString text;
    for (NSAttributedString *s : strings) {
        if (!text.isEmpty())
            text += QChar::LineSeparator;
        text += QString::fromNSString(s.string);
    }
    m_renderer->setSubtitleText(text);
}

@end


AVFVideoRendererControl::AVFVideoRendererControl(QObject *parent)
    : QObject(parent)
{
    m_displayLink = new AVFDisplayLink(this);

    connect(m_displayLink, &AVFDisplayLink::tick, this, &AVFVideoRendererControl::updateVideoFrame);
}

AVFVideoRendererControl::~AVFVideoRendererControl()
{
#ifdef QT_DEBUG_AVF
    qDebug() << Q_FUNC_INFO;
#endif
    m_displayLink->stop();
    m_displayLink->disconnect(this);
    if (m_videoOutput)
        [m_videoOutput release];
    if (m_subtitleOutput)
        [m_subtitleOutput release];
    if (m_subtitleDelegate)
        [m_subtitleDelegate release];
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

    m_displayLink->start();

    nativeSizeChanged();
}

void AVFVideoRendererControl::setLayer(CALayer *layer)
{
    if (m_layer == layer)
        return;

    AVPlayerLayer *plLayer = playerLayer();
    if (plLayer) {
        if (m_videoOutput)
            [[[plLayer player] currentItem] removeOutput:m_videoOutput];

        if (m_subtitleOutput)
            [[[plLayer player] currentItem] removeOutput:m_subtitleOutput];
    }

    if (!layer && m_sink)
        m_sink->setVideoFrame(QVideoFrame());

    AVFVideoSinkInterface::setLayer(layer);
}

void AVFVideoRendererControl::setVideoRotation(QtVideo::Rotation rotation)
{
    m_rotation = rotation;
}

void AVFVideoRendererControl::setVideoMirrored(bool mirrored)
{
    m_mirrored = mirrored;
}

void AVFVideoRendererControl::updateVideoFrame(const CVTimeStamp &ts)
{
    Q_UNUSED(ts);

    if (!m_sink)
        return;

    if (!m_layer)
        return;

    auto *layer = playerLayer();
    if (!layer.readyForDisplay)
        return;
    nativeSizeChanged();

    QVideoFrame frame;
    size_t width, height;
    QCFType<CVPixelBufferRef> pixelBuffer = copyPixelBufferFromLayer(width, height);
    if (!pixelBuffer)
        return;
    //    qDebug() << "Got pixelbuffer with format" << fmt << Qt::hex <<
    //    CVPixelBufferGetPixelFormatType(pixelBuffer);
    auto buffer = std::make_unique<AVFVideoBuffer>(this, std::move(pixelBuffer));

    const auto format = buffer->videoFormat();
    frame = QVideoFramePrivate::createFrame(std::move(buffer), format);
    frame.setRotation(m_rotation);
    frame.setMirrored(m_mirrored);
    m_sink->setVideoFrame(frame);
}

QCFType<CVPixelBufferRef> AVFVideoRendererControl::copyPixelBufferFromLayer(size_t &width,
                                                                            size_t &height)
{
    AVPlayerLayer *layer = playerLayer();
    //Is layer valid
    if (!layer) {
#ifdef QT_DEBUG_AVF
        qWarning("copyPixelBufferFromLayer: invalid layer");
#endif
        return nullptr;
    }

    AVPlayerItem * item = [[layer player] currentItem];

    if (!m_videoOutput) {
        if (!m_outputSettings)
            setOutputSettings();
        m_videoOutput = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:m_outputSettings];
        [m_videoOutput setDelegate:nil queue:nil];
    }
    if (!m_subtitleOutput) {
        m_subtitleOutput = [[AVPlayerItemLegibleOutput alloc] init];
        m_subtitleDelegate = [[SubtitleDelegate alloc] initWithRenderer:this];
        [m_subtitleOutput setDelegate:m_subtitleDelegate queue:dispatch_get_main_queue()];
    }
    if (![item.outputs containsObject:m_videoOutput])
        [item addOutput:m_videoOutput];
    if (![item.outputs containsObject:m_subtitleOutput])
        [item addOutput:m_subtitleOutput];

    CFTimeInterval currentCAFrameTime =  CACurrentMediaTime();
    CMTime currentCMFrameTime =  [m_videoOutput itemTimeForHostTime:currentCAFrameTime];
    // happens when buffering / loading
    if (CMTimeCompare(currentCMFrameTime, kCMTimeZero) < 0) {
        return nullptr;
    }

    if (![m_videoOutput hasNewPixelBufferForItemTime:currentCMFrameTime])
        return nullptr;

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
    return QCFType<CVPixelBufferRef>{ pixelBuffer };
}

#include "moc_avfvideorenderercontrol_p.cpp"
