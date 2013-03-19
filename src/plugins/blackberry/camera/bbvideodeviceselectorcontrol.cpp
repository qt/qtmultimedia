/****************************************************************************
**
** Copyright (C) 2013 Research In Motion
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "bbvideodeviceselectorcontrol.h"

#include "bbcamerasession.h"

#include <QDebug>

QT_BEGIN_NAMESPACE

BbVideoDeviceSelectorControl::BbVideoDeviceSelectorControl(BbCameraSession *session, QObject *parent)
    : QVideoDeviceSelectorControl(parent)
    , m_session(session)
    , m_selected(0)
{
    enumerateDevices(&m_devices, &m_descriptions);

    // pre-select the rear camera
    const int index = m_devices.indexOf(BbCameraSession::cameraIdentifierRear());
    if (index != -1)
        m_selected = index;
}

int BbVideoDeviceSelectorControl::deviceCount() const
{
    return m_devices.count();
}

QString BbVideoDeviceSelectorControl::deviceName(int index) const
{
    if (index < 0 || index >= m_devices.count())
        return QString();

    return QString::fromUtf8(m_devices.at(index));
}

QString BbVideoDeviceSelectorControl::deviceDescription(int index) const
{
    if (index < 0 || index >= m_descriptions.count())
        return QString();

    return m_descriptions.at(index);
}

int BbVideoDeviceSelectorControl::defaultDevice() const
{
    return 0;
}

int BbVideoDeviceSelectorControl::selectedDevice() const
{
    return m_selected;
}

void BbVideoDeviceSelectorControl::enumerateDevices(QList<QByteArray> *devices, QStringList *descriptions)
{
    devices->clear();
    descriptions->clear();

    camera_unit_t cameras[10];

    unsigned int knownCameras = 0;
    const camera_error_t result = camera_get_supported_cameras(10, &knownCameras, cameras);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve supported camera types:" << result;
        return;
    }

    for (unsigned int i = 0; i < knownCameras; ++i) {
        switch (cameras[i]) {
        case CAMERA_UNIT_FRONT:
            devices->append(BbCameraSession::cameraIdentifierFront());
            descriptions->append(tr("Front Camera"));
            break;
        case CAMERA_UNIT_REAR:
            devices->append(BbCameraSession::cameraIdentifierRear());
            descriptions->append(tr("Rear Camera"));
            break;
        case CAMERA_UNIT_DESKTOP:
            devices->append(BbCameraSession::cameraIdentifierDesktop());
            descriptions->append(tr("Desktop Camera"));
            break;
        default:
            break;
        }
    }
}

void BbVideoDeviceSelectorControl::setSelectedDevice(int index)
{
    if (index < 0 || index >= m_devices.count())
        return;

    if (!m_session)
        return;

    const QByteArray device = m_devices.at(index);
    if (device == m_session->device())
        return;

    m_session->setDevice(device);
    m_selected = index;

    emit selectedDeviceChanged(QString::fromUtf8(device));
    emit selectedDeviceChanged(index);
}

QT_END_NAMESPACE
