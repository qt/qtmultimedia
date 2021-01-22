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

#ifndef DSIMAGECAPTURECONTROL_H
#define DSIMAGECAPTURECONTROL_H

#include <qcameraimagecapturecontrol.h>
#include "dscamerasession.h"

QT_BEGIN_NAMESPACE

class DSImageCaptureControl : public QCameraImageCaptureControl
{
    Q_OBJECT
public:
    DSImageCaptureControl(DSCameraSession *session);
    ~DSImageCaptureControl() override;

    bool isReadyForCapture() const override;
    int capture(const QString &fileName) override;

    QCameraImageCapture::DriveMode driveMode() const override;
    void setDriveMode(QCameraImageCapture::DriveMode mode) override;

    void cancelCapture() override {}

private:
    DSCameraSession *m_session;
};

QT_END_NAMESPACE

#endif // DSCAPTURECONTROL_H
