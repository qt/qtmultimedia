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

#ifndef AVFCAPTUREDESTINATIONCONTROL_H
#define AVFCAPTUREDESTINATIONCONTROL_H

#include <qcameracapturedestinationcontrol.h>

QT_BEGIN_NAMESPACE

class AVFCaptureDestinationControl : public QCameraCaptureDestinationControl
{
public:
    AVFCaptureDestinationControl() = default;
    ~AVFCaptureDestinationControl() = default;

    bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const override;
    QCameraImageCapture::CaptureDestinations captureDestination() const override;
    void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination) override;

private:
    QCameraImageCapture::CaptureDestinations m_destination = QCameraImageCapture::CaptureToFile;
};

QT_END_NAMESPACE

#endif // AVFCAPTUREDESTINATIONCONTROL_H
