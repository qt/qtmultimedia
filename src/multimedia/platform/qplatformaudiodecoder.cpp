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

#include "qplatformaudiodecoder_p.h"
#include "qthread.h"

QT_BEGIN_NAMESPACE


/*!
    \class QPlatformAudioDecoder
    \obsolete
    \inmodule QtMultimedia


    \ingroup multimedia_control

    \brief The QPlatformAudioDecoder class provides access to audio decoding
    functionality.

    \preliminary
*/

/*!
    Constructs a new audio decoder control with the given \a parent.
*/
QPlatformAudioDecoder::QPlatformAudioDecoder(QAudioDecoder *parent)
    : QObject(parent),
    q(parent)
{
}

/*!
    \fn QPlatformAudioDecoder::source() const

    Returns the current media source filename, or a null QString if none (or a device)
*/

/*!
    \fn QPlatformAudioDecoder::setSource(const QUrl &fileName)

    Sets the current source to \a fileName.  Changing the source will
    stop any current decoding and discard any buffers.

    Sources are exclusive, so only one can be set.
*/

/*!
    \fn QPlatformAudioDecoder::sourceDevice() const

    Returns the current media source QIODevice, or 0 if none (or a file).
*/

/*!
    \fn QPlatformAudioDecoder::setSourceDevice(QIODevice *device)

    Sets the current source to \a device.  Changing the source will
    stop any current decoding and discard any buffers.

    Sources are exclusive, so only one can be set.
*/

/*!
    \fn QPlatformAudioDecoder::start()

    Starts decoding the current media.

    \sa read()
*/

/*!
    \fn QPlatformAudioDecoder::stop()

    Stops playback of the current media and discards any buffers.

    If successful, the player control will immediately stop decoding.
*/

/*!
    \fn QPlatformAudioDecoder::error(int error, const QString &errorString)

    Signals that an \a error has occurred.  The \a errorString provides a more detailed explanation.
*/

void QPlatformAudioDecoder::error(int error, const QString &errorString)
{
    if (error == m_error && errorString == m_errorString)
        return;
    m_error = QAudioDecoder::Error(error);
    m_errorString = errorString;

    if (m_error != QAudioDecoder::NoError) {
        setIsDecoding(false);
        emit q->error(m_error);
    }
}

/*!
    \fn QPlatformAudioDecoder::bufferAvailableChanged(bool available)

    Signals that the bufferAvailable property has changed to \a available.
*/
void QPlatformAudioDecoder::bufferAvailableChanged(bool available)
{
    if (QThread::currentThread() != q->thread())
        QMetaObject::invokeMethod(q, "bufferAvailableChanged", Qt::QueuedConnection, Q_ARG(bool, available));
    else
        emit q->bufferAvailableChanged(available);
}

/*!
    \fn QPlatformAudioDecoder::bufferReady()

    Signals that a new buffer is ready for reading.
*/
void QPlatformAudioDecoder::bufferReady()
{
    if (QThread::currentThread() != q->thread())
        QMetaObject::invokeMethod(q, "bufferReady", Qt::QueuedConnection);
    else
        emit q->bufferReady();
}

/*!
    \fn QPlatformAudioDecoder::sourceChanged()

    Signals that the current source of the decoder has changed.

    \sa source(), sourceDevice()
*/
void QPlatformAudioDecoder::sourceChanged()
{
    emit q->sourceChanged();
}

/*!
    \fn QPlatformAudioDecoder::formatChanged(const QAudioFormat &format)

    Signals that the current audio format of the decoder has changed to \a format.
*/
void QPlatformAudioDecoder::formatChanged(const QAudioFormat &format)
{
    emit q->formatChanged(format);
}

/*!
    \fn void QPlatformAudioDecoder::finished()

    Signals that the decoding has finished successfully.
    If decoding fails, error signal is emitted instead.

    \sa start(), stop(), error()
*/
void QPlatformAudioDecoder::finished()
{
    setIsDecoding(false);
    emit q->finished();
}

/*!
    \fn void QPlatformAudioDecoder::positionChanged(qint64 position)

    Signals that the current \a position of the decoder has changed.

    \sa durationChanged()
*/
void QPlatformAudioDecoder::positionChanged(qint64 position)
{
    q->positionChanged(position);
}

/*!
    \fn void QPlatformAudioDecoder::durationChanged(qint64 duration)

    Signals that the estimated \a duration of the decoded data has changed.

    \sa positionChanged()
*/
void QPlatformAudioDecoder::durationChanged(qint64 duration)
{
    q->durationChanged(duration);
}

/*!
    \fn QPlatformAudioDecoder::read()
    Attempts to read a buffer from the decoder, without blocking. Returns invalid buffer if there are
    no decoded buffers available, or on error.
*/

/*!
    \fn QPlatformAudioDecoder::position() const
    Returns position (in milliseconds) of the last buffer read from
    the decoder or -1 if no buffers have been read.
*/

/*!
    \fn QPlatformAudioDecoder::duration() const
    Returns total duration (in milliseconds) of the audio stream
    or -1 if not available.
*/

QT_END_NAMESPACE
