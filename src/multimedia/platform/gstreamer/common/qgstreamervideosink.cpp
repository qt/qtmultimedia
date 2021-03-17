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
#include <private/qpaintervideosurface_p.h>

#include <QtCore/qdebug.h>

#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcMediaVideoSink, "qt.multimedia.videosink")

class QGstreamerVideoSurface : public QAbstractVideoSurface
{
public:
    explicit QGstreamerVideoSurface(QGstreamerVideoSink *parent = nullptr)
        : QAbstractVideoSurface(parent)
    {
        m_sink = parent;
    }
    ~QGstreamerVideoSurface();

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
        QVideoFrame::HandleType /*type*/) const override
    {
        // All the formats that both we and gstreamer support
        return QList<QVideoFrame::PixelFormat>()
               << QVideoFrame::Format_YUV420P
               << QVideoFrame::Format_YUV422P
               << QVideoFrame::Format_YV12
               << QVideoFrame::Format_UYVY
               << QVideoFrame::Format_YUYV
               << QVideoFrame::Format_NV12
               << QVideoFrame::Format_NV21
               << QVideoFrame::Format_AYUV444
               << QVideoFrame::Format_YUV444
               << QVideoFrame::Format_P010LE
               << QVideoFrame::Format_P010BE
               << QVideoFrame::Format_Y8
               << QVideoFrame::Format_RGB32
               << QVideoFrame::Format_BGR32
               << QVideoFrame::Format_ARGB32
               << QVideoFrame::Format_ABGR32
               << QVideoFrame::Format_BGRA32
               << QVideoFrame::Format_RGB555
               << QVideoFrame::Format_BGR555
               << QVideoFrame::Format_Y16
               << QVideoFrame::Format_RGB24
               << QVideoFrame::Format_BGR24
               << QVideoFrame::Format_RGB565;
    }
    bool present(const QVideoFrame &frame) override
    {
        m_sink->videoSink()->newVideoFrame(frame);
        return true;
    }
    QGstreamerVideoSink *m_sink;
};

QGstreamerVideoSurface::~QGstreamerVideoSurface() = default;

QGstreamerVideoSink::QGstreamerVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
    createOverlay();
    createRenderer();
}

QGstreamerVideoSink::~QGstreamerVideoSink()
{
    delete m_videoOverlay;
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
    return m_videoRenderer->videoSink();
}

WId QGstreamerVideoSink::winId() const
{
    return m_windowId;
}

void QGstreamerVideoSink::setWinId(WId id)
{
    if (m_windowId == id)
        return;

    m_windowId = id;
    m_videoOverlay->setWindowHandle(m_windowId);
}

bool QGstreamerVideoSink::processSyncMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay->processSyncMessage(message);
}

bool QGstreamerVideoSink::processBusMessage(const QGstreamerMessage &message)
{
    return m_videoOverlay->processBusMessage(message);
}

QRect QGstreamerVideoSink::displayRect() const
{
    return m_displayRect;
}

void QGstreamerVideoSink::setDisplayRect(const QRect &rect)
{
    m_videoOverlay->setRenderRectangle(m_displayRect = rect);
    repaint();
}

Qt::AspectRatioMode QGstreamerVideoSink::aspectRatioMode() const
{
    return m_videoOverlay->aspectRatioMode();
}

void QGstreamerVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_videoOverlay->setAspectRatioMode(mode);
}

void QGstreamerVideoSink::repaint()
{
    m_videoOverlay->expose();
}

int QGstreamerVideoSink::brightness() const
{
    return m_videoOverlay->brightness();
}

void QGstreamerVideoSink::setBrightness(int brightness)
{
    m_videoOverlay->setBrightness(brightness);
}

int QGstreamerVideoSink::contrast() const
{
    return m_videoOverlay->contrast();
}

void QGstreamerVideoSink::setContrast(int contrast)
{
    m_videoOverlay->setContrast(contrast);
}

int QGstreamerVideoSink::hue() const
{
    return m_videoOverlay->hue();
}

void QGstreamerVideoSink::setHue(int hue)
{
    m_videoOverlay->setHue(hue);
}

int QGstreamerVideoSink::saturation() const
{
    return m_videoOverlay->saturation();
}

void QGstreamerVideoSink::setSaturation(int saturation)
{
    m_videoOverlay->setSaturation(saturation);
}

bool QGstreamerVideoSink::isFullScreen() const
{
    return m_fullScreen;
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
    m_videoRenderer = new QGstreamerVideoRenderer(this);
    m_videoSurface = new QGstreamerVideoSurface(this);
    m_videoRenderer->setSurface(m_videoSurface);

    qCDebug(qLcMediaVideoSink) << Q_FUNC_INFO;
    connect(m_videoRenderer, SIGNAL(sinkChanged()), this, SLOT(sinkChanged()));
}
