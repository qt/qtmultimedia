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

#ifndef DIRECTSHOWCAMERAZOOMCONTROL_H
#define DIRECTSHOWCAMERAZOOMCONTROL_H

#include <QtMultimedia/qcamerazoomcontrol.h>
#include <QtMultimedia/qcamera.h>

QT_BEGIN_NAMESPACE

class DSCameraSession;

class DirectShowCameraZoomControl : public QCameraZoomControl
{
    Q_OBJECT
public:
    DirectShowCameraZoomControl(DSCameraSession *session);

    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;
    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;
    void zoomTo(qreal optical, qreal digital) override;

private Q_SLOTS:
    void onStatusChanged(QCamera::Status status);

private:
    DSCameraSession *m_session;
    struct ZoomValues
    {
        long maxZoom;
        long minZoom;
        long stepping;
        long defaultZoom;
        long caps;
    } m_opticalZoom;

    qreal m_currentOpticalZoom;
    qreal m_requestedOpticalZoom;
    qreal m_maxOpticalZoom;

    void updateZoomValues();
    bool opticalZoomToPrivate(qreal value);
};

QT_END_NAMESPACE

#endif // DIRECTSHOWCAMERAZOOMCONTROL_H
