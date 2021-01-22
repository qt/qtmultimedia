/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qgstreamervideorenderer_p.h"
#include <private/qvideosurfacegstsink_p.h>
#include <private/qgstutils_p.h>
#include <qabstractvideosurface.h>
#include <QtCore/qdebug.h>

#include <gst/gst.h>

static inline void resetSink(GstElement *&element, GstElement *v = nullptr)
{
    if (element)
        gst_object_unref(GST_OBJECT(element));

    if (v)
        qt_gst_object_ref_sink(GST_OBJECT(v));

    element = v;
}

QGstreamerVideoRenderer::QGstreamerVideoRenderer(QObject *parent)
    : QVideoRendererControl(parent)
{
}

QGstreamerVideoRenderer::~QGstreamerVideoRenderer()
{
    resetSink(m_videoSink);
}

void QGstreamerVideoRenderer::setVideoSink(GstElement *sink)
{
    if (!sink)
        return;

    resetSink(m_videoSink, sink);
    emit sinkChanged();
}

GstElement *QGstreamerVideoRenderer::videoSink()
{
    if (!m_videoSink && m_surface) {
        auto sink = reinterpret_cast<GstElement *>(QVideoSurfaceGstSink::createSink(m_surface));
        resetSink(m_videoSink, sink);
    }

    return m_videoSink;
}

void QGstreamerVideoRenderer::stopRenderer()
{
    if (m_surface)
        m_surface->stop();
}

QAbstractVideoSurface *QGstreamerVideoRenderer::surface() const
{
    return m_surface;
}

void QGstreamerVideoRenderer::setSurface(QAbstractVideoSurface *surface)
{
    if (m_surface != surface) {
        resetSink(m_videoSink);

        if (m_surface) {
            disconnect(m_surface.data(), SIGNAL(supportedFormatsChanged()),
                       this, SLOT(handleFormatChange()));
        }

        bool wasReady = isReady();

        m_surface = surface;

        if (m_surface) {
            connect(m_surface.data(), SIGNAL(supportedFormatsChanged()),
                    this, SLOT(handleFormatChange()));
        }

        if (wasReady != isReady())
            emit readyChanged(isReady());

        emit sinkChanged();
    }
}

void QGstreamerVideoRenderer::handleFormatChange()
{
    setVideoSink(nullptr);
}
