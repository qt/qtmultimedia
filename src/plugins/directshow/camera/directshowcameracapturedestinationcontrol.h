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

#ifndef DIRECTSHOWCAMERACAPTUREDESTINATIONCONTROL_H
#define DIRECTSHOWCAMERACAPTUREDESTINATIONCONTROL_H

#include <QtMultimedia/qcameracapturedestinationcontrol.h>

QT_BEGIN_NAMESPACE

class DSCameraSession;

class DirectShowCameraCaptureDestinationControl : public QCameraCaptureDestinationControl
{
    Q_OBJECT
public:
    DirectShowCameraCaptureDestinationControl(DSCameraSession *session);

    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const override;
    QCameraImageCapture::CaptureDestinations captureDestination() const override;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination) override;

private:
    DSCameraSession *m_session;

};

QT_END_NAMESPACE

#endif // DIRECTSHOWCAMERACAPTUREDESTINATIONCONTROL_H
