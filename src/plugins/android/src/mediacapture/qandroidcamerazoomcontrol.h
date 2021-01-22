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

#ifndef QANDROIDCAMERAZOOMCONTROL_H
#define QANDROIDCAMERAZOOMCONTROL_H

#include <qcamerazoomcontrol.h>
#include <qcamera.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidCameraZoomControl : public QCameraZoomControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraZoomControl(QAndroidCameraSession *session);

    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;
    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;
    void zoomTo(qreal optical, qreal digital) override;

private Q_SLOTS:
    void onCameraOpened();

private:
    QAndroidCameraSession *m_cameraSession;

    qreal m_maximumZoom;
    QList<int> m_zoomRatios;
    qreal m_requestedZoom;
    qreal m_currentZoom;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAZOOMCONTROL_H
