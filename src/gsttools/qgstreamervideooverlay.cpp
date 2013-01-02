/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qgstreamervideooverlay_p.h"
#include <private/qvideosurfacegstsink_p.h>

#include <qvideosurfaceformat.h>

#include <qx11videosurface_p.h>

QGstreamerVideoOverlay::QGstreamerVideoOverlay(QObject *parent)
    : QVideoWindowControl(parent)
    , m_surface(new QX11VideoSurface)
    , m_videoSink(reinterpret_cast<GstElement*>(QVideoSurfaceGstSink::createSink(m_surface)))
    , m_aspectRatioMode(Qt::KeepAspectRatio)
    , m_fullScreen(false)
{
    if (m_videoSink) {
        gst_object_ref(GST_OBJECT(m_videoSink)); //Take ownership
        gst_object_sink(GST_OBJECT(m_videoSink));
    }

    connect(m_surface, SIGNAL(surfaceFormatChanged(QVideoSurfaceFormat)),
            this, SLOT(surfaceFormatChanged()));
}

QGstreamerVideoOverlay::~QGstreamerVideoOverlay()
{
    if (m_videoSink)
        gst_object_unref(GST_OBJECT(m_videoSink));

    delete m_surface;
}

WId QGstreamerVideoOverlay::winId() const
{
    return m_surface->winId();
}

void QGstreamerVideoOverlay::setWinId(WId id)
{
    bool wasReady = isReady();
    m_surface->setWinId(id);

    if (isReady() != wasReady)
        emit readyChanged(!wasReady);
}

QRect QGstreamerVideoOverlay::displayRect() const
{
    return m_displayRect;
}

void QGstreamerVideoOverlay::setDisplayRect(const QRect &rect)
{
    m_displayRect = rect;

    setScaledDisplayRect();
}

Qt::AspectRatioMode QGstreamerVideoOverlay::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void QGstreamerVideoOverlay::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_aspectRatioMode = mode;

    setScaledDisplayRect();
}

void QGstreamerVideoOverlay::repaint()
{
}

int QGstreamerVideoOverlay::brightness() const
{
    return m_surface->brightness();
}

void QGstreamerVideoOverlay::setBrightness(int brightness)
{
    m_surface->setBrightness(brightness);

    emit brightnessChanged(m_surface->brightness());
}

int QGstreamerVideoOverlay::contrast() const
{
    return m_surface->contrast();
}

void QGstreamerVideoOverlay::setContrast(int contrast)
{
    m_surface->setContrast(contrast);

    emit contrastChanged(m_surface->contrast());
}

int QGstreamerVideoOverlay::hue() const
{
    return m_surface->hue();
}

void QGstreamerVideoOverlay::setHue(int hue)
{
    m_surface->setHue(hue);

    emit hueChanged(m_surface->hue());
}

int QGstreamerVideoOverlay::saturation() const
{
    return m_surface->saturation();
}

void QGstreamerVideoOverlay::setSaturation(int saturation)
{
    m_surface->setSaturation(saturation);

    emit saturationChanged(m_surface->saturation());
}

bool QGstreamerVideoOverlay::isFullScreen() const
{
    return m_fullScreen;
}

void QGstreamerVideoOverlay::setFullScreen(bool fullScreen)
{
    emit fullScreenChanged(m_fullScreen = fullScreen);
}

QSize QGstreamerVideoOverlay::nativeSize() const
{
    return m_surface->surfaceFormat().sizeHint();
}

QAbstractVideoSurface *QGstreamerVideoOverlay::surface() const
{
    return m_surface;
}

GstElement *QGstreamerVideoOverlay::videoSink()
{
    return m_videoSink;
}

void QGstreamerVideoOverlay::surfaceFormatChanged()
{
    setScaledDisplayRect();

    emit nativeSizeChanged();
}

void QGstreamerVideoOverlay::setScaledDisplayRect()
{
    QRect formatViewport = m_surface->surfaceFormat().viewport();

    switch (m_aspectRatioMode) {
    case Qt::KeepAspectRatio:
        {
            QSize size = m_surface->surfaceFormat().sizeHint();
            size.scale(m_displayRect.size(), Qt::KeepAspectRatio);

            QRect rect(QPoint(0, 0), size);
            rect.moveCenter(m_displayRect.center());

            m_surface->setDisplayRect(rect);
            m_surface->setViewport(formatViewport);
        }
        break;
    case Qt::IgnoreAspectRatio:
        m_surface->setDisplayRect(m_displayRect);
        m_surface->setViewport(formatViewport);
        break;
    case Qt::KeepAspectRatioByExpanding:
        {
            QSize size = m_displayRect.size();
            size.scale(m_surface->surfaceFormat().sizeHint(), Qt::KeepAspectRatio);

            QRect viewport(QPoint(0, 0), size);
            viewport.moveCenter(formatViewport.center());
            m_surface->setDisplayRect(m_displayRect);
            m_surface->setViewport(viewport);
        }
        break;
    };
}
