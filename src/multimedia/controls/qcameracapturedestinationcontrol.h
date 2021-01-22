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

#ifndef QCAMERACAPTUREDESTINATIONCONTROL_H
#define QCAMERACAPTUREDESTINATIONCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qcameraimagecapture.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraCaptureDestinationControl : public QMediaControl
{
    Q_OBJECT
public:
    ~QCameraCaptureDestinationControl();

    virtual bool isCaptureDestinationSupported(QCameraImageCapture::CaptureDestinations destination) const = 0;
    virtual QCameraImageCapture::CaptureDestinations captureDestination() const = 0;
    virtual void setCaptureDestination(QCameraImageCapture::CaptureDestinations destination) = 0;

Q_SIGNALS:
    void captureDestinationChanged(QCameraImageCapture::CaptureDestinations destination);

protected:
    explicit QCameraCaptureDestinationControl(QObject *parent = nullptr);
};

#define QCameraCaptureDestinationControl_iid "org.qt-project.qt.cameracapturedestinationcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraCaptureDestinationControl, QCameraCaptureDestinationControl_iid)

QT_END_NAMESPACE


#endif

