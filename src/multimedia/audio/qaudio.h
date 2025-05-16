// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#ifndef QAUDIO_H
#define QAUDIO_H

#if 0
#pragma qt_class(QAudio)
#endif

#include <QtMultimedia/qtmultimediaglobal.h>

QT_BEGIN_NAMESPACE

// ### Qt7: Remove the QAudio namespace
#if defined(Q_QDOC)
namespace QtAudio
#else
namespace QAudio
#endif
{
enum Error
{
    NoError,
    OpenError,
    IOError,
    UnderrunError,
    FatalError
};
enum State
{
    ActiveState,
    SuspendedState,
    StoppedState,
    IdleState
};

enum VolumeScale
{
    LinearVolumeScale,
    CubicVolumeScale,
    LogarithmicVolumeScale,
    DecibelVolumeScale
};

Q_MULTIMEDIA_EXPORT float convertVolume(float volume, VolumeScale from, VolumeScale to);

} // namespace QtAudio

#if !defined(Q_QDOC)
namespace QtAudio = QAudio;
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QtAudio::Error error);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QtAudio::State state);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QtAudio::VolumeScale role);
#endif

QT_END_NAMESPACE

#endif // QAUDIO_H
