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

#include "camerabinrecorder.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabincontainer.h"
#include <QtCore/QDebug>

#include <gst/pbutils/encoding-profile.h>

QT_BEGIN_NAMESPACE

CameraBinRecorder::CameraBinRecorder(CameraBinSession *session)
    :QMediaRecorderControl(session),
     m_session(session),
     m_state(QMediaRecorder::StoppedState),
     m_status(QMediaRecorder::UnloadedStatus)
{
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SLOT(updateStatus()));
    connect(m_session, SIGNAL(pendingStateChanged(QCamera::State)), SLOT(updateStatus()));
    connect(m_session, SIGNAL(busyChanged(bool)), SLOT(updateStatus()));

    connect(m_session, SIGNAL(durationChanged(qint64)), SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
}

CameraBinRecorder::~CameraBinRecorder()
{
}

QUrl CameraBinRecorder::outputLocation() const
{
    return m_session->outputLocation();
}

bool CameraBinRecorder::setOutputLocation(const QUrl &sink)
{
    m_session->setOutputLocation(sink);
    return true;
}

QMediaRecorder::State CameraBinRecorder::state() const
{
    return m_state;
}

QMediaRecorder::Status CameraBinRecorder::status() const
{
    return m_status;
}

void CameraBinRecorder::updateStatus()
{
    QCamera::State sessionState = m_session->state();

    QMediaRecorder::State oldState = m_state;
    QMediaRecorder::Status oldStatus = m_status;

    if (sessionState == QCamera::ActiveState &&
            m_session->captureMode().testFlag(QCamera::CaptureVideo)) {

        if (m_state == QMediaRecorder::RecordingState) {
            m_status = QMediaRecorder::RecordingStatus;
        } else {
            m_status = m_session->isBusy() ?
                        QMediaRecorder::FinalizingStatus :
                        QMediaRecorder::LoadedStatus;
        }
    } else {
        if (m_state == QMediaRecorder::RecordingState) {
            m_state = QMediaRecorder::StoppedState;
            m_session->stopVideoRecording();
        }
        m_status = m_session->pendingState() == QCamera::ActiveState ?
                    QMediaRecorder::LoadingStatus :
                    QMediaRecorder::UnloadedStatus;
    }

    if (m_state != oldState)
        emit stateChanged(m_state);

    if (m_status != oldStatus)
        emit statusChanged(m_status);
}

qint64 CameraBinRecorder::duration() const
{
    return m_session->duration();
}


void CameraBinRecorder::applySettings()
{
    //settings are applied during camera startup
}

GstEncodingContainerProfile *CameraBinRecorder::videoProfile()
{
    GstEncodingContainerProfile *containerProfile = m_session->mediaContainerControl()->createProfile();

    if (containerProfile) {
        GstEncodingProfile *audioProfile = m_session->audioEncodeControl()->createProfile();
        GstEncodingProfile *videoProfile = m_session->videoEncodeControl()->createProfile();

        gst_encoding_container_profile_add_profile(containerProfile, audioProfile);
        gst_encoding_container_profile_add_profile(containerProfile, videoProfile);
    }

    return containerProfile;
}

void CameraBinRecorder::setState(QMediaRecorder::State state)
{
    if (m_state == state)
        return;

    QMediaRecorder::State oldState = m_state;
    QMediaRecorder::Status oldStatus = m_status;

    switch (state) {
    case QMediaRecorder::StoppedState:
        m_state = state;
        m_status = QMediaRecorder::FinalizingStatus;
        m_session->stopVideoRecording();
        break;
    case QMediaRecorder::PausedState:
        emit error(QMediaRecorder::ResourceError, tr("QMediaRecorder::pause() is not supported by camerabin2."));
        break;
    case QMediaRecorder::RecordingState:
        if (m_session->state() == QCamera::ActiveState) {
            m_session->recordVideo();
            m_state = state;
            m_status = QMediaRecorder::RecordingStatus;
            emit actualLocationChanged(m_session->outputLocation());
        } else {
            emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));
        }
    }

    if (m_state != oldState)
        emit stateChanged(m_state);

    if (m_status != oldStatus)
        emit statusChanged(m_status);
}

bool CameraBinRecorder::isMuted() const
{
    return m_session->isMuted();
}

void CameraBinRecorder::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

QT_END_NAMESPACE
