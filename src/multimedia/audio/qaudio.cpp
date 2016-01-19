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


#include <qaudio.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

static void qRegisterAudioMetaTypes()
{
    qRegisterMetaType<QAudio::Error>();
    qRegisterMetaType<QAudio::State>();
    qRegisterMetaType<QAudio::Mode>();
    qRegisterMetaType<QAudio::Role>();
}

Q_CONSTRUCTOR_FUNCTION(qRegisterAudioMetaTypes)

/*!
    \namespace QAudio
    \ingroup multimedia-namespaces
    \brief The QAudio namespace contains enums used by the audio classes.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio
*/

/*!
    \enum QAudio::Error

    \value NoError         No errors have occurred
    \value OpenError       An error occurred opening the audio device
    \value IOError         An error occurred during read/write of audio device
    \value UnderrunError   Audio data is not being fed to the audio device at a fast enough rate
    \value FatalError      A non-recoverable error has occurred, the audio device is not usable at this time.
*/

/*!
    \enum QAudio::State

    \value ActiveState     Audio data is being processed, this state is set after start() is called
                           and while audio data is available to be processed.
    \value SuspendedState  The audio device is in a suspended state, this state will only be entered
                           after suspend() is called.
    \value StoppedState    The audio device is closed, and is not processing any audio data
    \value IdleState       The QIODevice passed in has no data and audio system's buffer is empty, this state
                           is set after start() is called and while no audio data is available to be processed.
*/

/*!
    \enum QAudio::Mode

    \value AudioOutput   audio output device
    \value AudioInput    audio input device
*/

/*!
    \enum QAudio::Role

    This enum describes the role of an audio stream.

    \value UnknownRole              The role is unknown or undefined
    \value MusicRole                Music
    \value VideoRole                Soundtrack from a movie or a video
    \value VoiceCommunicationRole   Voice communications, such as telephony
    \value AlarmRole                Alarm
    \value NotificationRole         Notification, such as an incoming e-mail or a chat request
    \value RingtoneRole             Ringtone
    \value AccessibilityRole        For accessibility, such as with a screen reader
    \value SonificationRole         Sonification, such as with user interface sounds
    \value GameRole                 Game audio

    \since 5.6
    \sa QMediaPlayer::setAudioRole()
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAudio::Error error)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (error) {
        case QAudio::NoError:
            dbg << "NoError";
            break;
        case QAudio::OpenError:
            dbg << "OpenError";
            break;
        case QAudio::IOError:
            dbg << "IOError";
            break;
        case QAudio::UnderrunError:
            dbg << "UnderrunError";
            break;
        case QAudio::FatalError:
            dbg << "FatalError";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, QAudio::State state)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (state) {
        case QAudio::ActiveState:
            dbg << "ActiveState";
            break;
        case QAudio::SuspendedState:
            dbg << "SuspendedState";
            break;
        case QAudio::StoppedState:
            dbg << "StoppedState";
            break;
        case QAudio::IdleState:
            dbg << "IdleState";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, QAudio::Mode mode)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (mode) {
        case QAudio::AudioInput:
            dbg << "AudioInput";
            break;
        case QAudio::AudioOutput:
            dbg << "AudioOutput";
            break;
    }
    return dbg;
}

QDebug operator<<(QDebug dbg, QAudio::Role role)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (role) {
    case QAudio::UnknownRole:
        dbg << "UnknownRole";
        break;
    case QAudio::AccessibilityRole:
        dbg << "AccessibilityRole";
        break;
    case QAudio::AlarmRole:
        dbg << "AlarmRole";
        break;
    case QAudio::GameRole:
        dbg << "GameRole";
        break;
    case QAudio::MusicRole:
        dbg << "MusicRole";
        break;
    case QAudio::NotificationRole:
        dbg << "NotificationRole";
        break;
    case QAudio::RingtoneRole:
        dbg << "RingtoneRole";
        break;
    case QAudio::SonificationRole:
        dbg << "SonificationRole";
        break;
    case QAudio::VideoRole:
        dbg << "VideoRole";
        break;
    case QAudio::VoiceCommunicationRole:
        dbg << "VoiceCommunicationRole";
        break;
    }
    return dbg;
}
#endif


QT_END_NAMESPACE

