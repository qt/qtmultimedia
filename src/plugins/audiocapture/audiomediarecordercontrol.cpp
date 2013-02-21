/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "audiocapturesession.h"
#include "audiomediarecordercontrol.h"

#include <QtCore/qdebug.h>

AudioMediaRecorderControl::AudioMediaRecorderControl(QObject *parent)
    :QMediaRecorderControl(parent)
    , m_state(QMediaRecorder::StoppedState)
    , m_prevStatus(QMediaRecorder::UnloadedStatus)
{
    m_session = qobject_cast<AudioCaptureSession*>(parent);
    connect(m_session,SIGNAL(positionChanged(qint64)),this,SIGNAL(durationChanged(qint64)));
    connect(m_session,SIGNAL(stateChanged(QMediaRecorder::State)), this,SLOT(updateStatus()));
    connect(m_session,SIGNAL(error(int,QString)),this,SLOT(handleSessionError(int,QString)));
}

AudioMediaRecorderControl::~AudioMediaRecorderControl()
{
}

QUrl AudioMediaRecorderControl::outputLocation() const
{
    return m_session->outputLocation();
}

bool AudioMediaRecorderControl::setOutputLocation(const QUrl& sink)
{
    return m_session->setOutputLocation(sink);
}

QMediaRecorder::State AudioMediaRecorderControl::state() const
{
    return (QMediaRecorder::State)m_session->state();
}

QMediaRecorder::Status AudioMediaRecorderControl::status() const
{
    static QMediaRecorder::Status statusTable[3][3] = {
        //Stopped recorder state:
        { QMediaRecorder::LoadedStatus, QMediaRecorder::FinalizingStatus, QMediaRecorder::FinalizingStatus },
        //Recording recorder state:
        { QMediaRecorder::StartingStatus, QMediaRecorder::RecordingStatus, QMediaRecorder::PausedStatus },
        //Paused recorder state:
        { QMediaRecorder::StartingStatus, QMediaRecorder::RecordingStatus, QMediaRecorder::PausedStatus }
    };

    return statusTable[m_state][m_session->state()];
}

qint64 AudioMediaRecorderControl::duration() const
{
    return m_session->position();
}

bool AudioMediaRecorderControl::isMuted() const
{
    return false;
}

qreal AudioMediaRecorderControl::volume() const
{
    //TODO: implement muting and audio gain
    return 1.0;
}

void AudioMediaRecorderControl::setState(QMediaRecorder::State state)
{
    if (m_state == state)
        return;

    m_state = state;

    switch (state) {
    case QMediaRecorder::StoppedState:
        m_session->stop();
        break;
    case QMediaRecorder::PausedState:
        m_session->pause();
        break;
    case QMediaRecorder::RecordingState:
        m_session->record();
        break;
    }

    updateStatus();
}

void AudioMediaRecorderControl::setMuted(bool)
{
}

void AudioMediaRecorderControl::setVolume(qreal volume)
{
    if (!qFuzzyCompare(volume, qreal(1.0)))
        qWarning() << "Media service doesn't support recorder audio gain.";
}

void AudioMediaRecorderControl::updateStatus()
{
    QMediaRecorder::Status newStatus = status();
    if (m_prevStatus != newStatus) {
        m_prevStatus = newStatus;
        emit statusChanged(m_prevStatus);
    }
}

void AudioMediaRecorderControl::handleSessionError(int code, const QString &description)
{
    emit error(code, description);
    setState(QMediaRecorder::StoppedState);
}
