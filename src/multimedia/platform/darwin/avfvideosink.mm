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

#include "avfvideosink_p.h"

#include <AVFoundation/AVFoundation.h>
#import <QuartzCore/CATransaction.h>

#if QT_HAS_INCLUDE(<AppKit/AppKit.h>)
#include <AppKit/AppKit.h>
#endif

#if QT_HAS_INCLUDE(<UIKit/UIKit.h>)
#include <UIKit/UIKit.h>
#endif

QT_USE_NAMESPACE

AVFVideoSink::AVFVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
}

AVFVideoSink::~AVFVideoSink()
{
}

bool AVFVideoSink::setGraphicsType(QVideoSink::GraphicsType type)
{
    if (type == m_graphicsType)
        return true;
    m_graphicsType = type;
    if (m_interface)
        m_interface->reconfigure();
    return true;
}

WId AVFVideoSink::winId() const
{
    return m_winId;
}

void AVFVideoSink::setWinId(WId id)
{
    if (id == m_winId)
        return;
    m_winId = id;
    m_nativeView = (NativeView*)m_winId;
    if (m_interface)
        m_interface->reconfigure();
}

void AVFVideoSink::setRhi(QRhi *rhi)
{
    m_rhi = rhi;
    if (m_interface)
        m_interface->setRhi(rhi);
}

QRect AVFVideoSink::displayRect() const
{
    return m_displayRect;
}

void AVFVideoSink::setDisplayRect(const QRect &rect)
{
    if (m_displayRect == rect)
        return;
    m_displayRect = rect;
    if (m_interface)
        m_interface->updateLayerBounds();
}

bool AVFVideoSink::isFullScreen() const
{
    return m_fullscreen;
}

void AVFVideoSink::setFullScreen(bool fullScreen)
{
    if (fullScreen == m_fullscreen)
        return;
    m_fullscreen = fullScreen;
    if (m_interface)
        m_interface->reconfigure();
}

void AVFVideoSink::repaint()
{
}

QSize AVFVideoSink::nativeSize() const
{
    return m_nativeSize;
}

void AVFVideoSink::setNativeSize(QSize size)
{
    if (size == m_nativeSize)
        return;
    m_nativeSize = size;
    if (m_interface)
        m_interface->nativeSizeChanged();
}

Qt::AspectRatioMode AVFVideoSink::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void AVFVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (m_aspectRatioMode == mode)
        return;
    m_aspectRatioMode = mode;
    if (m_interface)
        m_interface->updateAspectRatio();
}

int AVFVideoSink::brightness() const
{
    return m_brightness;
}

void AVFVideoSink::setBrightness(int brightness)
{
    m_brightness = brightness;
}

int AVFVideoSink::contrast() const
{
    return m_contrast;
}

void AVFVideoSink::setContrast(int contrast)
{
    m_contrast = contrast;
}

int AVFVideoSink::hue() const
{
    return m_hue;
}

void AVFVideoSink::setHue(int hue)
{
    m_hue = hue;
}

int AVFVideoSink::saturation() const
{
    return m_saturation;
}

void AVFVideoSink::setSaturation(int saturation)
{
    m_saturation = saturation;
}

void AVFVideoSink::setLayer(CALayer *)
{
}

void AVFVideoSink::setVideoSinkInterface(AVFVideoSinkInterface *interface)
{
    m_interface = interface;
    if (m_interface)
        m_interface->setRhi(m_rhi);
}

AVFVideoSinkInterface::~AVFVideoSinkInterface()
{
    if (m_layer)
        [m_layer release];
}

void AVFVideoSinkInterface::setVideoSink(AVFVideoSink *sink)
{
    if (sink == m_sink)
        return;

    m_sink = sink;
    if (m_sink)
        m_sink->setVideoSinkInterface(this);
    reconfigure();
}
#include <qdebug.h>

void AVFVideoSinkInterface::setLayer(CALayer *layer)
{
    if (layer == m_layer)
        return;

    if (m_layer) {
        renderToNativeView(false);
        [m_layer release];
    }
    m_layer = layer;
    [m_layer retain];

    reconfigure();
}

void AVFVideoSinkInterface::renderToNativeView(bool enable)
{
    auto *view = nativeView();
    if (enable && view && m_layer) {
        CALayer *nativeLayer = [view layer];
        [nativeLayer addSublayer:m_layer];
        m_rendersToWindow = true;
    } else {
        if (m_layer)
            [m_layer removeFromSuperlayer];
        m_rendersToWindow = false;
    }
    updateLayerBounds();
}

void AVFVideoSinkInterface::updateLayerBounds()
{
    if (!m_layer)
        return;
    [CATransaction begin];
    [CATransaction setDisableActions: YES]; // disable animation/flicks
    if (m_rendersToWindow) {
        m_layer.frame = displayRect().toCGRect();
    } else {
        m_layer.frame = QRectF(0, 0, nativeSize().width(), nativeSize().height()).toCGRect();
        m_layer.bounds = m_layer.frame;
    }
    [CATransaction commit];
}


#include "moc_avfvideosink_p.cpp"
