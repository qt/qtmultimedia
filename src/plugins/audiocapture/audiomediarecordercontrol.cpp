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

QT_BEGIN_NAMESPACE

AudioMediaRecorderControl::AudioMediaRecorderControl(QObject *parent)
    : QMediaRecorderControl(parent)
{
    m_session = qobject_cast<AudioCaptureSession*>(parent);
    connect(m_session, SIGNAL(positionChanged(qint64)),
            this, SIGNAL(durationChanged(qint64)));
    connect(m_session, SIGNAL(stateChanged(QMediaRecorder::State)),
            this, SIGNAL(stateChanged(QMediaRecorder::State)));
    connect(m_session, SIGNAL(statusChanged(QMediaRecorder::Status)),
            this, SIGNAL(statusChanged(QMediaRecorder::Status)));
    connect(m_session, SIGNAL(actualLocationChanged(QUrl)),
            this, SIGNAL(actualLocationChanged(QUrl)));
    connect(m_session, SIGNAL(error(int,QString)),
            this, SIGNAL(error(int,QString)));
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
    return m_session->state();
}

QMediaRecorder::Status AudioMediaRecorderControl::status() const
{
    return m_session->status();
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
    m_session->setState(state);
}

void AudioMediaRecorderControl::setMuted(bool muted)
{
    if (muted)
        qWarning("Muting the audio recording is not supported.");
}

void AudioMediaRecorderControl::setVolume(qreal volume)
{
    if (!qFuzzyCompare(volume, qreal(1.0)))
        qWarning("Changing the audio recording volume is not supported.");
}

QT_END_NAMESPACE
