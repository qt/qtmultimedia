/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qqnxdevicemanager_p.h"
#include "qmediadevicemanager.h"
#include "qcamerainfo_p.h"

#include "private/qnxaudioinput_p.h"
#include "private/qnxaudiooutput_p.h"
#include "private/qnxaudiodeviceinfo_p.h"
#include "bbcamerasession_p.h"

QT_BEGIN_NAMESPACE

static QList<QCameraInfo> enumerateCameras()
{

    camera_unit_t cameras[10];

    unsigned int knownCameras = 0;
    const camera_error_t result = camera_get_supported_cameras(10, &knownCameras, cameras);
    if (result != CAMERA_EOK) {
        qWarning() << "Unable to retrieve supported camera types:" << result;
        return {};
    }

    QList<QCameraInfo> cameras;
    for (unsigned int i = 0; i < knownCameras; ++i) {
        QCameraInfoPrivate *p = new QCameraInfoPrivate;
        switch (cameras[i]) {
        case CAMERA_UNIT_FRONT:
            p->id = BbCameraSession::cameraIdentifierFront();
            p->description = tr("Front Camera");
            p->position = QCameraInfo::FrontFace;
            break;
        case CAMERA_UNIT_REAR:
            p->id = BbCameraSession::cameraIdentifierRear();
            p->description = tr("Rear Camera");
            p->position = QCameraInfo::BackFace;
            break;
        case CAMERA_UNIT_DESKTOP:
            p->id = devices->append(BbCameraSession::cameraIdentifierDesktop();
            p->description = tr("Desktop Camera");
            p->position = QCameraInfo::UnspecifiedPosition;
            break;
        default:
            break;
        }
        if (i == 0)
            p->isDefault = true;
        cameras.append(p->create());
    }
    return cameras;
}

QQnxDeviceManager::QQnxDeviceManager()
    : QMediaPlatformDeviceManager()
{
}

QList<QAudioDeviceInfo> QQnxDeviceManager::audioInputs() const
{
    return { QAudioDeviceInfo(new QnxAudioDeviceInfo("default", QAudio::AudioInput)) };
}

QList<QAudioDeviceInfo> QQnxDeviceManager::audioOutputs() const
{
    return { QAudioDeviceInfo(new QnxAudioDeviceInfo("default", QAudio::AudioOutput)) };
}

QList<QCameraInfo> QQnxDeviceManager::videoInputs() const
{
    return enumerateCameras();
}

QAbstractAudioInput *QQnxDeviceManager::createAudioInputDevice(const QAudioDeviceInfo &deviceInfo)
{
    return new QnxAudioInput();
}

QAbstractAudioOutput *QQnxDeviceManager::createAudioOutputDevice(const QAudioDeviceInfo &deviceInfo)
{
    return new QNxAudioOutput();
}

QT_END_NAMESPACE
