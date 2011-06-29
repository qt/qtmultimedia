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

#include "s60cameraservice.h"
#include "s60mediarecordercontrol.h"
#include "s60cameracontrol.h"
#include "s60videocapturesession.h"

S60MediaRecorderControl::S60MediaRecorderControl(QObject *parent) :
    QMediaRecorderControl(parent)
{
}

S60MediaRecorderControl::S60MediaRecorderControl(S60CameraService *service,
                                                 S60VideoCaptureSession *session,
                                                 QObject *parent):
    QMediaRecorderControl(parent),
    m_state(QMediaRecorder::StoppedState) // Default RecorderState
{
    m_session = session;
    m_service = service;
    m_cameraControl = qobject_cast<S60CameraControl *>(m_service->requestControl(QCameraControl_iid));

    // Connect signals
    connect(m_session, SIGNAL(stateChanged(S60VideoCaptureSession::TVideoCaptureState)),
        this, SLOT(updateState(S60VideoCaptureSession::TVideoCaptureState)));
    connect(m_session, SIGNAL(positionChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
    connect(m_session, SIGNAL(error(int,const QString &)), this, SIGNAL(error(int,const QString &)));
}

S60MediaRecorderControl::~S60MediaRecorderControl()
{
    // Release requested control
    if (m_cameraControl)
        m_service->releaseControl(m_cameraControl);
}

QUrl S60MediaRecorderControl::outputLocation() const
{
    return m_session->outputLocation();
}

bool S60MediaRecorderControl::setOutputLocation(const QUrl& sink)
{
    // Output location can only be set in StoppedState
    if (m_state == QMediaRecorder::StoppedState)
        return m_session->setOutputLocation(sink);

    // Do not signal error, but notify that setting was not effective
    return false;
}

QMediaRecorder::State S60MediaRecorderControl::convertInternalStateToQtState(S60VideoCaptureSession::TVideoCaptureState aState) const
{
    QMediaRecorder::State state;

    switch (aState) {
        case S60VideoCaptureSession::ERecording:
            state = QMediaRecorder::RecordingState;
            break;
        case S60VideoCaptureSession::EPaused:
            state = QMediaRecorder::PausedState;
            break;

        default:
            // All others
            state = QMediaRecorder::StoppedState;
            break;
    }

    return state;
}

void S60MediaRecorderControl::updateState(S60VideoCaptureSession::TVideoCaptureState state)
{
    QMediaRecorder::State newState = convertInternalStateToQtState(state);

    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }
}

QMediaRecorder::State S60MediaRecorderControl::state() const
{
    return m_state;
}

qint64 S60MediaRecorderControl::duration() const
{
    return m_session->position();
}

/*
This method is called after encoder configuration is done.
Encoder can load necessary resources at this point,
to reduce delay before recording is started. Calling this method reduces the
latency when calling record() to start video recording.
*/
void S60MediaRecorderControl::applySettings()
{
    m_session->applyAllSettings();
}

void S60MediaRecorderControl::record()
{
    if (m_state == QMediaRecorder::RecordingState)
        return;

    if (m_cameraControl && m_cameraControl->captureMode() != QCamera::CaptureVideo) {
        emit error(QCamera::CameraError, tr("Video capture mode is not selected."));
        return;
    }

    m_session->startRecording();
}

void S60MediaRecorderControl::pause()
{
    if (m_state != QMediaRecorder::RecordingState) {
        // Discard
        return;
    }

    m_session->pauseRecording();
}

void S60MediaRecorderControl::stop()
{
    if (m_state == QMediaRecorder::StoppedState) {
        // Ignore
        return;
    }

    m_session->stopRecording();
}

bool S60MediaRecorderControl::isMuted() const
{
    return m_session->isMuted();
}

void S60MediaRecorderControl::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

// End of file
