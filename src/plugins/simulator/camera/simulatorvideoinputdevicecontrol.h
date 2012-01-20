/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSIMULATORVIDEOINPUTDEVICECONTROL_H
#define QSIMULATORVIDEOINPUTDEVICECONTROL_H

#include <qvideodevicecontrol.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>

#include "../qsimulatormultimediaconnection_p.h"

QT_USE_NAMESPACE

class QSimulatorVideoInputDeviceControl : public QVideoDeviceControl
{
Q_OBJECT
public:
    QSimulatorVideoInputDeviceControl(QObject *parent);
    ~QSimulatorVideoInputDeviceControl();

    int deviceCount() const;

    QString deviceName(int index) const;
    QString deviceDescription(int index) const;

    int defaultDevice() const;
    int selectedDevice() const;

    void updateDeviceList(const QtMobility::QCameraData &data);

public Q_SLOTS:
    void setSelectedDevice(int index);
    void addDevice(const QString &name, const QtMobility::QCameraData::QCameraDetails &details);
    void removeDevice(const QString &name);
    void changeDevice(const QString &name, const QtMobility::QCameraData::QCameraDetails &details);

private:
    int mSelectedDevice;
    QList<QString> mDevices;
    QList<QString> mDescriptions;
};

#endif // QSIMULATORVIDEOINPUTDEVICECONTROL_H
