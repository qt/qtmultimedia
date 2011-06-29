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

#include "qgstreamercameracontrol.h"
#include "qgstreamerimageencode.h"

#include <QtCore/qdebug.h>
#include <QtCore/qfile.h>


QGstreamerCameraControl::QGstreamerCameraControl(QGstreamerCaptureSession *session)
    :QCameraControl(session),
    m_captureMode(QCamera::CaptureStillImage),
    m_session(session),
    m_state(QCamera::UnloadedState),
    m_status(QCamera::UnloadedStatus),
    m_reloadPending(false)

{
    connect(m_session, SIGNAL(stateChanged(QGstreamerCaptureSession::State)),
            this, SLOT(updateStatus()));

    connect(m_session->imageEncodeControl(), SIGNAL(settingsChanged()),
            SLOT(reloadLater()));
    connect(m_session, SIGNAL(viewfinderChanged()),
            SLOT(reloadLater()));
    connect(m_session, SIGNAL(readyChanged(bool)),
            SLOT(reloadLater()));
}

QGstreamerCameraControl::~QGstreamerCameraControl()
{
}

void QGstreamerCameraControl::setCaptureMode(QCamera::CaptureMode mode)
{
    if (m_captureMode == mode)
        return;

    switch (mode) {
    case QCamera::CaptureStillImage:
        m_session->setCaptureMode(QGstreamerCaptureSession::Image);
        break;
    case QCamera::CaptureVideo:
        m_session->setCaptureMode(QGstreamerCaptureSession::AudioAndVideo);
        break;
    }

    emit captureModeChanged(mode);
    updateStatus();
    reloadLater();
}

void QGstreamerCameraControl::setState(QCamera::State state)
{
    if (m_state == state)
        return;

    m_state = state;
    switch (state) {
    case QCamera::UnloadedState:
    case QCamera::LoadedState:
        m_session->setState(QGstreamerCaptureSession::StoppedState);
        break;
    case QCamera::ActiveState:
        //postpone changing to Active if the session is nor ready yet
        if (m_session->isReady()) {
            m_session->setState(QGstreamerCaptureSession::PreviewState);
        } else {
#ifdef CAMEABIN_DEBUG
            qDebug() << "Camera session is not ready yet, postpone activating";
#endif
        }
        break;
    default:
        emit error(QCamera::NotSupportedFeatureError, tr("State not supported."));
    }

    updateStatus();
    emit stateChanged(m_state);
}

QCamera::State QGstreamerCameraControl::state() const
{
    return m_state;
}

void QGstreamerCameraControl::updateStatus()
{
    QCamera::Status oldStatus = m_status;

    switch (m_state) {
    case QCamera::UnloadedState:
        m_status = QCamera::UnloadedStatus;
        break;
    case QCamera::LoadedState:
        m_status = QCamera::LoadedStatus;
        break;
    case QCamera::ActiveState:
        if (m_session->state() == QGstreamerCaptureSession::StoppedState)
            m_status = QCamera::StartingStatus;
        else
            m_status = QCamera::ActiveStatus;
        break;
    }

    if (oldStatus != m_status) {
        //qDebug() << "Status changed:" << m_status;
        emit statusChanged(m_status);
    }
}

void QGstreamerCameraControl::reloadLater()
{
    //qDebug() << "reload pipeline requested";
    if (!m_reloadPending && m_state == QCamera::ActiveState) {
        m_reloadPending = true;
        m_session->setState(QGstreamerCaptureSession::StoppedState);
        QMetaObject::invokeMethod(this, "reloadPipeline", Qt::QueuedConnection);
    }
}

void QGstreamerCameraControl::reloadPipeline()
{
    //qDebug() << "reload pipeline";
    if (m_reloadPending) {
        m_reloadPending = false;
        if (m_state == QCamera::ActiveState && m_session->isReady()) {
            m_session->setState(QGstreamerCaptureSession::PreviewState);
        }
    }
}

bool QGstreamerCameraControl::canChangeProperty(PropertyChangeType changeType, QCamera::Status status) const
{
    Q_UNUSED(status);

    switch (changeType) {
    case QCameraControl::CaptureMode:
    case QCameraControl::ImageEncodingSettings:
    case QCameraControl::VideoEncodingSettings:
    case QCameraControl::Viewfinder:
        return true;
    default:
        return false;
    }
}
