// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QAUDIO_H
#define QAUDIO_H

#if 0
#pragma qt_class(QAudio)
#endif

#include <QtMultimedia/qtmultimediaglobal.h>

#include <QtCore/qmetatype.h>

QT_BEGIN_NAMESPACE

//QTM_SYNC_HEADER_EXPORT QAudio

// Class forward declaration required for QDoc bug
class QString;
namespace QAudio
{
    enum Error { NoError, OpenError, IOError, UnderrunError, FatalError };
    enum State { ActiveState, SuspendedState, StoppedState, IdleState };

    enum VolumeScale {
        LinearVolumeScale,
        CubicVolumeScale,
        LogarithmicVolumeScale,
        DecibelVolumeScale
    };

    Q_MULTIMEDIA_EXPORT float convertVolume(float volume, VolumeScale from, VolumeScale to);
}

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::Error error);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::State state);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::VolumeScale role);
#endif

QT_END_NAMESPACE

#endif // QAUDIO_H
