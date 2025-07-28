// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include <qtaudio.h>
#include <qmath.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

#define LOG100 4.60517018599

/*!
    \namespace QtAudio
    \ingroup multimedia-namespaces
    \brief The QtAudio namespace contains enums used by the audio classes.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio
*/

/*!
    \enum QtAudio::Error

    \value NoError         No errors have occurred
    \value OpenError       An error occurred opening the audio device
    \value IOError         An error occurred during read/write of audio device. This can happen when
                           e.g. an external audio interface is disconnected.
    \value UnderrunError   Audio data is not being fed to the audio device at a fast enough rate
    \value FatalError      A non-recoverable error has occurred, the audio device is not usable at this time.
*/

/*!
    \enum QtAudio::State

    \value ActiveState       Audio data is being processed, this state is set after start() is called
                             and while audio data is available to be processed.
    \value SuspendedState    The audio stream is in a suspended state.  Entered after suspend() is called
                             or when another stream takes control of the audio device.  In the later case,
                             a call to resume will return control of the audio device to this stream.  This
                             should usually only be done upon user request.
    \value StoppedState      The audio device is closed, and is not processing any audio data
    \value IdleState         This state indicates that the audio system is temporarily idle due to
                             a buffering condition.
                             For a QAudioSink, IdleState means there isn’t enough data available
                             from the QIODevice to read.
                             For a QAudioSource, IdleState is entered when the ring buffer that
                             feeds the QIODevice becomes full. In that case, any new audio data
                             arriving from the audio interface is discarded until the application
                             reads from the QIODevice, which frees up space in the buffer.
*/

/*!
    \enum QtAudio::VolumeScale

    This enum defines the different audio volume scales.

    \value LinearVolumeScale        Linear scale. \c 0.0 (0%) is silence and \c 1.0 (100%) is full
                                    volume. All Qt Multimedia classes that have an audio volume use
                                    a linear scale.
    \value CubicVolumeScale         Cubic scale. \c 0.0 (0%) is silence and \c 1.0 (100%) is full
                                    volume.
    \value LogarithmicVolumeScale   Logarithmic Scale. \c 0.0 (0%) is silence and \c 1.0 (100%) is
                                    full volume. UI volume controls should usually use a logarithmic
                                    scale.
    \value DecibelVolumeScale       Decibel (dB, amplitude) logarithmic scale. \c -200 is silence
                                    and \c 0 is full volume.

    \sa QtAudio::convertVolume()
*/

namespace QtAudio
{

/*!
    Converts an audio \a volume \a from a volume scale \a to another, and returns the result.

    Depending on the context, different scales are used to represent audio volume. All Qt Multimedia
    classes that have an audio volume use a linear scale, the reason is that the loudness of a
    speaker is controlled by modulating its voltage on a linear scale. The human ear on the other
    hand, perceives loudness in a logarithmic way. Using a logarithmic scale for volume controls
    is therefore appropriate in most applications. The decibel scale is logarithmic by nature and
    is commonly used to define sound levels, it is usually used for UI volume controls in
    professional audio applications. The cubic scale is a computationally cheap approximation of a
    logarithmic scale, it provides more control over lower volume levels.

    The following example shows how to convert the volume value from a slider control before passing
    it to a QMediaPlayer. As a result, the perceived increase in volume is the same when increasing
    the volume slider from 20 to 30 as it is from 50 to 60:

    \snippet multimedia-snippets/audio.cpp Volume conversion

    \sa VolumeScale, QAudioSink::setVolume(), QAudioSource::setVolume(),
    QSoundEffect::setVolume()
*/
float convertVolume(float volume, VolumeScale from, VolumeScale to)
{
    switch (from) {
    case LinearVolumeScale:
        volume = qMax(float(0), volume);
        switch (to) {
        case LinearVolumeScale:
            return volume;
        case CubicVolumeScale:
            return qPow(volume, float(1 / 3.0));
        case LogarithmicVolumeScale:
            return 1 - std::exp(-volume * LOG100);
        case DecibelVolumeScale:
            if (volume < 0.001)
                return float(-200);
            else
                return float(20.0) * std::log10(volume);
        }
        break;
    case CubicVolumeScale:
        volume = qMax(float(0), volume);
        switch (to) {
        case LinearVolumeScale:
            return volume * volume * volume;
        case CubicVolumeScale:
            return volume;
        case LogarithmicVolumeScale:
            return 1 - std::exp(-volume * volume * volume * LOG100);
        case DecibelVolumeScale:
            if (volume < 0.001)
                return float(-200);
            else
                return float(3.0 * 20.0) * std::log10(volume);
        }
        break;
    case LogarithmicVolumeScale:
        volume = qMax(float(0), volume);
        switch (to) {
        case LinearVolumeScale:
            if (volume > 0.99)
                return 1;
            else
                return -std::log(1 - volume) / LOG100;
        case CubicVolumeScale:
            if (volume > 0.99)
                return 1;
            else
                return qPow(-std::log(1 - volume) / LOG100, float(1 / 3.0));
        case LogarithmicVolumeScale:
            return volume;
        case DecibelVolumeScale:
            if (volume < 0.001)
                return float(-200);
            else if (volume > 0.99)
                return 0;
            else
                return float(20.0) * std::log10(-std::log(1 - volume) / LOG100);
        }
        break;
    case DecibelVolumeScale:
        switch (to) {
        case LinearVolumeScale:
            return qPow(float(10.0), volume / float(20.0));
        case CubicVolumeScale:
            return qPow(float(10.0), volume / float(3.0 * 20.0));
        case LogarithmicVolumeScale:
            if (qFuzzyIsNull(volume))
                return 1;
            else
                return 1 - std::exp(-qPow(float(10.0), volume / float(20.0)) * LOG100);
        case DecibelVolumeScale:
            return volume;
        }
        break;
    }

    return volume;
}

} // namespace QtAudio

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

QDebug operator<<(QDebug dbg, QAudio::VolumeScale scale)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();
    switch (scale) {
    case QAudio::LinearVolumeScale:
        dbg << "LinearVolumeScale";
        break;
    case QAudio::CubicVolumeScale:
        dbg << "CubicVolumeScale";
        break;
    case QAudio::LogarithmicVolumeScale:
        dbg << "LogarithmicVolumeScale";
        break;
    case QAudio::DecibelVolumeScale:
        dbg << "DecibelVolumeScale";
        break;
    }
    return dbg;
}

#endif


QT_END_NAMESPACE

