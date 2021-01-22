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

#ifndef QANDROIDVIDEODEVICESELECTORCONTROL_H
#define QANDROIDVIDEODEVICESELECTORCONTROL_H

#include <qvideodeviceselectorcontrol.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidVideoDeviceSelectorControl : public QVideoDeviceSelectorControl
{
    Q_OBJECT
public:
    explicit QAndroidVideoDeviceSelectorControl(QAndroidCameraSession *session);
    ~QAndroidVideoDeviceSelectorControl();

    int deviceCount() const;

    QString deviceName(int index) const;
    QString deviceDescription(int index) const;

    int defaultDevice() const;
    int selectedDevice() const;
    void setSelectedDevice(int index);

private:
    int m_selectedDevice;

    QAndroidCameraSession *m_cameraSession;
};

QT_END_NAMESPACE

#endif // QANDROIDVIDEODEVICESELECTORCONTROL_H
