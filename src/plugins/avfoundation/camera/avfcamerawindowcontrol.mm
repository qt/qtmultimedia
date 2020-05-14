/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfcamerawindowcontrol.h"

#import <AVFoundation/AVFoundation.h>
#import <QuartzCore/CATransaction.h>

#if QT_HAS_INCLUDE(<AppKit/AppKit.h>)
#import <AppKit/AppKit.h>
#endif

#if QT_HAS_INCLUDE(<UIKit/UIKit.h>)
#import <UIKit/UIKit.h>
#endif

QT_USE_NAMESPACE

AVFCameraWindowControl::AVFCameraWindowControl(QObject *parent)
    : QVideoWindowControl(parent)
{
    setObjectName(QStringLiteral("AVFCameraWindowControl"));
}

AVFCameraWindowControl::~AVFCameraWindowControl()
{
    releaseNativeLayer();
}

WId AVFCameraWindowControl::winId() const
{
    return m_winId;
}

void AVFCameraWindowControl::setWinId(WId id)
{
    if (m_winId == id)
        return;

    m_winId = id;

    detachNativeLayer();
    m_nativeView = (NativeView*)m_winId;
    attachNativeLayer();
}

QRect AVFCameraWindowControl::displayRect() const
{
    return m_displayRect;
}

void AVFCameraWindowControl::setDisplayRect(const QRect &rect)
{
    if (m_displayRect != rect) {
        m_displayRect = rect;
        updateCaptureLayerBounds();
    }
}

bool AVFCameraWindowControl::isFullScreen() const
{
    return m_fullscreen;
}

void AVFCameraWindowControl::setFullScreen(bool fullscreen)
{
    if (m_fullscreen != fullscreen) {
        m_fullscreen = fullscreen;
        Q_EMIT fullScreenChanged(fullscreen);
    }
}

void AVFCameraWindowControl::repaint()
{
    if (m_captureLayer)
        [m_captureLayer setNeedsDisplay];
}

QSize AVFCameraWindowControl::nativeSize() const
{
    return m_nativeSize;
}

void AVFCameraWindowControl::setNativeSize(QSize size)
{
    if (m_nativeSize != size) {
        m_nativeSize = size;
        Q_EMIT nativeSizeChanged();
    }
}

Qt::AspectRatioMode AVFCameraWindowControl::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void AVFCameraWindowControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (m_aspectRatioMode != mode) {
        m_aspectRatioMode = mode;
        updateAspectRatio();
    }
}

int AVFCameraWindowControl::brightness() const
{
    return 0;
}

void AVFCameraWindowControl::setBrightness(int brightness)
{
    if (0 != brightness)
        qWarning("AVFCameraWindowControl doesn't support changing Brightness");
}

int AVFCameraWindowControl::contrast() const
{
    return 0;
}

void AVFCameraWindowControl::setContrast(int contrast)
{
    if (0 != contrast)
        qWarning("AVFCameraWindowControl doesn't support changing Contrast");
}

int AVFCameraWindowControl::hue() const
{
    return 0;
}

void AVFCameraWindowControl::setHue(int hue)
{
    if (0 != hue)
        qWarning("AVFCameraWindowControl doesn't support changing Hue");
}

int AVFCameraWindowControl::saturation() const
{
    return 0;
}

void AVFCameraWindowControl::setSaturation(int saturation)
{
    if (0 != saturation)
        qWarning("AVFCameraWindowControl doesn't support changing Saturation");
}

void AVFCameraWindowControl::setLayer(AVCaptureVideoPreviewLayer *capturePreviewLayer)
{
    if (m_captureLayer == capturePreviewLayer)
        return;

    releaseNativeLayer();

    m_captureLayer = capturePreviewLayer;

    if (m_captureLayer)
        retainNativeLayer();
}

void AVFCameraWindowControl::updateAspectRatio()
{
    if (m_captureLayer) {
        switch (m_aspectRatioMode) {
        case Qt::IgnoreAspectRatio:
            [m_captureLayer setVideoGravity:AVLayerVideoGravityResize];
            break;
        case Qt::KeepAspectRatio:
            [m_captureLayer setVideoGravity:AVLayerVideoGravityResizeAspect];
            break;
        case Qt::KeepAspectRatioByExpanding:
            [m_captureLayer setVideoGravity:AVLayerVideoGravityResizeAspectFill];
            break;
        default:
            break;
        }
    }
}

void AVFCameraWindowControl::updateCaptureLayerBounds()
{
    if (m_captureLayer && m_nativeView) {
        [CATransaction begin];
        [CATransaction setDisableActions: YES]; // disable animation/flicks
        m_captureLayer.frame = m_displayRect.toCGRect();
        [CATransaction commit];
    }
}

void AVFCameraWindowControl::retainNativeLayer()
{
    [m_captureLayer retain];

    updateAspectRatio();
    attachNativeLayer();
}

void AVFCameraWindowControl::releaseNativeLayer()
{
    if (m_captureLayer) {
        detachNativeLayer();
        [m_captureLayer release];
        m_captureLayer = nullptr;
    }
}

void AVFCameraWindowControl::attachNativeLayer()
{
    if (m_captureLayer && m_nativeView) {
#if defined(Q_OS_MACOS)
        m_nativeView.wantsLayer = YES;
#endif
        CALayer *nativeLayer = m_nativeView.layer;
        [nativeLayer addSublayer:m_captureLayer];
        updateCaptureLayerBounds();
    }
}

void AVFCameraWindowControl::detachNativeLayer()
{
    if (m_captureLayer && m_nativeView)
        [m_captureLayer removeFromSuperlayer];
}

#include "moc_avfcamerawindowcontrol.cpp"
