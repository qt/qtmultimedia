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

#include "simulatorvideoinputdevicecontrol.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

using namespace QTM_NAMESPACE;
QSimulatorVideoInputDeviceControl::QSimulatorVideoInputDeviceControl(QObject *parent)
    : QVideoDeviceControl(parent)
    , mSelectedDevice(-1)
{
}

QSimulatorVideoInputDeviceControl::~QSimulatorVideoInputDeviceControl()
{
}

int QSimulatorVideoInputDeviceControl::deviceCount() const
{
    return mDevices.count();
}

QString QSimulatorVideoInputDeviceControl::deviceName(int index) const
{
    if (index >= mDevices.count() || index < 0)
        return QString();
    return mDevices.at(index);
}

QString QSimulatorVideoInputDeviceControl::deviceDescription(int index) const
{
    if (index >= mDevices.count() || index < 0)
        return QString();

    return mDescriptions[index];
}

int QSimulatorVideoInputDeviceControl::defaultDevice() const
{
    if (mDevices.isEmpty())
        return -1;
    return 0;
}

int QSimulatorVideoInputDeviceControl::selectedDevice() const
{
    return mSelectedDevice;
}


void QSimulatorVideoInputDeviceControl::setSelectedDevice(int index)
{
    if (index != mSelectedDevice) {
        mSelectedDevice = index;
        emit selectedDeviceChanged(index);
        emit selectedDeviceChanged(deviceName(index));
    }
}

void QSimulatorVideoInputDeviceControl::updateDeviceList(const QtMobility::QCameraData &data)
{
    mDevices.clear();
    mDescriptions.clear();
    QHashIterator<QString, QCameraData::QCameraDetails> iter(data.cameras);
    while(iter.hasNext()) {
        iter.next();
        mDevices.append(iter.key());
        mDescriptions.append(iter.value().description);
    }
    emit devicesChanged();
}

void QSimulatorVideoInputDeviceControl::addDevice(const QString &name, const QtMobility::QCameraData::QCameraDetails &details)
{
    if (mDevices.contains(name))
        return;

    mDevices.append(name);
    mDescriptions.append(details.description);
    emit devicesChanged();
}

void QSimulatorVideoInputDeviceControl::removeDevice(const QString &name)
{
    int index = mDevices.indexOf(name);
    if (index == -1)
        return;

    mDevices.removeAt(index);
    mDescriptions.removeAt(index);
    if (index == mSelectedDevice)
        setSelectedDevice(defaultDevice());
    emit devicesChanged();
}

void QSimulatorVideoInputDeviceControl::changeDevice(const QString &name, const QtMobility::QCameraData::QCameraDetails &details)
{
    int index = mDevices.indexOf(name);
    if (index == -1)
        return;

    if (mDescriptions.at(index) != details.description)
        mDescriptions[index] = details.description;
}
