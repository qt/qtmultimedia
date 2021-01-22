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
#ifndef BBVIDEODEVICESELECTORCONTROL_H
#define BBVIDEODEVICESELECTORCONTROL_H

#include <qvideodeviceselectorcontrol.h>
#include <QStringList>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbVideoDeviceSelectorControl : public QVideoDeviceSelectorControl
{
    Q_OBJECT
public:
    explicit BbVideoDeviceSelectorControl(BbCameraSession *session, QObject *parent = 0);

    int deviceCount() const override;
    QString deviceName(int index) const override;
    QString deviceDescription(int index) const override;
    int defaultDevice() const override;
    int selectedDevice() const override;

    static void enumerateDevices(QList<QByteArray> *devices, QStringList *descriptions);

public Q_SLOTS:
    void setSelectedDevice(int index) override;

private:
    BbCameraSession* m_session;

    QList<QByteArray> m_devices;
    QStringList m_descriptions;

    int m_default;
    int m_selected;
};

QT_END_NAMESPACE

#endif
