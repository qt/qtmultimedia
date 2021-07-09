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

#include <private/qtmultimediaglobal_p.h>
#include "qaudiosystem_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformAudioSink
    \brief The QPlatformAudioSink class is a base class for audio backends.

    \ingroup multimedia
    \inmodule QtMultimedia

    QPlatformAudioSink implements audio functionality for
    QAudioSink, i.e., QAudioSink routes function calls to
    QPlatformAudioSink. For a description of the functionality that
    is implemented, see the QAudioSink class and function
    descriptions.

    \sa QAudioSink
*/

/*!
    \fn void QPlatformAudioSink::start(QIODevice* device)
    Uses the \a device as the QIODevice to transfer data.
*/

/*!
    \fn QIODevice* QPlatformAudioSink::start()
    Returns a pointer to the QIODevice being used to handle
    the data transfer. This QIODevice can be used to write() audio data directly.
*/

/*!
    \fn void QPlatformAudioSink::stop()
    Stops the audio output.
*/

/*!
    \fn void QPlatformAudioSink::reset()
    Drops all audio data in the buffers, resets buffers to zero.
*/

/*!
    \fn void QPlatformAudioSink::suspend()
    Stops processing audio data, preserving buffered audio data.
*/

/*!
    \fn void QPlatformAudioSink::resume()
    Resumes processing audio data after a suspend()
*/

/*!
    \fn qsizetype QPlatformAudioSink::bytesFree() const
    Returns the free space available in bytes in the audio buffer.
*/

/*!
    \fn void QPlatformAudioSink::setBufferSize(qsizetype value)
    Sets the audio buffer size to \a value in bytes.
*/

/*!
    \fn qsizetype QPlatformAudioSink::bufferSize() const
    Returns the audio buffer size in bytes.
*/

/*!
    \fn qint64 QPlatformAudioSink::processedUSecs() const
    Returns the amount of audio data processed since start() was called in milliseconds.
*/

/*!
    \fn QAudio::Error QPlatformAudioSink::error() const
    Returns the error state.
*/

/*!
    \fn QAudio::State QPlatformAudioSink::state() const
    Returns the state of audio processing.
*/

/*!
    \fn void QPlatformAudioSink::setFormat(const QAudioFormat& fmt)
    Set the QAudioFormat to use to \a fmt.
    Setting the format is only allowable while in QAudio::StoppedState.
*/

/*!
    \fn QAudioFormat QPlatformAudioSink::format() const
    Returns the QAudioFormat being used.
*/

/*!
    \fn void QPlatformAudioSink::setVolume(qreal volume)
    Sets the volume.
    Where \a volume is between 0.0 and 1.0.
*/

/*!
    \fn qreal QPlatformAudioSink::volume() const
    Returns the volume in the range 0.0 and 1.0.
*/

/*!
    \fn QPlatformAudioSink::errorChanged(QAudio::Error error)
    This signal is emitted when the \a error state has changed.
*/

/*!
    \fn QPlatformAudioSink::stateChanged(QAudio::State state)
    This signal is emitted when the device \a state has changed.
*/

/*!
    \class QPlatformAudioSource
    \brief The QPlatformAudioSource class provides access for QAudioSource to access the audio
    device provided by the plugin.

    \ingroup multimedia
    \inmodule QtMultimedia

    QAudioDeviceInput keeps an instance of QPlatformAudioSource and
    routes calls to functions of the same name to QPlatformAudioSource.
    This means that it is QPlatformAudioSource that implements the
    audio functionality. For a description of the functionality, see
    the QAudioSource class description.

    \sa QAudioSource
*/

/*!
    \fn void QPlatformAudioSource::start(QIODevice* device)
    Uses the \a device as the QIODevice to transfer data.
*/

/*!
    \fn QIODevice* QPlatformAudioSource::start()
    Returns a pointer to the QIODevice being used to handle
    the data transfer. This QIODevice can be used to read() audio data directly.
*/

/*!
    \fn void QPlatformAudioSource::stop()
    Stops the audio input.
*/

/*!
    \fn void QPlatformAudioSource::reset()
    Drops all audio data in the buffers, resets buffers to zero.
*/

/*!
    \fn void QPlatformAudioSource::suspend()
    Stops processing audio data, preserving buffered audio data.
*/

/*!
    \fn void QPlatformAudioSource::resume()
    Resumes processing audio data after a suspend().
*/

/*!
    \fn qsizetype QPlatformAudioSource::bytesReady() const
    Returns the amount of audio data available to read in bytes.
*/

/*!
    \fn void QPlatformAudioSource::setBufferSize(qsizetype value)
    Sets the audio buffer size to \a value in milliseconds.
*/

/*!
    \fn qsizetype QPlatformAudioSource::bufferSize() const
    Returns the audio buffer size in milliseconds.
*/

/*!
    \fn qint64 QPlatformAudioSource::processedUSecs() const
    Returns the amount of audio data processed since start() was called in milliseconds.
*/

/*!
    \fn QAudio::Error QPlatformAudioSource::error() const
    Returns the error state.
*/

/*!
    \fn QAudio::State QPlatformAudioSource::state() const
    Returns the state of audio processing.
*/

/*!
    \fn void QPlatformAudioSource::setFormat(const QAudioFormat& fmt)
    Set the QAudioFormat to use to \a fmt.
    Setting the format is only allowable while in QAudio::StoppedState.
*/

/*!
    \fn QAudioFormat QPlatformAudioSource::format() const
    Returns the QAudioFormat being used
*/

/*!
    \fn QPlatformAudioSource::errorChanged(QAudio::Error error)
    This signal is emitted when the \a error state has changed.
*/

/*!
    \fn QPlatformAudioSource::stateChanged(QAudio::State state)
    This signal is emitted when the device \a state has changed.
*/

QT_END_NAMESPACE

#include "moc_qaudiosystem_p.cpp"
