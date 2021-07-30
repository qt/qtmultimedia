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
#include "private/qgstvideorenderersink_p.h"
#include <private/qgstutils_p.h>
#include <QtGui/private/qrhi_p.h>

#include <QtCore/qdebug.h>

#include <QtCore/qloggingcategory.h>

Q_LOGGING_CATEGORY(qLcMediaVideoSink, "qt.multimedia.videosink")

QGstreamerVideoSink::QGstreamerVideoSink(QVideoSink *parent)
    : QPlatformVideoSink(parent)
{
    sinkBin = QGstBin("videoSinkBin");
    // This is a hack for some iMX platforms. Thos require the use of a special video
    // conversion element in the pipeline before the video sink, as they unfortunately
    // output some proprietary format from the decoder even though it's marked as
    // a regular supported video/x-raw format.
    //
    // To fix this, simply insert the element into the pipeline if it's available. Otherwise
    // we simply use an identity element.
    auto imxVideoConvert = QGstElement("imxvideoconvert_g2d");
    if (!imxVideoConvert.isNull())
        gstPreprocess = imxVideoConvert;
    else
        gstPreprocess = QGstElement("identity");
    sinkBin.add(gstPreprocess);
    sinkBin.addGhostPad(gstPreprocess, "sink");
    createOverlay();
}

QGstreamerVideoSink::~QGstreamerVideoSink()
{
    setPipeline(QGstPipeline());
    delete m_videoOverlay;
}

QGstElement QGstreamerVideoSink::gstSink()
{
    updateSinkElement();
    return sinkBin;
}

void QGstreamerVideoSink::setPipeline(QGstPipeline pipeline)
{
    if (pipeline != gstPipeline) {
        if (!gstPipeline.isNull())
            gstPipeline.removeMessageFilter(m_videoOverlay);
    }
    gstPipeline = pipeline;
    if (!gstPipeline.isNull())
        gstPipeline.installMessageFilter(m_videoOverlay);
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
    if (!gstQtSink.isNull()) {
        // force creation of a new sink with proper caps
        createQtSink();
        updateSinkElement();
    }
}

void QGstreamerVideoSink::setDisplayRect(const QRect &rect)
{
    m_videoOverlay->setRenderRectangle(m_displayRect = rect);
}

void QGstreamerVideoSink::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    m_videoOverlay->setAspectRatioMode(mode);
}

void QGstreamerVideoSink::setFullScreen(bool fullScreen)
{
    if (fullScreen == m_fullScreen)
        return;
    m_fullScreen = fullScreen;
    if (!m_windowId)
        updateSinkElement();
    m_videoOverlay->setFullScreen(fullScreen);
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

void QGstreamerVideoSink::createQtSink()
{
    gstQtSink = QGstElement(reinterpret_cast<GstElement *>(QGstVideoRendererSink::createSink(videoSink())));
}

void QGstreamerVideoSink::updateSinkElement()
{
    QGstElement newSink;
    if (!m_videoOverlay->isNull() && (m_fullScreen || m_windowId)) {
        newSink = m_videoOverlay->videoSink();
    } else {
        if (gstQtSink.isNull())
            createQtSink();
        newSink = gstQtSink;
    }

    if (newSink == gstVideoSink)
        return;

    gstPipeline.beginConfig();

    if (!gstVideoSink.isNull()) {
        gstVideoSink.setStateSync(GST_STATE_NULL);
        sinkBin.remove(gstVideoSink);
    }

    gstVideoSink = newSink;
    sinkBin.add(gstVideoSink);
    if (!gstPreprocess.link(gstVideoSink))
        qCDebug(qLcMediaVideoSink) << "couldn't link preprocess and sink";
    gstVideoSink.setState(GST_STATE_PAUSED);

    gstPipeline.endConfig();
    gstPipeline.dumpGraph("updateVideoSink");
}
