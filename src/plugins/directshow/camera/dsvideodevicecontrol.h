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

#ifndef DSVIDEODEVICECONTROL_H
#define DSVIDEODEVICECONTROL_H

#include <qvideodeviceselectorcontrol.h>
#include <QStringList>

QT_BEGIN_NAMESPACE
class DSCameraSession;

//QTM_USE_NAMESPACE

using DSVideoDeviceInfo = QPair<QByteArray, QString>;

class DSVideoDeviceControl : public QVideoDeviceSelectorControl
{
    Q_OBJECT
public:
    DSVideoDeviceControl(QObject *parent = nullptr);

    int deviceCount() const override;
    QString deviceName(int index) const override;
    QString deviceDescription(int index) const override;
    int defaultDevice() const override;
    int selectedDevice() const override;

    static const QList<DSVideoDeviceInfo> &availableDevices();

public Q_SLOTS:
    void setSelectedDevice(int index) override;

private:
    static void updateDevices();

    DSCameraSession* m_session;
    int selected;
};

QT_END_NAMESPACE

#endif
