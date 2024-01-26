// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#define QT_MULTIMEDIA_BUILD_REMOVED_API

#include <QtMultimedia/qtmultimediaglobal.h>

QT_USE_NAMESPACE

#if QT_MULTIMEDIA_REMOVED_SINCE(6, 7)

// implement removed functions from qaudio.h
#include <QtMultimedia/qaudio.h>
#include <qdebug.h>

namespace QAudio
{
    enum VolumeScale {};
    enum Error {};
    enum State {};
    Q_MULTIMEDIA_EXPORT float convertVolume(float volume, QAudio::VolumeScale from, QAudio::VolumeScale to)
    {
        return QtAudio::convertVolume(volume, static_cast<QtAudio::VolumeScale>(from),
                                              static_cast<QtAudio::VolumeScale>(to));
    }
}

#ifndef QT_NO_DEBUG_STREAM
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::Error error)
{
    return dbg << static_cast<QtAudio::Error>(error);
}
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::State state)
{
    return dbg << static_cast<QtAudio::State>(state);
}
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug dbg, QAudio::VolumeScale role)
{
    return dbg << static_cast<QtAudio::VolumeScale>(role);
}
#endif

#include <QtMultimedia/qaudiosink.h>

QAudio::Error QAudioSink::error() const
{
    return static_cast<QAudio::Error>(error(QT6_CALL_NEW_OVERLOAD));
}

QAudio::State QAudioSink::state() const
{
    return static_cast<QAudio::State>(state(QT6_CALL_NEW_OVERLOAD));
}

#include <QtMultimedia/qaudiosource.h>

QAudio::Error QAudioSource::error() const
{
    return static_cast<QAudio::Error>(error(QT6_CALL_NEW_OVERLOAD));
}

QAudio::State QAudioSource::state() const
{
    return static_cast<QAudio::State>(state(QT6_CALL_NEW_OVERLOAD));
}

// #include "qotherheader.h"
// // implement removed functions from qotherheader.h
// order sections alphabetically

#endif // QT_MULTIMEDIA_REMOVED_SINCE(6, 7)
