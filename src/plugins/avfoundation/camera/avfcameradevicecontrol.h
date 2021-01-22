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

#ifndef AVFCAMERADEVICECONTROL_H
#define AVFCAMERADEVICECONTROL_H

#include <QtMultimedia/qvideodeviceselectorcontrol.h>
#include <QtCore/qstringlist.h>

#import <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;

class AVFCameraDeviceControl : public QVideoDeviceSelectorControl
{
Q_OBJECT
public:
    AVFCameraDeviceControl(AVFCameraService *service, QObject *parent = nullptr);
    ~AVFCameraDeviceControl();

    int deviceCount() const override;

    QString deviceName(int index) const override;
    QString deviceDescription(int index) const override;

    int defaultDevice() const override;
    int selectedDevice() const override;

public Q_SLOTS:
    void setSelectedDevice(int index) override;

public:
    //device changed since the last createCaptureDevice()
    bool isDirty() const { return m_dirty; }
    AVCaptureDevice *createCaptureDevice();

private:
    AVFCameraService *m_service;

    int m_selectedDevice;
    bool m_dirty;
};

QT_END_NAMESPACE

#endif
