/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AVFVIDEODEVICECONTROL_H
#define AVFVIDEODEVICECONTROL_H

#include <QtMultimedia/qvideodeviceselectorcontrol.h>
#include <QtCore/qstringlist.h>

#import <AVFoundation/AVFoundation.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;

class AVFVideoDeviceControl : public QVideoDeviceSelectorControl
{
Q_OBJECT
public:
    AVFVideoDeviceControl(AVFCameraService *service, QObject *parent = 0);
    ~AVFVideoDeviceControl();

    int deviceCount() const;

    QString deviceName(int index) const;
    QString deviceDescription(int index) const;

    int defaultDevice() const;
    int selectedDevice() const;

public Q_SLOTS:
    void setSelectedDevice(int index);

public:
    //device changed since the last createCaptureDevice()
    bool isDirty() const { return m_dirty; }
    AVCaptureDevice *createCaptureDevice();

private:
    AVFCameraService *m_service;

    int m_selectedDevice;
    bool m_dirty;
    QStringList m_devices;
    QStringList m_deviceDescriptions;
};

QT_END_NAMESPACE

#endif
