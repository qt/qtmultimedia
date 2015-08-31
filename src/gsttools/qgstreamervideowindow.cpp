/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamervideowindow_p.h"
#include <private/qgstutils_p.h>

#include <QtCore/qdebug.h>

QGstreamerVideoWindow::QGstreamerVideoWindow(QObject *parent, const QByteArray &elementName)
    : QVideoWindowControl(parent)
    , m_videoOverlay(this, !elementName.isEmpty() ? elementName : qgetenv("QT_GSTREAMER_WINDOW_VIDEOSINK"))
    , m_windowId(0)
    , m_fullScreen(false)
    , m_colorKey(QColor::Invalid)
{
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::nativeVideoSizeChanged,
            this, &QGstreamerVideoWindow::nativeSizeChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::brightnessChanged,
            this, &QGstreamerVideoWindow::brightnessChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::contrastChanged,
            this, &QGstreamerVideoWindow::contrastChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::hueChanged,
            this, &QGstreamerVideoWindow::hueChanged);
    connect(&m_videoOverlay, &QGstreamerVideoOverlay::saturationChanged,
            this, &QGstreamerVideoWindow::saturationChanged);
}

QGstreamerVideoWindow::~QGstreamerVideoWindow()
{
}

GstElement *QGstreamerVideoWindow::videoSink()
{
    return m_videoOverlay.videoSink();
}

WId QGstreamerVideoWindow::winId() const
{
    return m_windowId;
}

void QGstreamerVideoWindow::setWinId(WId id)
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

bool QGstreamerVideoWindow::processSyncMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay.processSyncMessage(message);
}

bool QGstreamerVideoWindow::processBusMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay.processBusMessage(message);
}

QRect QGstreamerVideoWindow::displayRect() const
{
    return m_displayRect;
}

void QGstreamerVideoWindow::setDisplayRect(const QRect &rect)
{
    m_videoOverlay.setRenderRectangle(m_displayRect = rect);
    repaint();
}

Qt::AspectRatioMode QGstreamerVideoWindow::aspectRatioMode() const
{
    return m_videoOverlay.aspectRatioMode();
}

void QGstreamerVideoWindow::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_videoOverlay.setAspectRatioMode(mode);
}

void QGstreamerVideoWindow::repaint()
{
    m_videoOverlay.expose();
}

int QGstreamerVideoWindow::brightness() const
{
    return m_videoOverlay.brightness();
}

void QGstreamerVideoWindow::setBrightness(int brightness)
{
    m_videoOverlay.setBrightness(brightness);
}

int QGstreamerVideoWindow::contrast() const
{
    return m_videoOverlay.contrast();
}

void QGstreamerVideoWindow::setContrast(int contrast)
{
    m_videoOverlay.setContrast(contrast);
}

int QGstreamerVideoWindow::hue() const
{
    return m_videoOverlay.hue();
}

void QGstreamerVideoWindow::setHue(int hue)
{
    m_videoOverlay.setHue(hue);
}

int QGstreamerVideoWindow::saturation() const
{
    return m_videoOverlay.saturation();
}

void QGstreamerVideoWindow::setSaturation(int saturation)
{
    m_videoOverlay.setSaturation(saturation);
}

bool QGstreamerVideoWindow::isFullScreen() const
{
    return m_fullScreen;
}

void QGstreamerVideoWindow::setFullScreen(bool fullScreen)
{
    emit fullScreenChanged(m_fullScreen = fullScreen);
}

QSize QGstreamerVideoWindow::nativeSize() const
{
    return m_videoOverlay.nativeVideoSize();
}
