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

#ifndef CAMERABINFOCUSCONTROL_H
#define CAMERABINFOCUSCONTROL_H

#include <qcamera.h>
#include <qcamerafocuscontrol.h>

#include <private/qgstreamerbufferprobe_p.h>

#include <qbasictimer.h>
#include <qmutex.h>
#include <qvector.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinFocus
    : public QCameraFocusControl
#if GST_CHECK_VERSION(1,0,0)
    , QGstreamerBufferProbe
#endif
{
    Q_OBJECT

public:
    CameraBinFocus(CameraBinSession *session);
    virtual ~CameraBinFocus();

    QCameraFocus::FocusModes focusMode() const;
    void setFocusMode(QCameraFocus::FocusModes mode);
    bool isFocusModeSupported(QCameraFocus::FocusModes mode) const;

    QCameraFocus::FocusPointMode focusPointMode() const;
    void setFocusPointMode(QCameraFocus::FocusPointMode mode) ;
    bool isFocusPointModeSupported(QCameraFocus::FocusPointMode) const;
    QPointF customFocusPoint() const;
    void setCustomFocusPoint(const QPointF &point);

    QCameraFocusZoneList focusZones() const;

    void handleFocusMessage(GstMessage*);
    QCamera::LockStatus focusStatus() const { return m_focusStatus; }

Q_SIGNALS:
    void _q_focusStatusChanged(QCamera::LockStatus status, QCamera::LockChangeReason reason);

public Q_SLOTS:
    void _q_startFocusing();
    void _q_stopFocusing();

    void setViewfinderResolution(const QSize &resolution);

#if GST_CHECK_VERSION(1,0,0)
protected:
    void timerEvent(QTimerEvent *event);
#endif

private Q_SLOTS:
    void _q_setFocusStatus(QCamera::LockStatus status, QCamera::LockChangeReason reason);
    void _q_handleCameraStatusChange(QCamera::Status status);

#if GST_CHECK_VERSION(1,0,0)
    void _q_updateFaces();
#endif

private:
    void resetFocusPoint();
    void updateRegionOfInterest(const QRectF &rectangle);
    void updateRegionOfInterest(const QVector<QRect> &rectangles);

#if GST_CHECK_VERSION(1,0,0)
    bool probeBuffer(GstBuffer *buffer);
#endif

    CameraBinSession *m_session;
    QCamera::Status m_cameraStatus;
    QCameraFocus::FocusModes m_focusMode;
    QCameraFocus::FocusPointMode m_focusPointMode;
    QCamera::LockStatus m_focusStatus;
    QCameraFocusZone::FocusZoneStatus m_focusZoneStatus;
    QPointF m_focusPoint;
    QRectF m_focusRect;
    QSize m_viewfinderResolution;
    QVector<QRect> m_faces;
    QVector<QRect> m_faceFocusRects;
    QBasicTimer m_faceResetTimer;
    mutable QMutex m_mutex;
};

QT_END_NAMESPACE

#endif // CAMERABINFOCUSCONTROL_H
