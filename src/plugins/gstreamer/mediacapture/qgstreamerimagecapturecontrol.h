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


#ifndef QGSTREAMERIMAGECAPTURECONTROL_H
#define QGSTREAMERIMAGECAPTURECONTROL_H

#include <qcameraimagecapturecontrol.h>
#include "qgstreamercapturesession.h"

QT_BEGIN_NAMESPACE

class QGstreamerImageCaptureControl : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    QGstreamerImageCaptureControl(QGstreamerCaptureSession *session);
    virtual ~QGstreamerImageCaptureControl();

    QCameraImageCapture::DriveMode driveMode() const override { return QCameraImageCapture::SingleImageCapture; }
    void setDriveMode(QCameraImageCapture::DriveMode) override {}

    bool isReadyForCapture() const override;
    int capture(const QString &fileName) override;
    void cancelCapture() override;

private slots:
    void updateState();

private:
    QGstreamerCaptureSession *m_session;
    bool m_ready;
    int m_lastId;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAPTURECORNTROL_H
