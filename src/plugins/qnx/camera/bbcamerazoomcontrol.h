/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBCAMERAZOOMCONTROL_H
#define BBCAMERAZOOMCONTROL_H

#include <qcamera.h>
#include <qcamerazoomcontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraZoomControl : public QCameraZoomControl
{
    Q_OBJECT
public:
    explicit BbCameraZoomControl(BbCameraSession *session, QObject *parent = 0);

    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;
    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;
    void zoomTo(qreal optical, qreal digital) override;

private Q_SLOTS:
    void statusChanged(QCamera::Status status);

private:
    BbCameraSession *m_session;

    qreal m_minimumZoomFactor;
    qreal m_maximumZoomFactor;
    bool m_supportsSmoothZoom;
    qreal m_requestedZoomFactor;
};

QT_END_NAMESPACE

#endif
