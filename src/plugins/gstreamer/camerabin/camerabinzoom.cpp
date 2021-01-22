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

#include "camerabinzoom.h"
#include "camerabinsession.h"

#define ZOOM_PROPERTY "zoom"
#define MAX_ZOOM_PROPERTY "max-zoom"

QT_BEGIN_NAMESPACE

CameraBinZoom::CameraBinZoom(CameraBinSession *session)
    : QCameraZoomControl(session)
    , m_session(session)
    , m_requestedOpticalZoom(1.0)
    , m_requestedDigitalZoom(1.0)
{
    GstElement *camerabin = m_session->cameraBin();
    g_signal_connect(G_OBJECT(camerabin), "notify::zoom", G_CALLBACK(updateZoom), this);
    g_signal_connect(G_OBJECT(camerabin), "notify::max-zoom", G_CALLBACK(updateMaxZoom), this);
}

CameraBinZoom::~CameraBinZoom()
{
}

qreal CameraBinZoom::maximumOpticalZoom() const
{
    return 1.0;
}

qreal CameraBinZoom::maximumDigitalZoom() const
{
    gfloat zoomFactor = 1.0;
    g_object_get(GST_BIN(m_session->cameraBin()), MAX_ZOOM_PROPERTY, &zoomFactor, NULL);
    return zoomFactor;
}

qreal CameraBinZoom::requestedDigitalZoom() const
{
    return m_requestedDigitalZoom;
}

qreal CameraBinZoom::requestedOpticalZoom() const
{
    return m_requestedOpticalZoom;
}

qreal CameraBinZoom::currentOpticalZoom() const
{
    return 1.0;
}

qreal CameraBinZoom::currentDigitalZoom() const
{
    gfloat zoomFactor = 1.0;
    g_object_get(GST_BIN(m_session->cameraBin()), ZOOM_PROPERTY, &zoomFactor, NULL);
    return zoomFactor;
}

void CameraBinZoom::zoomTo(qreal optical, qreal digital)
{
    qreal oldDigitalZoom = currentDigitalZoom();

    if (m_requestedDigitalZoom != digital) {
        m_requestedDigitalZoom = digital;
        emit requestedDigitalZoomChanged(digital);
    }

    if (m_requestedOpticalZoom != optical) {
        m_requestedOpticalZoom = optical;
        emit requestedOpticalZoomChanged(optical);
    }

    digital = qBound(qreal(1.0), digital, maximumDigitalZoom());
    g_object_set(GST_BIN(m_session->cameraBin()), ZOOM_PROPERTY, digital, NULL);

    qreal newDigitalZoom = currentDigitalZoom();
    if (!qFuzzyCompare(oldDigitalZoom, newDigitalZoom))
        emit currentDigitalZoomChanged(digital);
}

void CameraBinZoom::updateZoom(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(p);

    gfloat zoomFactor = 1.0;
    g_object_get(o, ZOOM_PROPERTY, &zoomFactor, NULL);

    CameraBinZoom *zoom = reinterpret_cast<CameraBinZoom *>(d);

    QMetaObject::invokeMethod(zoom, "currentDigitalZoomChanged",
                              Qt::QueuedConnection,
                              Q_ARG(qreal, zoomFactor));
}

void CameraBinZoom::updateMaxZoom(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(p);

    gfloat zoomFactor = 1.0;
    g_object_get(o, MAX_ZOOM_PROPERTY, &zoomFactor, NULL);

    CameraBinZoom *zoom = reinterpret_cast<CameraBinZoom *>(d);

    QMetaObject::invokeMethod(zoom, "maximumDigitalZoomChanged",
                              Qt::QueuedConnection,
                              Q_ARG(qreal, zoomFactor));
}

QT_END_NAMESPACE
