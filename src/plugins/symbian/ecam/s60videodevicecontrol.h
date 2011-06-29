/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef S60VIDEODEVICECONTROL_H
#define S60VIDEODEVICECONTROL_H

#include "qvideodevicecontrol.h"

QT_USE_NAMESPACE

class S60CameraControl;
class QString;
class QIcon;

/*
 * Control for providing information of the video device (r. camera) and to
 * enable other camera device (e.g. secondary camera if one exists).
 */
class S60VideoDeviceControl : public QVideoDeviceControl
{
    Q_OBJECT

public: // Constructors & Destructor

    S60VideoDeviceControl(QObject *parent);
    S60VideoDeviceControl(S60CameraControl *control, QObject *parent = 0);
    virtual ~S60VideoDeviceControl();

public: // QVideoDeviceControl

    int deviceCount() const;

    QString deviceName(int index) const;
    QString deviceDescription(int index) const;
    QIcon deviceIcon(int index) const;

    int defaultDevice() const;
    int selectedDevice() const;

public slots: // QVideoDeviceControl

    void setSelectedDevice(int index);

/*
Q_SIGNALS:
void selectedDeviceChanged(int index);
void selectedDeviceChanged(const QString &deviceName);
void devicesChanged();
*/

private: // Data

    S60CameraControl    *m_control;
    int                 m_selectedDevice;
};

#endif // S60VIDEODEVICECONTROL_H
