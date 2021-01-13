/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmediadevicemanager.h"
#include "private/qmediaplatformintegration_p.h"
#include "private/qmediaplatformdevicemanager_p.h"

#include <qaudiodeviceinfo.h>
#include <qcamerainfo.h>

QT_BEGIN_NAMESPACE

class QMediaDeviceManagerPrivate
{
public:
    QMediaDeviceManagerPrivate()
        : manager(new QMediaDeviceManager)
    {
        pmanager = QMediaPlatformIntegration::instance()->deviceManager();
        pmanager->setDeviceManager(manager);
    }
    ~QMediaDeviceManagerPrivate()
    {
        delete manager;
        manager = nullptr;
    }

    QMediaDeviceManager *manager = nullptr;
    QMediaPlatformDeviceManager *pmanager = nullptr;

} priv;

/*!
    \class QMediaDeviceManager
    \brief The QMediaDeviceManager class provides information about available
    multimedia input and output devices.
    \ingroup multimedia
    \inmodule QtMultimedia

    The QMediaDeviceManager class helps in managing the available multimedia
    input and output devices. It manages three types of devices:
    \list
    \li Audio input devices (Microphones)
    \li Audio output devices (Speakers, Headsets)
    \li Video input devices (Cameras)
    \endlist

    QMediaDeviceManager allows listing all available devices and will emit
    signals when the list of available devices has changed.

    While using the default input and output devices is often sufficient for
    playing back or recording multimedia, there is often a need to explicitly
    select the device to be used.

    QMediaDeviceManager is a singleton object and all getters are thread-safe.
*/

/*!
    Returns the device manager instance.
*/
QMediaDeviceManager *QMediaDeviceManager::instance()
{
    return priv.manager;
}

QList<QAudioDeviceInfo> QMediaDeviceManager::audioInputs()
{
    return priv.pmanager->audioInputs();
}

QList<QAudioDeviceInfo> QMediaDeviceManager::audioOutputs()
{
    return priv.pmanager->audioOutputs();
}

QList<QCameraInfo> QMediaDeviceManager::videoInputs()
{
    return priv.pmanager->videoInputs();
}

QAudioDeviceInfo QMediaDeviceManager::defaultAudioInput()
{
    const auto inputs = audioInputs();
    for (auto info : inputs)
        if (info.isDefault())
            return info;
    return inputs.value(0);
}

QAudioDeviceInfo QMediaDeviceManager::defaultAudioOutput()
{
    const auto outputs = audioOutputs();
    for (auto info : outputs)
        if (info.isDefault())
            return info;
    return outputs.value(0);
}

QCameraInfo QMediaDeviceManager::defaultVideInput()
{
    const auto inputs = videoInputs();
    for (auto info : inputs)
        if (info.isDefault())
            return info;
    return inputs.value(0);
}

/*!
    \internal
*/
QMediaDeviceManager::QMediaDeviceManager()
{

}

/*!
    \internal
*/
QMediaDeviceManager::~QMediaDeviceManager()
{

}


QT_END_NAMESPACE

#include "moc_qmediadevicemanager.cpp"
