// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiobufferoutput_p.h"
#include "qmediaplayer.h"
#include "qaudiobuffer.h"

QT_BEGIN_NAMESPACE

/*!
    \class QAudioBufferOutput
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio
    \since 6.8

    \brief The QAudioBufferOutput class is used for capturing audio data provided by \l QMediaPlayer.

    QAudioBufferOutput can be set to QMediaPlayer in order to receive audio buffers
    decoded by the media player. The received audio data can be used for any
    processing or visualization. An audio level meter implementation can be seen
    in the widget based \l{Media Player Example}.

    QAudioBufferOutput is only supported with the FFmpeg backend.

    \sa QMediaPlayer, QMediaPlayer::setAudioBufferOutput, QAudioBuffer
*/

/*!
    Constructs a new QAudioBufferOutput object with \a parent.

    The audio format of output audio buffers will depend on
    the source media file and the inner audio decoder in \l QMediaPlayer.
*/
QAudioBufferOutput::QAudioBufferOutput(QObject *parent)
    : QObject(*new QAudioBufferOutputPrivate, parent)
{
}

/*!
    Constructs a new QAudioBufferOutput object with audio \a format and \a parent.

    If the specified \a format is valid, it will be the format of output
    audio buffers. Otherwise, the format of output audio buffers
    will depend on the source media file and the inner audio decoder in \l QMediaPlayer.
*/
QAudioBufferOutput::QAudioBufferOutput(const QAudioFormat &format, QObject *parent)
    : QObject(*new QAudioBufferOutputPrivate(format), parent)
{
}

/*!
    Destroys the audio buffer output object.
*/
QAudioBufferOutput::~QAudioBufferOutput()
{
    Q_D(QAudioBufferOutput);

    if (d->mediaPlayer)
        d->mediaPlayer->setAudioBufferOutput(nullptr);
}

/*!
    Gets the audio format specified in the constructor.

    If the format is valid, it specifies the format of output oudio buffers.
*/
QAudioFormat QAudioBufferOutput::format() const
{
    Q_D(const QAudioBufferOutput);
    return d->format;
}

/*!
    \fn void QAudioBufferOutput::audioBufferReceived(const QAudioBuffer &buffer)

    Signals that a new audio \a buffer has been received from \l QMediaPlayer.
*/

QT_END_NAMESPACE

#include "moc_qaudiobufferoutput.cpp"
