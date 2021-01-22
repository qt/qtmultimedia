/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/


#ifndef QAUDIO_H
#define QAUDIO_H

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtMultimedia/qmultimedia.h>

#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

//QTM_SYNC_HEADER_EXPORT QAudio

// Class forward declaration required for QDoc bug
class QString;
namespace QAudio
{
    enum Error { NoError, OpenError, IOError, UnderrunError, FatalError };
    enum State { ActiveState, SuspendedState, StoppedState, IdleState, InterruptedState };
    enum Mode { AudioInput, AudioOutput };

    enum Role {
        UnknownRole,
        MusicRole,
        VideoRole,
        VoiceCommunicationRole,
        AlarmRole,
        NotificationRole,
        RingtoneRole,
        AccessibilityRole,
        SonificationRole,
        GameRole,
        CustomRole
    };

    enum VolumeScale {
        LinearVolumeScale,
        CubicVolumeScale,
        LogarithmicVolumeScale,
        DecibelVolumeScale
    };

    Q_MULTIMEDIA_EXPORT qreal convertVolume(qreal volume, VolumeScale from, VolumeScale to);
}

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::Error error);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::State state);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::Mode mode);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::Role role);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::VolumeScale role);
#endif

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QAudio::Error)
Q_DECLARE_METATYPE(QAudio::State)
Q_DECLARE_METATYPE(QAudio::Mode)
Q_DECLARE_METATYPE(QAudio::Role)
Q_DECLARE_METATYPE(QAudio::VolumeScale)

#endif // QAUDIO_H
