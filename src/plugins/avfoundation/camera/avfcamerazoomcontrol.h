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

#ifndef AVFCAMERAZOOMCONTROL_H
#define AVFCAMERAZOOMCONTROL_H

#include <QtMultimedia/qcamerazoomcontrol.h>
#include <QtMultimedia/qcamera.h>

#include <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraService;
class AVFCameraSession;
class AVFCameraControl;

class AVFCameraZoomControl : public QCameraZoomControl
{
    Q_OBJECT
public:
    AVFCameraZoomControl(AVFCameraService *service);

    qreal maximumOpticalZoom() const override;
    qreal maximumDigitalZoom() const override;

    qreal requestedOpticalZoom() const override;
    qreal requestedDigitalZoom() const override;
    qreal currentOpticalZoom() const override;
    qreal currentDigitalZoom() const override;

    void zoomTo(qreal optical, qreal digital) override;

private Q_SLOTS:
    void cameraStateChanged();

private:
    void zoomToRequestedDigital();

    AVFCameraSession *m_session;

    CGFloat m_maxZoomFactor;
    CGFloat m_zoomFactor;
    CGFloat m_requestedZoomFactor;
};

QT_END_NAMESPACE

#endif
