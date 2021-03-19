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

#include "avfvideowindowcontrol_p.h"

#include <AVFoundation/AVFoundation.h>
#import <QuartzCore/CATransaction.h>

#if QT_HAS_INCLUDE(<AppKit/AppKit.h>)
#include <AppKit/AppKit.h>
#endif

#if QT_HAS_INCLUDE(<UIKit/UIKit.h>)
#include <UIKit/UIKit.h>
#endif

QT_USE_NAMESPACE

AVFVideoWindowControl::AVFVideoWindowControl(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
}

AVFVideoWindowControl::~AVFVideoWindowControl()
{
    if (m_layer) {
        [m_layer removeFromSuperlayer];
        [m_layer release];
    }
}

WId AVFVideoWindowControl::winId() const
{
    return m_winId;
}

void AVFVideoWindowControl::setWinId(WId id)
{
    m_winId = id;
    m_nativeView = (NativeView*)m_winId;
}

QRect AVFVideoWindowControl::displayRect() const
{
    return m_displayRect;
}

void AVFVideoWindowControl::setDisplayRect(const QRect &rect)
{
    if (m_displayRect != rect) {
        m_displayRect = rect;
        updatePlayerLayerBounds();
    }
}

bool AVFVideoWindowControl::isFullScreen() const
{
    return m_fullscreen;
}

void AVFVideoWindowControl::setFullScreen(bool fullScreen)
{
    m_fullscreen = fullScreen;
}

void AVFVideoWindowControl::repaint()
{
    if (m_layer)
        [m_layer setNeedsDisplay];
}

QSize AVFVideoWindowControl::nativeSize() const
{
    return m_nativeSize;
}

Qt::AspectRatioMode AVFVideoWindowControl::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void AVFVideoWindowControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (m_aspectRatioMode != mode) {
        m_aspectRatioMode = mode;
        updateAspectRatio();
    }
}

int AVFVideoWindowControl::brightness() const
{
    return m_brightness;
}

void AVFVideoWindowControl::setBrightness(int brightness)
{
    m_brightness = brightness;
}

int AVFVideoWindowControl::contrast() const
{
    return m_contrast;
}

void AVFVideoWindowControl::setContrast(int contrast)
{
    m_contrast = contrast;
}

int AVFVideoWindowControl::hue() const
{
    return m_hue;
}

void AVFVideoWindowControl::setHue(int hue)
{
    m_hue = hue;
}

int AVFVideoWindowControl::saturation() const
{
    return m_saturation;
}

void AVFVideoWindowControl::setSaturation(int saturation)
{
    m_saturation = saturation;
}

template<typename T> inline T* objc_cast(id from) {
    if ([from isKindOfClass:[T class]]) {
        return static_cast<T*>(from);
    }
    return nil;
}

void AVFVideoWindowControl::setLayer(CALayer *layer)
{
    m_playerLayer = objc_cast<AVPlayerLayer>(layer);
    m_previewLayer = objc_cast<AVCaptureVideoPreviewLayer>(layer);
    if (m_layer == layer)
        return;

    if (!m_winId) {
        qDebug("AVFVideoWindowControl: No video window");
        return;
    }

#if defined(Q_OS_OSX)
    [m_nativeView setWantsLayer:YES];
#endif

    if (m_layer) {
        [m_layer removeFromSuperlayer];
        [m_layer release];
    }

    m_layer = layer;

    CALayer *nativeLayer = [m_nativeView layer];

    if (layer) {
        [layer retain];

        m_nativeSize = QSize(m_layer.bounds.size.width,
                             m_layer.bounds.size.height);

        updateAspectRatio();
        [nativeLayer addSublayer:m_layer];
        updatePlayerLayerBounds();
    }
}

void AVFVideoWindowControl::updateAspectRatio()
{
    AVLayerVideoGravity gravity = AVLayerVideoGravityResizeAspect;

    switch (m_aspectRatioMode) {
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
    if (m_playerLayer)
        m_playerLayer.videoGravity = gravity;
    else if (m_previewLayer)
        m_previewLayer.videoGravity = gravity;
}
#include <qdebug.h>

void AVFVideoWindowControl::updatePlayerLayerBounds()
{
    if (m_layer) {
        [CATransaction begin];
        [CATransaction setDisableActions: YES]; // disable animation/flicks
        m_layer.frame = m_displayRect.toCGRect();
        [CATransaction commit];
    }
}

#include "moc_avfvideowindowcontrol_p.cpp"
