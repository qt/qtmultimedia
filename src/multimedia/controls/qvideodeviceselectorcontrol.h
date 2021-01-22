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

#ifndef QVIDEODEVICESELECTORCONTROL_H
#define QVIDEODEVICESELECTORCONTROL_H

#include <QtMultimedia/qmediacontrol.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QVideoDeviceSelectorControl : public QMediaControl
{
    Q_OBJECT

public:
    virtual ~QVideoDeviceSelectorControl();

    virtual int deviceCount() const = 0;

    virtual QString deviceName(int index) const = 0;
    virtual QString deviceDescription(int index) const = 0;

    virtual int defaultDevice() const = 0;
    virtual int selectedDevice() const = 0;

public Q_SLOTS:
    virtual void setSelectedDevice(int index) = 0;

Q_SIGNALS:
    void selectedDeviceChanged(int index);
    void selectedDeviceChanged(const QString &name);
    void devicesChanged();

protected:
    explicit QVideoDeviceSelectorControl(QObject *parent = nullptr);
};

#define QVideoDeviceSelectorControl_iid "org.qt-project.qt.videodeviceselectorcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QVideoDeviceSelectorControl, QVideoDeviceSelectorControl_iid)

QT_END_NAMESPACE

#endif // QVIDEODEVICESELECTORCONTROL_H
