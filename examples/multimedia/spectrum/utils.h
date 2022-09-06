// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef UTILS_H
#define UTILS_H

#include <QDebug>
#include <QtGlobal>

QT_FORWARD_DECLARE_CLASS(QAudioFormat)

//-----------------------------------------------------------------------------
// Miscellaneous utility functions
//-----------------------------------------------------------------------------

QString formatToString(const QAudioFormat &format);

qreal nyquistFrequency(const QAudioFormat &format);

// Scale PCM value to [-1.0, 1.0]
qreal pcmToReal(qint16 pcm);

// Scale real value in [-1.0, 1.0] to PCM
qint16 realToPcm(qreal real);

// Compile-time calculation of powers of two

template<int N>
class PowerOfTwo
{
public:
    static const int Result = PowerOfTwo<N - 1>::Result * 2;
};

template<>
class PowerOfTwo<0>
{
public:
    static const int Result = 1;
};

//-----------------------------------------------------------------------------
// Debug output
//-----------------------------------------------------------------------------

class NullDebug
{
public:
    template<typename T>
    NullDebug &operator<<(const T &)
    {
        return *this;
    }
};

inline NullDebug nullDebug()
{
    return NullDebug();
}

#ifdef LOG_ENGINE
#    define ENGINE_DEBUG qDebug()
#else
#    define ENGINE_DEBUG nullDebug()
#endif

#ifdef LOG_SPECTRUMANALYSER
#    define SPECTRUMANALYSER_DEBUG qDebug()
#else
#    define SPECTRUMANALYSER_DEBUG nullDebug()
#endif

#ifdef LOG_WAVEFORM
#    define WAVEFORM_DEBUG qDebug()
#else
#    define WAVEFORM_DEBUG nullDebug()
#endif

#endif // UTILS_H
