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

#include "qandroidcameracontrol_p.h"
#include "qandroidcamerasession_p.h"
#include "qandroidcameraexposurecontrol_p.h"
#include "qandroidcamerafocuscontrol_p.h"
#include "qandroidcameraimageprocessingcontrol_p.h"
#include "qandroidcameravideorenderercontrol_p.h"
#include <qmediadevicemanager.h>
#include <qcamerainfo.h>
#include <qtimer.h>

QT_BEGIN_NAMESPACE

QAndroidCameraControl::QAndroidCameraControl(QAndroidCameraSession *session)
    : QCameraControl(0)
    , m_cameraSession(session)

{
    connect(m_cameraSession, SIGNAL(statusChanged(QCamera::Status)),
            this, SIGNAL(statusChanged(QCamera::Status)));

    connect(m_cameraSession, SIGNAL(stateChanged(QCamera::State)),
            this, SIGNAL(stateChanged(QCamera::State)));

    connect(m_cameraSession, SIGNAL(error(int,QString)), this, SIGNAL(error(int,QString)));

    m_recalculateTimer = new QTimer(this);
    m_recalculateTimer->setInterval(1000);
    m_recalculateTimer->setSingleShot(true);
    connect(m_recalculateTimer, SIGNAL(timeout()), this, SLOT(onRecalculateTimeOut()));
}

QAndroidCameraControl::~QAndroidCameraControl()
{
    delete m_renderer;
}

void QAndroidCameraControl::setState(QCamera::State state)
{
    m_cameraSession->setState(state);
}

QCamera::State QAndroidCameraControl::state() const
{
    return m_cameraSession->state();
}

QCamera::Status QAndroidCameraControl::status() const
{
    return m_cameraSession->status();
}

void QAndroidCameraControl::setCamera(const QCameraInfo &camera)
{
    int id = 0;
    auto cameras = QMediaDeviceManager::videoInputs();
    for (int i = 0; i < cameras.size(); ++i) {
        if (cameras.at(i) == camera) {
            id = i;
            break;
        }
    }
    m_cameraSession->setSelectedCamera(id);
}


bool QAndroidCameraControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    switch (changeType) {
    case QCameraControl::ImageEncodingSettings:
    case QCameraControl::VideoEncodingSettings:
    case QCameraControl::Viewfinder:
        return true;
    default:
        return false;
    }
}

QCameraFocusControl *QAndroidCameraControl::focusControl()
{
    return m_cameraSession->focusControl();
}

QCameraExposureControl *QAndroidCameraControl::exposureControl()
{
    return m_cameraSession->exposureControl();
}

QCameraImageProcessingControl *QAndroidCameraControl::imageProcessingControl()
{
    return m_cameraSession->imageProcessingControl();
}

void QAndroidCameraControl::setVideoSurface(QAbstractVideoSurface *surface)
{
    if (!m_renderer)
        m_renderer = new QAndroidCameraVideoRendererControl(m_cameraSession);
    m_renderer->setSurface(surface);
}

QT_END_NAMESPACE
