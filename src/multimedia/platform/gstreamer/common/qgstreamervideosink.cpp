/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qgstreamervideosink_p.h"
#include "qgstreamervideorenderer_p.h"
#include <private/qgstutils_p.h>
#include <QtGui/private/qrhi_p.h>

#include <QtCore/qdebug.h>

#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcMediaVideoSink, "qt.multimedia.videosink")

QGstreamerVideoSink::QGstreamerVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
    createOverlay();
    createRenderer();
}

QGstreamerVideoSink::~QGstreamerVideoSink()
{
    delete m_videoOverlay;
    delete m_videoRenderer;
}

QVideoSink::GraphicsType QGstreamerVideoSink::graphicsType() const
{
    return m_graphicsType;
}

bool QGstreamerVideoSink::setGraphicsType(QVideoSink::GraphicsType type)
{
    if (type == QVideoSink::NativeWindow)
        createOverlay();
    else if (type == QVideoSink::Memory)
        createRenderer();
    else
        return false;
    m_graphicsType = type;
    emit sinkChanged();
    return true;
}

QGstElement QGstreamerVideoSink::gstSink()
{
    if (m_fullScreen || m_graphicsType == QVideoSink::NativeWindow)
        return m_videoOverlay->videoSink();
    return m_videoRenderer->gstVideoSink();
}

void QGstreamerVideoSink::setWinId(WId id)
{
    if (m_windowId == id)
        return;

    m_windowId = id;
    m_videoOverlay->setWindowHandle(m_windowId);
}

void QGstreamerVideoSink::setRhi(QRhi *rhi)
{
    if (rhi && rhi->backend() != QRhi::OpenGLES2)
        rhi = nullptr;
    if (m_rhi == rhi)
        return;

    m_rhi = rhi;
}

bool QGstreamerVideoSink::processSyncMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay->processSyncMessage(message);
}

bool QGstreamerVideoSink::processBusMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay->processBusMessage(message);
}

void QGstreamerVideoSink::setDisplayRect(const QRect &rect)
{
    m_videoOverlay->setRenderRectangle(m_displayRect = rect);
}

void QGstreamerVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_videoOverlay->setAspectRatioMode(mode);
}

void QGstreamerVideoSink::setBrightness(float brightness)
{
    m_videoOverlay->setBrightness(brightness);
}

void QGstreamerVideoSink::setContrast(float contrast)
{
    m_videoOverlay->setContrast(contrast);
}

void QGstreamerVideoSink::setHue(float hue)
{
    m_videoOverlay->setHue(hue);
}

void QGstreamerVideoSink::setSaturation(float saturation)
{
    m_videoOverlay->setSaturation(saturation);
}

void QGstreamerVideoSink::setFullScreen(bool fullScreen)
{
    if (fullScreen == m_fullScreen)
        return;
    m_fullScreen = fullScreen;
    if (m_graphicsType != QVideoSink::NativeWindow)
        emit sinkChanged();
}

QSize QGstreamerVideoSink::nativeSize() const
{
    return m_videoOverlay->nativeVideoSize();
}

void QGstreamerVideoSink::createOverlay()
{
    if (m_videoOverlay)
        return;
    m_videoOverlay = new QGstreamerVideoOverlay(this, qgetenv("QT_GSTREAMER_WINDOW_VIDEOSINK"));
    connect(m_videoOverlay, &QGstreamerVideoOverlay::nativeVideoSizeChanged,
            this, &QGstreamerVideoSink::nativeSizeChanged);
}

void QGstreamerVideoSink::createRenderer()
{
    m_videoRenderer = new QGstreamerVideoRenderer(sink);
}
