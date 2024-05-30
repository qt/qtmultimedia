// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframeinput.h"
#include "qmediaframeinput_p.h"
#include "qmediainputencoderinterface_p.h"
#include "qplatformvideoframeinput_p.h"

QT_BEGIN_NAMESPACE

class QVideoFrameInputPrivate : public QMediaFrameInputPrivate
{
public:
    QVideoFrameInputPrivate(QVideoFrameInput *q) : q(q) { }

    bool sendVideoFrame(const QVideoFrame &frame)
    {
        return sendMediaFrame([&]() { emit m_platfromVideoFrameInput->newVideoFrame(frame); });
    }

    void initialize()
    {
        m_platfromVideoFrameInput = std::make_unique<QPlatformVideoFrameInput>();
        addUpdateSignal(m_platfromVideoFrameInput.get(), &QPlatformVideoFrameInput::encoderUpdated);
    }

    void uninitialize()
    {
        m_platfromVideoFrameInput.reset();

        if (captureSession())
            captureSession()->setVideoFrameInput(nullptr);
    }

    QPlatformVideoFrameInput *platfromVideoFrameInput() const
    {
        return m_platfromVideoFrameInput.get();
    }

protected:
    void updateCaptureSessionConnections(QMediaCaptureSession *prevSession,
                                         QMediaCaptureSession *newSession) override
    {
        if (prevSession)
            removeUpdateSignal(prevSession, &QMediaCaptureSession::videoOutputChanged);

        if (newSession)
            addUpdateSignal(newSession, &QMediaCaptureSession::videoOutputChanged);
    }

    bool checkIfCanSendMediaFrame() const override
    {
        if (auto encoderInterface = m_platfromVideoFrameInput->encoderInterface())
            return encoderInterface->canPushFrame();

        return captureSession()->videoOutput() || captureSession()->videoSink();
    }

    void emitReadyToSendMediaFrame() override { emit q->readyToSendVideoFrame(); }

private:
    QVideoFrameInput *q = nullptr;
    std::unique_ptr<QPlatformVideoFrameInput> m_platfromVideoFrameInput;
};

/*!
    \class QVideoFrameInput
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video
    \since 6.8

    \brief The QVideoFrameInput class is used for providing custom video frames
    to \l QMediaRecorder or a video output through \l QMediaCaptureSession.

    \sa QMediaRecorder, QMediaCaptureSession, QVideoSink
*/

/*!
    Constructs a new QVideoFrameInput object with \a parent.
*/
QVideoFrameInput::QVideoFrameInput(QObject *parent)
    : QObject(*new QVideoFrameInputPrivate(this), parent)
{
    Q_D(QVideoFrameInput);
    d->initialize();
}

/*!
    Destroys the object.
 */
QVideoFrameInput::~QVideoFrameInput()
{
    Q_D(QVideoFrameInput);
    d->uninitialize();
}

/*!
    Sends \l QVideoFrame to \l QMediaRecorder or a video output
    through \l QMediaCaptureSession.

    Returns \c true if the specified \a frame has been sent successfully
    to the destination. Returns \c false, if the frame hasn't been sent,
    which can happen if the instance is not assigned to
    \l QMediaCaptureSession, the session doesn't have video outputs or
    a media recorder, the media recorder is not started or its queue is full.
    The signal \l readyToSendVideoFrame will be sent as soon as
    the destination is able to handle a new frame.

    Sending of an empty video frame is treated by \l QMediaRecorder
    as an end of the input stream. QMediaRecorder stops the recording
    automatically if \l QMediaRecorder::autoStop is \c true and
    all the inputs have reported the end of the stream.
*/
bool QVideoFrameInput::sendVideoFrame(const QVideoFrame &frame)
{
    Q_D(QVideoFrameInput);
    return d->sendVideoFrame(frame);
}

/*!
    Returns the capture session this video frame input is connected to, or
    a \c nullptr if the video frame input is not connected to a capture session.

    Use QMediaCaptureSession::setVideoFrameInput() to connect
    the video frame input to a session.
*/
QMediaCaptureSession *QVideoFrameInput::captureSession() const
{
    Q_D(const QVideoFrameInput);
    return d->captureSession();
}

void QVideoFrameInput::setCaptureSession(QMediaCaptureSession *captureSession)
{
    Q_D(QVideoFrameInput);
    d->setCaptureSession(captureSession);
}

QPlatformVideoFrameInput *QVideoFrameInput::platformVideoFrameInput() const
{
    Q_D(const QVideoFrameInput);
    return d->platfromVideoFrameInput();
}

/*!
    \fn void QVideoFrameInput::readyToSendVideoFrame()

    Signals that a new frame can be sent to the video frame input.
    After receiving the signal, if you have frames to be sent, invoke \l sendVideoFrame
    once or in a loop until it returns \c false.

    \sa sendVideoFrame()
*/

QT_END_NAMESPACE
