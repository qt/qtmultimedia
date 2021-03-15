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
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QGstreamerVideoSink::QGstreamerVideoSink(QObject *parent, const QByteArray &elementName)
    : QPlatformVideoSink(parent)
    , m_videoOverlay(this, !elementName.isEmpty() ? elementName : qgetenv("QT_GSTREAMER_WINDOW_VIDEOSINK"))
{
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::nativeVideoSizeChanged,
            this, &QGstreamerVideoSink::nativeSizeChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::brightnessChanged,
            this, &QGstreamerVideoSink::brightnessChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::contrastChanged,
            this, &QGstreamerVideoSink::contrastChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::hueChanged,
            this, &QGstreamerVideoSink::hueChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::saturationChanged,
            this, &QGstreamerVideoSink::saturationChanged);
}

QGstreamerVideoSink::~QGstreamerVideoSink() = default;

GstElement *QGstreamerVideoSink::videoSink()
{
    return m_videoOverlay.videoSink();
}

WId QGstreamerVideoSink::winId() const
{
    return m_windowId;
}

void QGstreamerVideoSink::setWinId(WId id)
{
    if (m_windowId == id)
        return;

    WId oldId = m_windowId;
    m_videoOverlay.setWindowHandle(m_windowId = id);

    if (!oldId)
        emit readyChanged(true);

    if (!id)
        emit readyChanged(false);
}

bool QGstreamerVideoSink::processSyncMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay.processSyncMessage(message);
}

bool QGstreamerVideoSink::processBusMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay.processBusMessage(message);
}

QRect QGstreamerVideoSink::displayRect() const
{
    return m_displayRect;
}

void QGstreamerVideoSink::setDisplayRect(const QRect &rect)
{
    m_videoOverlay.setRenderRectangle(m_displayRect = rect);
    repaint();
}

Qt::AspectRatioMode QGstreamerVideoSink::aspectRatioMode() const
{
    return m_videoOverlay.aspectRatioMode();
}

void QGstreamerVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_videoOverlay.setAspectRatioMode(mode);
}

void QGstreamerVideoSink::repaint()
{
    m_videoOverlay.expose();
}

int QGstreamerVideoSink::brightness() const
{
    return m_videoOverlay.brightness();
}

void QGstreamerVideoSink::setBrightness(int brightness)
{
    m_videoOverlay.setBrightness(brightness);
}

int QGstreamerVideoSink::contrast() const
{
    return m_videoOverlay.contrast();
}

void QGstreamerVideoSink::setContrast(int contrast)
{
    m_videoOverlay.setContrast(contrast);
}

int QGstreamerVideoSink::hue() const
{
    return m_videoOverlay.hue();
}

void QGstreamerVideoSink::setHue(int hue)
{
    m_videoOverlay.setHue(hue);
}

int QGstreamerVideoSink::saturation() const
{
    return m_videoOverlay.saturation();
}

void QGstreamerVideoSink::setSaturation(int saturation)
{
    m_videoOverlay.setSaturation(saturation);
}

bool QGstreamerVideoSink::isFullScreen() const
{
    return m_fullScreen;
}

void QGstreamerVideoSink::setFullScreen(bool fullScreen)
{
    emit fullScreenChanged(m_fullScreen = fullScreen);
}

QSize QGstreamerVideoSink::nativeSize() const
{
    return m_videoOverlay.nativeVideoSize();
}
