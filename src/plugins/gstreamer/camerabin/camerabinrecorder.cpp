/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "camerabinrecorder.h"
#include "camerabinaudioencoder.h"
#include "camerabinvideoencoder.h"
#include "camerabincontainer.h"
#include <QtCore/QDebug>

#include <gst/pbutils/encoding-profile.h>

CameraBinRecorder::CameraBinRecorder(CameraBinSession *session)
    :QMediaRecorderControl(session),
     m_session(session),
     m_state(QMediaRecorder::StoppedState)
{
    connect(m_session, SIGNAL(stateChanged(QCamera::State)), SLOT(updateState()));
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

void CameraBinRecorder::updateState()
{
    if (m_session->state() != QCamera::ActiveState &&
            m_state != QMediaRecorder::StoppedState) {
        m_session->stopVideoRecording();
        emit stateChanged(m_state = QMediaRecorder::StoppedState);
    }
}

qint64 CameraBinRecorder::duration() const
{
    return m_session->duration();
}

void CameraBinRecorder::record()
{
    if (m_session->state() == QCamera::ActiveState) {
        m_session->recordVideo();
        emit stateChanged(m_state = QMediaRecorder::RecordingState);
    } else
        emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));
}

void CameraBinRecorder::pause()
{
    emit error(QMediaRecorder::ResourceError, tr("QMediaRecorder::pause() is not supported by camerabin2."));
}

void CameraBinRecorder::stop()
{
    if (m_session->state() == QCamera::ActiveState) {
        m_session->stopVideoRecording();
        emit stateChanged(m_state = QMediaRecorder::StoppedState);
    }
}

void CameraBinRecorder::applySettings()
{
    GstEncodingContainerProfile *containerProfile = m_session->mediaContainerControl()->createProfile();

    if (containerProfile) {
        GstEncodingProfile *audioProfile = m_session->audioEncodeControl()->createProfile();
        GstEncodingProfile *videoProfile = m_session->videoEncodeControl()->createProfile();

        gst_encoding_container_profile_add_profile(containerProfile, audioProfile);
        gst_encoding_container_profile_add_profile(containerProfile, videoProfile);
    }

    g_object_set (G_OBJECT(m_session->cameraBin()), "video-profile", containerProfile, NULL);
}

bool CameraBinRecorder::isMuted() const
{
    return m_session->isMuted();
}

void CameraBinRecorder::setMuted(bool muted)
{
    m_session->setMuted(muted);
}
