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
#ifndef BBCAMERAIMAGECAPTURECONTROL_H
#define BBCAMERAIMAGECAPTURECONTROL_H

#include <qcameraimagecapturecontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraImageCaptureControl : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    explicit BbCameraImageCaptureControl(BbCameraSession *session, QObject *parent = 0);

    bool isReadyForCapture() const override;

    QCameraImageCapture::DriveMode driveMode() const override;
    void setDriveMode(QCameraImageCapture::DriveMode mode) override;

    int capture(const QString &fileName) override;
    void cancelCapture() override;

private:
    BbCameraSession *m_session;
};

QT_END_NAMESPACE

#endif
