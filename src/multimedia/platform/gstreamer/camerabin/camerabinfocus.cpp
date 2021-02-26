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

#include "camerabinfocus.h"
#include "camerabinsession.h"

#include <gst/interfaces/photography.h>

#include <QDebug>
#include <QtCore/qcoreevent.h>
#include <QtCore/qmetaobject.h>

#include <private/qgstutils_p.h>

#define ZOOM_PROPERTY "zoom"
#define MAX_ZOOM_PROPERTY "max-zoom"

//#define CAMERABIN_DEBUG 1

QT_BEGIN_NAMESPACE

CameraBinFocus::CameraBinFocus(CameraBinSession *session)
    :QPlatformCameraFocus(session),
     QGstreamerBufferProbe(ProbeBuffers),
     m_session(session),
     m_cameraStatus(QCamera::UnloadedStatus),
     m_focusMode(QCameraFocus::AutoFocus),
     m_focusPointMode(QCameraFocus::FocusPointAuto),
     m_focusStatus(QCamera::Unlocked),
     m_focusPoint(0.5, 0.5),
     m_focusRect(0, 0, 0.3, 0.3)
{
    m_focusRect.moveCenter(m_focusPoint);

    gst_photography_set_focus_mode(m_session->photography(), GST_PHOTOGRAPHY_FOCUS_MODE_AUTO);

    connect(m_session, SIGNAL(statusChanged(QCamera::Status)),
            this, SLOT(_q_handleCameraStatusChange(QCamera::Status)));

    GstElement *camerabin = m_session->cameraBin();
    g_signal_connect(G_OBJECT(camerabin), "notify::zoom", G_CALLBACK(updateZoom), this);
    g_signal_connect(G_OBJECT(camerabin), "notify::max-zoom", G_CALLBACK(updateMaxZoom), this);
}

CameraBinFocus::~CameraBinFocus()
{
}

QCameraFocus::FocusModes CameraBinFocus::focusMode() const
{
    return m_focusMode;
}

void CameraBinFocus::setFocusMode(QCameraFocus::FocusModes mode)
{
    GstPhotographyFocusMode photographyMode;

    switch (mode) {
    case QCameraFocus::AutoFocus:
        photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_AUTO;
        break;
    case QCameraFocus::HyperfocalFocus:
        photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_HYPERFOCAL;
        break;
    case QCameraFocus::InfinityFocus:
        photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_INFINITY;
        break;
    case QCameraFocus::ContinuousFocus:
        photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_CONTINUOUS_NORMAL;
        break;
    case QCameraFocus::MacroFocus:
        photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MACRO;
        break;
    default:
        if (mode & QCameraFocus::AutoFocus) {
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_AUTO;
            break;
        } else {
            return;
        }
    }

    if (gst_photography_set_focus_mode(m_session->photography(), photographyMode))
        m_focusMode = mode;
}

bool CameraBinFocus::isFocusModeSupported(QCameraFocus::FocusModes mode) const
{
    switch (mode) {
    case QCameraFocus::AutoFocus:
    case QCameraFocus::HyperfocalFocus:
    case QCameraFocus::InfinityFocus:
    case QCameraFocus::ContinuousFocus:
    case QCameraFocus::MacroFocus:
        return true;
    default:
        return mode & QCameraFocus::AutoFocus;
    }
}

QCameraFocus::FocusPointMode CameraBinFocus::focusPointMode() const
{
    return m_focusPointMode;
}

void CameraBinFocus::setFocusPointMode(QCameraFocus::FocusPointMode mode)
{
    GstElement *source = m_session->cameraSource();

    if (m_focusPointMode == mode || !source)
        return;

    if (m_focusPointMode == QCameraFocus::FocusPointFaceDetection) {
        g_object_set (G_OBJECT(source), "detect-faces", FALSE, NULL);

        if (GstPad *pad = gst_element_get_static_pad(source, "vfsrc")) {
            removeProbeFromPad(pad);
            gst_object_unref(GST_OBJECT(pad));
        }

        m_faceResetTimer.stop();
        m_faceFocusRects.clear();

        QMutexLocker locker(&m_mutex);
        m_faces.clear();
    }

    if (m_focusPointMode != QCameraFocus::FocusPointAuto)
        resetFocusPoint();

    switch (mode) {
    case QCameraFocus::FocusPointAuto:
    case QCameraFocus::FocusPointCustom:
        break;
    case QCameraFocus::FocusPointFaceDetection:
        if (g_object_class_find_property(G_OBJECT_GET_CLASS(source), "detect-faces")) {
            if (GstPad *pad = gst_element_get_static_pad(source, "vfsrc")) {
                addProbeToPad(pad);
                g_object_set (G_OBJECT(source), "detect-faces", TRUE, NULL);
                break;
            }
        }
        return;
    default:
        return;
    }

    m_focusPointMode = mode;
    emit focusPointModeChanged(m_focusPointMode);
}

bool CameraBinFocus::isFocusPointModeSupported(QCameraFocus::FocusPointMode mode) const
{
    switch (mode) {
    case QCameraFocus::FocusPointAuto:
    case QCameraFocus::FocusPointCustom:
        return true;
    case QCameraFocus::FocusPointFaceDetection:
        if (GstElement *source = m_session->cameraSource())
            return g_object_class_find_property(G_OBJECT_GET_CLASS(source), "detect-faces");
        return false;
    default:
        return false;
    }
}

QPointF CameraBinFocus::customFocusPoint() const
{
    return m_focusPoint;
}

void CameraBinFocus::setCustomFocusPoint(const QPointF &point)
{
    if (m_focusPoint != point) {
        m_focusPoint = point;

        // Bound the focus point so the focus rect remains entirely within the unit square.
        m_focusPoint.setX(qBound(m_focusRect.width() / 2, m_focusPoint.x(), 1 - m_focusRect.width() / 2));
        m_focusPoint.setY(qBound(m_focusRect.height() / 2, m_focusPoint.y(), 1 - m_focusRect.height() / 2));

        if (m_focusPointMode == QCameraFocus::FocusPointCustom) {
            const QRectF focusRect = m_focusRect;
            m_focusRect.moveCenter(m_focusPoint);

            updateRegionOfInterest(m_focusRect);
        }

        emit customFocusPointChanged(m_focusPoint);
    }
}

void CameraBinFocus::_q_handleCameraStatusChange(QCamera::Status status)
{
    m_cameraStatus = status;
    if (status == QCamera::ActiveStatus) {
        if (GstPad *pad = gst_element_get_static_pad(m_session->cameraSource(), "vfsrc")) {
            if (GstCaps *caps = qt_gst_pad_get_current_caps(pad)) {
                if (GstStructure *structure = gst_caps_get_structure(caps, 0)) {
                    int width = 0;
                    int height = 0;
                    gst_structure_get_int(structure, "width", &width);
                    gst_structure_get_int(structure, "height", &height);
                    setViewfinderResolution(QSize(width, height));
                }
                gst_caps_unref(caps);
            }
            gst_object_unref(GST_OBJECT(pad));
        }
        if (m_focusPointMode == QCameraFocus::FocusPointCustom) {
                updateRegionOfInterest(m_focusRect);
        }
    } else {
        _q_setFocusStatus(QCamera::Unlocked, QCamera::LockLost);

        resetFocusPoint();
    }
}

void CameraBinFocus::setViewfinderResolution(const QSize &resolution)
{
    if (resolution != m_viewfinderResolution) {
        m_viewfinderResolution = resolution;
        if (!resolution.isEmpty()) {
            const QPointF center = m_focusRect.center();
            m_focusRect.setWidth(m_focusRect.height() * resolution.height() / resolution.width());
            m_focusRect.moveCenter(center);
        }
    }
}

void CameraBinFocus::resetFocusPoint()
{
    const QRectF focusRect = m_focusRect;
    m_focusPoint = QPointF(0.5, 0.5);
    m_focusRect.moveCenter(m_focusPoint);

    updateRegionOfInterest(QList<QRect>());

    if (focusRect != m_focusRect) {
        emit customFocusPointChanged(m_focusPoint);
    }
}

static void appendRegion(GValue *regions, int priority, const QRect &rectangle)
{
    GstStructure *region = gst_structure_new(
                "region",
                "region-x"        , G_TYPE_UINT , rectangle.x(),
                "region-y"        , G_TYPE_UINT,  rectangle.y(),
                "region-w"        , G_TYPE_UINT , rectangle.width(),
                "region-h"        , G_TYPE_UINT,  rectangle.height(),
                "region-priority" , G_TYPE_UINT,  priority,
                NULL);

    GValue regionValue = G_VALUE_INIT;
    g_value_init(&regionValue, GST_TYPE_STRUCTURE);
    gst_value_set_structure(&regionValue, region);
    gst_structure_free(region);

    gst_value_list_append_value(regions, &regionValue);
    g_value_unset(&regionValue);
}

void CameraBinFocus::updateRegionOfInterest(const QRectF &rectangle)
{
    updateRegionOfInterest(QList<QRect>() << QRect(
            rectangle.x() * m_viewfinderResolution.width(),
            rectangle.y() * m_viewfinderResolution.height(),
            rectangle.width() * m_viewfinderResolution.width(),
            rectangle.height() * m_viewfinderResolution.height()));
}

void CameraBinFocus::updateRegionOfInterest(const QList<QRect> &rectangles)
{
    if (m_cameraStatus != QCamera::ActiveStatus)
        return;

    GstElement * const cameraSource = m_session->cameraSource();
    if (!cameraSource)
        return;

    GValue regions = G_VALUE_INIT;
    g_value_init(&regions, GST_TYPE_LIST);

    if (rectangles.isEmpty()) {
        appendRegion(&regions, 0, QRect(0, 0, 0, 0));
    } else {
        // Add padding around small face rectangles so the auto focus has a reasonable amount
        // of image to work with.
        const int minimumDimension = qMin(
                    m_viewfinderResolution.width(), m_viewfinderResolution.height()) * 0.3;
        const QRect viewfinderRectangle(QPoint(0, 0), m_viewfinderResolution);

        for (const QRect &rectangle : rectangles) {
            QRect paddedRectangle(
                        0,
                        0,
                        qMax(rectangle.width(), minimumDimension),
                        qMax(rectangle.height(), minimumDimension));
            paddedRectangle.moveCenter(rectangle.center());

            appendRegion(&regions, 1, viewfinderRectangle.intersected(paddedRectangle));
        }
    }

    GstStructure *regionsOfInterest = gst_structure_new(
                "regions-of-interest",
                "frame-width"     , G_TYPE_UINT , m_viewfinderResolution.width(),
                "frame-height"    , G_TYPE_UINT,  m_viewfinderResolution.height(),
                NULL);
    gst_structure_set_value(regionsOfInterest, "regions", &regions);
    g_value_unset(&regions);

    GstEvent *event = gst_event_new_custom(GST_EVENT_CUSTOM_UPSTREAM, regionsOfInterest);
    gst_element_send_event(cameraSource, event);
}

void CameraBinFocus::_q_updateFaces()
{
    if (m_focusPointMode != QCameraFocus::FocusPointFaceDetection)
        return;

    QList<QRect> faces;

    {
        QMutexLocker locker(&m_mutex);
        faces = m_faces;
    }

    if (!faces.isEmpty()) {
        m_faceResetTimer.stop();
        m_faceFocusRects = faces;
        updateRegionOfInterest(m_faceFocusRects);
    } else {
        m_faceResetTimer.start(500, this);
    }
}

void CameraBinFocus::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_faceResetTimer.timerId()) {
        m_faceResetTimer.stop();

        if (m_focusStatus == QCamera::Unlocked) {
            m_faceFocusRects.clear();
            updateRegionOfInterest(m_faceFocusRects);
        }
    } else {
        QPlatformCameraFocus::timerEvent(event);
    }
}

bool CameraBinFocus::probeBuffer(GstBuffer *buffer)
{
    QList<QRect> faces;

    gpointer state = NULL;
    const GstMetaInfo *info = GST_VIDEO_REGION_OF_INTEREST_META_INFO;

    while (GstMeta *meta = gst_buffer_iterate_meta(buffer, &state)) {
        if (meta->info->api != info->api)
            continue;

        GstVideoRegionOfInterestMeta *region = reinterpret_cast<GstVideoRegionOfInterestMeta *>(meta);

        faces.append(QRect(region->x, region->y, region->w, region->h));
    }

    QMutexLocker locker(&m_mutex);

    if (m_faces != faces) {
        m_faces = faces;

        static const int signalIndex = metaObject()->indexOfSlot("_q_updateFaces()");
        metaObject()->method(signalIndex).invoke(this, Qt::QueuedConnection);
    }

    return true;
}

qreal CameraBinFocus::maximumOpticalZoom() const
{
    return 1.0;
}

qreal CameraBinFocus::maximumDigitalZoom() const
{
    gfloat zoomFactor = 1.0;
    g_object_get(GST_BIN(m_session->cameraBin()), MAX_ZOOM_PROPERTY, &zoomFactor, NULL);
    return zoomFactor;
}

qreal CameraBinFocus::requestedDigitalZoom() const
{
    return m_requestedDigitalZoom;
}

qreal CameraBinFocus::requestedOpticalZoom() const
{
    return m_requestedOpticalZoom;
}

qreal CameraBinFocus::currentOpticalZoom() const
{
    return 1.0;
}

qreal CameraBinFocus::currentDigitalZoom() const
{
    gfloat zoomFactor = 1.0;
    g_object_get(GST_BIN(m_session->cameraBin()), ZOOM_PROPERTY, &zoomFactor, NULL);
    return zoomFactor;
}

void CameraBinFocus::zoomTo(qreal optical, qreal digital)
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

void CameraBinFocus::updateZoom(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(p);

    gfloat zoomFactor = 1.0;
    g_object_get(o, ZOOM_PROPERTY, &zoomFactor, NULL);

    CameraBinFocus *zoom = reinterpret_cast<CameraBinFocus *>(d);

    QMetaObject::invokeMethod(zoom, "currentDigitalZoomChanged",
                              Qt::QueuedConnection,
                              Q_ARG(qreal, zoomFactor));
}

void CameraBinFocus::updateMaxZoom(GObject *o, GParamSpec *p, gpointer d)
{
    Q_UNUSED(p);

    gfloat zoomFactor = 1.0;
    g_object_get(o, MAX_ZOOM_PROPERTY, &zoomFactor, NULL);

    CameraBinFocus *zoom = reinterpret_cast<CameraBinFocus *>(d);

    QMetaObject::invokeMethod(zoom, "maximumDigitalZoomChanged",
                              Qt::QueuedConnection,
                              Q_ARG(qreal, zoomFactor));
}

QT_END_NAMESPACE
