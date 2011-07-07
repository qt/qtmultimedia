/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#include "DebugMacros.h"

#include "s60audiomediarecordercontrol.h"
#include "s60audiocapturesession.h"

#include <QtCore/qdebug.h>

S60AudioMediaRecorderControl::S60AudioMediaRecorderControl(QObject *session, QObject *parent)
    :QMediaRecorderControl(parent), m_state(QMediaRecorder::StoppedState)
{
    DP0("S60AudioMediaRecorderControl::S60AudioMediaRecorderControl +++");

    m_session = qobject_cast<S60AudioCaptureSession*>(session);
    connect(m_session, SIGNAL(positionChanged(qint64)), this, SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(stateChanged(S60AudioCaptureSession::TAudioCaptureState)), this, SLOT(updateState(S60AudioCaptureSession::TAudioCaptureState)));
    connect(m_session,SIGNAL(error(int,const QString &)),this,SIGNAL(error(int,const QString &)));

    DP0("S60AudioMediaRecorderControl::S60AudioMediaRecorderControl ---");
}

S60AudioMediaRecorderControl::~S60AudioMediaRecorderControl()
{
    DP0("S60AudioMediaRecorderControl::~S60AudioMediaRecorderControl +++");

    DP0("S60AudioMediaRecorderControl::~S60AudioMediaRecorderControl - - ");
}

QUrl S60AudioMediaRecorderControl::outputLocation() const
{
    DP0("S60AudioMediaRecorderControl::outputLocation");

    return m_session->outputLocation();
}

bool S60AudioMediaRecorderControl::setOutputLocation(const QUrl& sink)
{
    DP0("S60AudioMediaRecorderControl::setOutputLocation");

    return m_session->setOutputLocation(sink);
}

QMediaRecorder::State S60AudioMediaRecorderControl::convertState(S60AudioCaptureSession::TAudioCaptureState aState) const
{
    DP0("S60AudioMediaRecorderControl::convertState +++");

    QMediaRecorder::State state = QMediaRecorder::StoppedState;;
    switch (aState) {
    case S60AudioCaptureSession::ERecording:
        state = QMediaRecorder::RecordingState;
        break;
    case S60AudioCaptureSession::EPaused:
        state = QMediaRecorder::PausedState;
        break;
    case S60AudioCaptureSession::ERecordComplete:
    case S60AudioCaptureSession::ENotInitialized:
    case S60AudioCaptureSession::EOpenCompelete:
    case S60AudioCaptureSession::EInitialized:
        state = QMediaRecorder::StoppedState;
        break;
    }

    DP1("S60AudioMediaRecorderControl::convertState:", state);

    DP0("S60AudioMediaRecorderControl::convertState ---");

    return state;
}

void S60AudioMediaRecorderControl::updateState(S60AudioCaptureSession::TAudioCaptureState aState)
{
    DP0("S60AudioMediaRecorderControl::updateState +++");

    QMediaRecorder::State newState = convertState(aState);
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged(m_state);
    }

    DP0("S60AudioMediaRecorderControl::updateState ---");
}

QMediaRecorder::State S60AudioMediaRecorderControl::state() const
{
    DP0("S60AudioMediaRecorderControl::state");

    return m_state;
}

qint64 S60AudioMediaRecorderControl::duration() const
{
  //  DP0("S60AudioMediaRecorderControl::duration +++");

    return m_session->position();
}

void S60AudioMediaRecorderControl::record()
{
    DP0("S60AudioMediaRecorderControl::record +++");

    m_session->record();

    DP0("S60AudioMediaRecorderControl::record ---");
}

void S60AudioMediaRecorderControl::pause()
{
    DP0("S60AudioMediaRecorderControl::pause +++");

    m_session->pause();

    DP0("S60AudioMediaRecorderControl::pause ---");
}

void S60AudioMediaRecorderControl::stop()
{
    DP0("S60AudioMediaRecorderControl::stop +++");

    m_session->stop();

    DP0("S60AudioMediaRecorderControl::stop ---");
}

bool S60AudioMediaRecorderControl::isMuted() const
{
    DP0("S60AudioMediaRecorderControl::isMuted");

    return m_session->muted();
}

void S60AudioMediaRecorderControl::setMuted(bool muted)
{
    DP0("S60AudioMediaRecorderControl::setMuted +++");

    DP1("S60AudioMediaRecorderControl::setMuted:", muted);

    m_session->mute(muted);

    DP0("S60AudioMediaRecorderControl::setMuted +++");
}
