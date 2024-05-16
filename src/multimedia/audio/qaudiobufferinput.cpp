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

    void initialize()
    {
        m_platfromAudioBufferInput = std::make_unique<QPlatformAudioBufferInput>();
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

    \sa QMediaRecorder, QMediaCaptureSession
*/

/*!
    Constructs a new QAudioBufferInput object with \a parent.
*/
QAudioBufferInput::QAudioBufferInput(QObject *parent)
    : QObject(*new QAudioBufferInputPrivate(this), parent)
{
    Q_D(QAudioBufferInput);
    d->initialize();
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
    The signal \l readyToSendAudiobuffer will be sent as soon as
    the destination is able to handle a new audio buffer.
*/
bool QAudioBufferInput::sendAudioBuffer(const QAudioBuffer &audioBuffer)
{
    Q_D(QAudioBufferInput);
    return d->sendAudioBuffer(audioBuffer);
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
