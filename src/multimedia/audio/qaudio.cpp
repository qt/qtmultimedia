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


#include <qaudio.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

namespace QAudio
{

class RegisterMetaTypes
{
public:
    RegisterMetaTypes()
    {
        qRegisterMetaType<QAudio::Error>();
        qRegisterMetaType<QAudio::State>();
        qRegisterMetaType<QAudio::Mode>();
    }

} _register;

}

/*
    \namespace QAudio
    \brief The QAudio namespace contains enums used by the audio classes.
    \inmodule QtMultimedia
    \ingroup multimedia
*/

/*
    \enum QAudio::Error

    \value NoError         No errors have occurred
    \value OpenError       An error occurred opening the audio device
    \value IOError         An error occurred during read/write of audio device
    \value UnderrunError   Audio data is not being fed to the audio device at a fast enough rate
    \value FatalError      A non-recoverable error has occurred, the audio device is not usable at this time.
*/

/*
    \enum QAudio::State

    \value ActiveState     Audio data is being processed, this state is set after start() is called
                           and while audio data is available to be processed.
    \value SuspendedState  The audio device is in a suspended state, this state will only be entered
                           after suspend() is called.
    \value StoppedState    The audio device is closed, and is not processing any audio data
    \value IdleState       The QIODevice passed in has no data and audio system's buffer is empty, this state
                           is set after start() is called and while no audio data is available to be processed.
*/

/*
    \enum QAudio::Mode

    \value AudioOutput   audio output device
    \value AudioInput    audio input device
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, QAudio::Error error)
{
    QDebug nospace = dbg.nospace();
    switch (error) {
        case QAudio::NoError:
            nospace << "NoError";
            break;
        case QAudio::OpenError:
            nospace << "OpenError";
            break;
        case QAudio::IOError:
            nospace << "IOError";
            break;
        case QAudio::UnderrunError:
            nospace << "UnderrunError";
            break;
        case QAudio::FatalError:
            nospace << "FatalError";
            break;
    }
    return nospace;
}

QDebug operator<<(QDebug dbg, QAudio::State state)
{
    QDebug nospace = dbg.nospace();
    switch (state) {
        case QAudio::ActiveState:
            nospace << "ActiveState";
            break;
        case QAudio::SuspendedState:
            nospace << "SuspendedState";
            break;
        case QAudio::StoppedState:
            nospace << "StoppedState";
            break;
        case QAudio::IdleState:
            nospace << "IdleState";
            break;
    }
    return nospace;
}

QDebug operator<<(QDebug dbg, QAudio::Mode mode)
{
    QDebug nospace = dbg.nospace();
    switch (mode) {
        case QAudio::AudioInput:
            nospace << "AudioInput";
            break;
        case QAudio::AudioOutput:
            nospace << "AudioOutput";
            break;
    }
    return nospace;
}
#endif


QT_END_NAMESPACE

