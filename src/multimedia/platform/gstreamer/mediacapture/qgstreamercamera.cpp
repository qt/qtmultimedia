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

#include "qgstreamercamera_p.h"
#include "qgstreamercameraimagecapture_p.h"

#include <qcamerainfo.h>

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>


QGstreamerCamera::QGstreamerCamera(QGstreamerCaptureSession *session)
    : QPlatformCamera(session),
    m_session(session),
    m_reloadPending(false)
{
    connect(m_session, SIGNAL(stateChanged(QGstreamerCaptureSession::State)),
            this, SLOT(updateStatus()));

    connect(m_session->imageCaptureControl(), SIGNAL(settingsChanged()),
            SLOT(reloadLater()));
    connect(m_session, SIGNAL(viewfinderChanged()),
            SLOT(reloadLater()));
    connect(m_session, SIGNAL(readyChanged(bool)),
            SLOT(reloadLater()));

    m_session->setCaptureMode(QGstreamerCaptureSession::AudioAndVideoAndImage);
}

QGstreamerCamera::~QGstreamerCamera() = default;

void QGstreamerCamera::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;
    if (!m_active)
        m_session->setState(QGstreamerCaptureSession::StoppedState);
    else {
        //postpone changing to Active if the session is nor ready yet
        if (m_session->isReady()) {
            m_session->setState(QGstreamerCaptureSession::PreviewState);
        } else {
#ifdef CAMEABIN_DEBUG
            qDebug() << "Camera session is not ready yet, postpone activating";
#endif
        }
    }

    updateStatus();
    emit activeChanged(active);
}

void QGstreamerCamera::setCamera(const QCameraInfo &camera)
{
    m_session->setVideoDevice(camera);
    reloadLater();
}

bool QGstreamerCamera::isActive() const
{
    return m_active;
}

void QGstreamerCamera::updateStatus()
{
    QCamera::Status oldStatus = m_status;

    if (m_active) {
        if (m_session->state() == QGstreamerCaptureSession::StoppedState)
            m_status = QCamera::StartingStatus;
        else
            m_status = QCamera::ActiveStatus;
    } else {
        if (m_session->state() == QGstreamerCaptureSession::StoppedState)
            m_status = QCamera::InactiveStatus;
        else
            m_status = QCamera::StoppingStatus;
    }

    if (oldStatus != m_status) {
        //qDebug() << "Status changed:" << m_status;
        emit statusChanged(m_status);
    }
}

void QGstreamerCamera::reloadLater()
{
    //qDebug() << "reload pipeline requested";
    if (!m_reloadPending && m_active) {
        m_reloadPending = true;
        m_session->setState(QGstreamerCaptureSession::StoppedState);
        QMetaObject::invokeMethod(this, "reloadPipeline", Qt::QueuedConnection);
    }
}

void QGstreamerCamera::reloadPipeline()
{
    //qDebug() << "reload pipeline";
    if (m_reloadPending) {
        m_reloadPending = false;
        if (m_active && m_session->isReady()) {
            m_session->setState(QGstreamerCaptureSession::PreviewState);
        }
    }
}

void QGstreamerCamera::setVideoSurface(QAbstractVideoSurface *surface)
{
    m_session->setVideoPreview(surface);
}
