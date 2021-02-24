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

#include "camerabinrecorder.h"
#include "camerabincontrol.h"
#include "qaudiodeviceinfo.h"
#include <QtCore/QDebug>


QT_BEGIN_NAMESPACE

CameraBinRecorder::CameraBinRecorder(CameraBinSession *session)
    :QPlatformMediaRecorder(session),
     m_session(session),
     m_state(QMediaRecorder::StoppedState),
     m_status(QMediaRecorder::UnloadedStatus)
{
    connect(m_session, SIGNAL(statusChanged(QCamera::Status)), SLOT(updateStatus()));
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
    QCamera::Status sessionStatus = m_session->status();

    QMediaRecorder::State oldState = m_state;
    QMediaRecorder::Status oldStatus = m_status;

    if (sessionStatus == QCamera::ActiveStatus &&
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
        m_status = m_session->pendingState() == QCamera::ActiveState
                    && m_session->captureMode().testFlag(QCamera::CaptureVideo)
                ? QMediaRecorder::LoadingStatus
                : QMediaRecorder::UnloadedStatus;
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
    // ######
//    QGStreamerContainerControl *containerControl = m_session->mediaContainerControl();
//    QGStreamerAudioEncoderControl *audioEncoderControl = m_session->audioEncodeControl();
//    QGStreamerVideoEncoderControl *audioEncoderControl = m_session->videoEncodeControl();
//    containerConrol->applySettings(audioEncoderControl, audioEncoderControl);
}

QAudioDeviceInfo CameraBinRecorder::audioInput() const
{
######
}

bool CameraBinRecorder::setAudioInput(const QAudioDeviceInfo &info)
{
    // ### do proper error checking
    m_session->setCaptureDevice(QString::fromLatin1(info.id()));
    return true;
}

GstEncodingContainerProfile *CameraBinRecorder::videoProfile()
{
    return m_session->mediaContainerControl()->fullProfile(m_session->audioEncodeControl(), m_session->videoEncodeControl());
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

        if (m_session->status() != QCamera::ActiveStatus) {
            emit error(QMediaRecorder::ResourceError, tr("Service has not been started"));
        } else {
            m_session->recordVideo();
            m_state = state;
            m_status = QMediaRecorder::RecordingStatus;
            emit actualLocationChanged(m_session->outputLocation());
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

qreal CameraBinRecorder::volume() const
{
    return 1.0;
}

void CameraBinRecorder::setMuted(bool muted)
{
    m_session->setMuted(muted);
}

void CameraBinRecorder::setVolume(qreal volume)
{
    if (!qFuzzyCompare(volume, qreal(1.0)))
        qWarning() << "Media service doesn't support recorder audio gain.";
}

QT_END_NAMESPACE

