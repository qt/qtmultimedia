// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiobufferinput.h"
#include "qplatformaudiobufferinput_p.h"
#include "qmediainputencoderinterface_p.h"
#include "qmediaframeinput_p.h"

QT_BEGIN_NAMESPACE

class QAudioBufferInputPrivate : public QMediaFrameInputPrivate
{
public:
    QAudioBufferInputPrivate(QAudioBufferInput *q) : q(q) { }

    bool sendAudioBuffer(const QAudioBuffer &audioBuffer)
    {
        return sendMediaFrame(
                [&]() { emit m_platfromAudioBufferInput->newAudioBuffer(audioBuffer); });
    }

    void initialize(const QAudioFormat &format = {})
    {
        m_platfromAudioBufferInput = std::make_unique<QPlatformAudioBufferInput>(format);
        addUpdateSignal(m_platfromAudioBufferInput.get(),
                        &QPlatformAudioBufferInput::encoderUpdated);
    }

    void uninitialize()
    {
        m_platfromAudioBufferInput.reset();

        if (captureSession())
            captureSession()->setAudioBufferInput(nullptr);
    }

    QMediaCaptureSession *session() const { return m_captureSession; }

    QPlatformAudioBufferInput *platfromAudioBufferInput() const
    {
        return m_platfromAudioBufferInput.get();
    }

private:
    void updateCaptureSessionConnections(QMediaCaptureSession *prevSession,
                                         QMediaCaptureSession *newSession) override
    {
        if (prevSession)
            removeUpdateSignal(prevSession, &QMediaCaptureSession::audioOutputChanged);

        if (newSession)
            addUpdateSignal(newSession, &QMediaCaptureSession::audioOutputChanged);
    }

    bool checkIfCanSendMediaFrame() const override
    {
        if (auto encoderInterface = m_platfromAudioBufferInput->encoderInterface())
            return encoderInterface->canPushFrame();

        // Not implemented yet
        // return captureSession()->audioOutput() != nullptr;
        return false;
    }

    void emitReadyToSendMediaFrame() override { emit q->readyToSendAudioBuffer(); }

private:
    QAudioBufferInput *q = nullptr;
    QMediaCaptureSession *m_captureSession = nullptr;
    std::unique_ptr<QPlatformAudioBufferInput> m_platfromAudioBufferInput;
};

/*!
    \class QAudioBufferInput
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_audio
    \since 6.8

    \brief The QAudioBufferInput class is used for providing custom audio buffers
    to \l QMediaRecorder through \l QMediaCaptureSession.

    QAudioBufferInput is only supported with the FFmpeg backend.

    Custom audio buffers can be recorded by connecting a \l QAudioBufferInput and a
    \l QMediaRecorder to a \l QMediaCaptureSession. For a pull mode implementation,
    call \l sendAudioBuffer() in response to the \l readyToSendAudioBuffer() signal.
    In the snippet below this is done by connecting the signal to a slot in a custom
    media generator class. The slot function emits another signal with a new audio
    buffer, which is connected to  \l sendAudioBuffer():

    \snippet custommediainputsnippets/custommediainputsnippets.cpp QAudioBufferInput setup

    Here's a minimal implementation of the slot function that provides audio buffers:

    \snippet custommediainputsnippets/custommediainputsnippets.cpp nextAudioBuffer()

    For more details see readyToSendAudioBuffer() and sendAudioBuffer().

    \sa QMediaRecorder and QMediaCaptureSession.
*/

/*!
    Constructs a new QAudioBufferInput object with \a parent.
*/
QAudioBufferInput::QAudioBufferInput(QObject *parent) : QAudioBufferInput({}, parent) { }

/*!
    Constructs a new QAudioBufferInput object with audio \a format and \a parent.

    The specified \a format will work as a hint for the initialization of the matching
    audio encoder upon invoking \l QMediaRecorder::record().
    If the format is not specified or not valid, the audio encoder will be initialized
    upon sending the first audio buffer.

    We recommend specifying the format if you know in advance what kind of audio buffers
    you're going to send.
*/
QAudioBufferInput::QAudioBufferInput(const QAudioFormat &format, QObject *parent)
    : QObject(*new QAudioBufferInputPrivate(this), parent)
{
    Q_D(QAudioBufferInput);
    d->initialize(format);
}

/*!
    Destroys the object.
 */
QAudioBufferInput::~QAudioBufferInput()
{
    Q_D(QAudioBufferInput);
    d->uninitialize();
}

/*!
    Sends \l QAudioBuffer to \l QMediaRecorder through \l QMediaCaptureSession.

    Returns \c true if the specified \a audioBuffer has been sent successfully
    to the destination. Returns \c false, if the buffer hasn't been sent,
    which can happen if the instance is not assigned to
    \l QMediaCaptureSession, the session doesn't have a media recorder,
    the media recorder is not started or its queue is full.
    The \l readyToSendAudioBuffer() signal will be emitted as soon as
    the destination is able to handle a new audio buffer.

    Sending of an empty audio buffer is treated by \l QMediaRecorder
    as an end of the input stream. QMediaRecorder stops the recording
    automatically if \l QMediaRecorder::autoStop is \c true and
    all the inputs have reported the end of the stream.
*/
bool QAudioBufferInput::sendAudioBuffer(const QAudioBuffer &audioBuffer)
{
    Q_D(QAudioBufferInput);
    return d->sendAudioBuffer(audioBuffer);
}

/*!
    Returns the audio format that was specified upon construction of the audio buffer input.
*/
QAudioFormat QAudioBufferInput::format() const
{
    Q_D(const QAudioBufferInput);
    return d->platfromAudioBufferInput()->audioFormat();
}

/*!
    Returns the capture session this audio buffer input is connected to, or
    a \c nullptr if the audio buffer input is not connected to a capture session.

    Use QMediaCaptureSession::setAudioBufferInput() to connect
    the audio buffer input to a session.
*/
QMediaCaptureSession *QAudioBufferInput::captureSession() const
{
    Q_D(const QAudioBufferInput);
    return d->captureSession();
}

void QAudioBufferInput::setCaptureSession(QMediaCaptureSession *captureSession)
{
    Q_D(QAudioBufferInput);
    d->setCaptureSession(captureSession);
}

QPlatformAudioBufferInput *QAudioBufferInput::platformAudioBufferInput() const
{
    Q_D(const QAudioBufferInput);
    return d->platfromAudioBufferInput();
}

/*!
    \fn void QAudioBufferInput::readyToSendAudioBuffer()

    Signals that a new audio buffer can be sent to the audio buffer input.
    After receiving the signal, if you have audio date to be sent, invoke \l sendAudioBuffer
    once or in a loop until it returns \c false.

    \sa sendAudioBuffer()
*/

QT_END_NAMESPACE
