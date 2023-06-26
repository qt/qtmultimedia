// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediacapturesession.h"
#include "qaudiodevice.h"
#include "qcamera.h"
#include "qmediarecorder.h"
#include "qimagecapture.h"
#include "qvideosink.h"
#include "qscreencapture.h"
#include "qwindowcapture.h"

#include <qpointer.h>

#include "qplatformmediaintegration_p.h"
#include "qplatformmediacapture_p.h"
#include "qaudioinput.h"
#include "qaudiooutput.h"

QT_BEGIN_NAMESPACE

class QMediaCaptureSessionPrivate
{
public:
    QMediaCaptureSession *q = nullptr;
    QPlatformMediaCaptureSession *captureSession = nullptr;
    QAudioInput *audioInput = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QPointer<QCamera> camera;
    QPointer<QScreenCapture> screenCapture;
    QPointer<QWindowCapture> windowCapture;
    QPointer<QImageCapture> imageCapture;
    QPointer<QMediaRecorder> recorder;
    QPointer<QVideoSink> videoSink;
    QPointer<QObject> videoOutput;

    void setVideoSink(QVideoSink *sink)
    {
        if (sink == videoSink)
            return;
        if (videoSink)
            videoSink->setSource(nullptr);
        videoSink = sink;
        if (sink)
            sink->setSource(q);
        if (captureSession)
            captureSession->setVideoPreview(sink);
        emit q->videoOutputChanged();
    }
};

/*!
    \class QMediaCaptureSession

    \brief The QMediaCaptureSession class allows capturing of audio and video content.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video
    \ingroup multimedia_audio

    The QMediaCaptureSession is the central class that manages capturing of media on the local device.

    You can connect a video input to QMediaCaptureSession using setCamera(), setScreenCapture() or setWindowCapture().
    A preview of the captured media can be seen by setting a QVideoWidget or QGraphicsVideoItem using setVideoOutput().

    You can connect a microphone to QMediaCaptureSession using setAudioInput().
    The captured sound can be heard by routing the audio to an output device using setAudioOutput().

    You can capture still images from a camera by setting a QImageCapture object on the capture session,
    and record audio/video using a QMediaRecorder.

    \sa QCamera, QAudioDevice, QMediaRecorder, QImageCapture, QScreenCapture, QWindowCapture, QMediaRecorder, QGraphicsVideoItem
*/

/*!
    \qmltype CaptureSession
    \since 6.2
    \instantiates QMediaCaptureSession
    \brief Allows capturing of audio and video content.

    \inqmlmodule QtMultimedia
    \ingroup multimedia_qml
    \ingroup multimedia_audio_qml
    \ingroup multimedia_video_qml

    This is the central type that manages capturing of media on the local device.

    Connect a camera and a microphone to a CaptureSession by assigning Camera
    and AudioInput objects to the relevant properties.

    Capture a screen by connecting a ScreenCapture object to
    the screenCapture property.

    Capture a window by connecting a WindowCapture object to
    the windowCapture property.

    Enable a preview of the captured media by assigning a VideoOutput element to
    the videoOutput property.

    Route audio to an output device by assigning an AudioOutput object
    to the audioOutput property.

    Capture still images from a camera by assigning an ImageCapture to the
    imageCapture property.

    Record audio/video by assigning a MediaRecorder to the recorder property.

\qml
    CaptureSession {
        id: captureSession
        camera: Camera {
            id: camera
        }
        imageCapture: ImageCapture {
            id: imageCapture
        }

        recorder: MediaRecorder {
            id: recorder
        }
        videoOutput: preview
    }
\endqml

    \sa Camera, MediaDevices, MediaRecorder, ImageCapture, ScreenCapture, WindowCapture, AudioInput, VideoOutput
*/

/*!
    Creates a session for media capture from the \a parent object.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject(parent),
    d_ptr(new QMediaCaptureSessionPrivate)
{
    d_ptr->q = this;
    auto maybeCaptureSession = QPlatformMediaIntegration::instance()->createCaptureSession();
    if (maybeCaptureSession) {
        d_ptr->captureSession = maybeCaptureSession.value();
        d_ptr->captureSession->setCaptureSession(this);
    } else {
        qWarning() << "Failed to initialize QMediaCaptureSession" << maybeCaptureSession.error();
    }
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
    setCamera(nullptr);
    setRecorder(nullptr);
    setImageCapture(nullptr);
    setScreenCapture(nullptr);
    setWindowCapture(nullptr);
    setAudioInput(nullptr);
    setAudioOutput(nullptr);
    d_ptr->setVideoSink(nullptr);
    delete d_ptr->captureSession;
    delete d_ptr;
}
/*!
    \qmlproperty AudioInput QtMultimedia::CaptureSession::audioInput

    This property holds the audio input that is being used to capture audio.
*/

/*!
    \property QMediaCaptureSession::audioInput

    Returns the device that is being used to capture audio.
*/
QAudioInput *QMediaCaptureSession::audioInput() const
{
    return d_ptr->audioInput;
}

/*!
    Sets the audio input device to \a input. If setting it to an empty
    QAudioDevice the capture session will use the default input as
    defined by the operating system.
*/
void QMediaCaptureSession::setAudioInput(QAudioInput *input)
{
    QAudioInput *oldInput = d_ptr->audioInput;
    if (oldInput == input)
        return;
    d_ptr->audioInput = input;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setAudioInput(nullptr);
    if (oldInput)
        oldInput->setDisconnectFunction({});
    if (input) {
        input->setDisconnectFunction([this](){ setAudioInput(nullptr); });
        if (d_ptr->captureSession)
            d_ptr->captureSession->setAudioInput(input->handle());
    }
    emit audioInputChanged();
}

/*!
    \qmlproperty Camera QtMultimedia::CaptureSession::camera

    \brief The camera used to capture video.

    Record video or take images by adding a camera to the capture session using
    this property.
*/

/*!
    \property QMediaCaptureSession::camera

    \brief The camera used to capture video.

    Record video or take images by adding a camera to the capture session
    using this property.
*/
QCamera *QMediaCaptureSession::camera() const
{
    return d_ptr->camera;
}

void QMediaCaptureSession::setCamera(QCamera *camera)
{
    // TODO: come up with an unification of the captures setup
    QCamera *oldCamera = d_ptr->camera;
    if (oldCamera == camera)
        return;
    d_ptr->camera = camera;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setCamera(nullptr);
    if (oldCamera) {
        if (oldCamera->captureSession() && oldCamera->captureSession() != this)
            oldCamera->captureSession()->setCamera(nullptr);
        oldCamera->setCaptureSession(nullptr);
    }
    if (camera) {
        if (camera->captureSession())
            camera->captureSession()->setCamera(nullptr);
        if (d_ptr->captureSession)
            d_ptr->captureSession->setCamera(camera->platformCamera());
        camera->setCaptureSession(this);
    }
    emit cameraChanged();
}

/*!
    \qmlproperty ScreenCapture QtMultimedia::CaptureSession::screenCapture
    \since 6.5

    \brief The object used to capture a screen.

    Record a screen by adding a screen capture objet
    to the capture session using this property.
*/

/*!
    \property QMediaCaptureSession::screenCapture
    \since 6.5

    \brief The object used to capture a screen.

    Record a screen by adding a screen capture object
    to the capture session using this property.
*/
QScreenCapture *QMediaCaptureSession::screenCapture()
{
    return d_ptr ? d_ptr->screenCapture : nullptr;
}

void QMediaCaptureSession::setScreenCapture(QScreenCapture *screenCapture)
{
    // TODO: come up with an unification of the captures setup
    QScreenCapture *oldScreenCapture = d_ptr->screenCapture;
    if (oldScreenCapture == screenCapture)
        return;
    d_ptr->screenCapture = screenCapture;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setScreenCapture(nullptr);
    if (oldScreenCapture) {
        if (oldScreenCapture->captureSession() && oldScreenCapture->captureSession() != this)
            oldScreenCapture->captureSession()->setScreenCapture(nullptr);
        oldScreenCapture->setCaptureSession(nullptr);
    }
    if (screenCapture) {
        if (screenCapture->captureSession())
            screenCapture->captureSession()->setScreenCapture(nullptr);
        if (d_ptr->captureSession)
            d_ptr->captureSession->setScreenCapture(screenCapture->platformScreenCapture());
        screenCapture->setCaptureSession(this);
    }
    emit screenCaptureChanged();
}

/*!
    \qmlproperty WindowCapture QtMultimedia::CaptureSession::windowCapture
    \since 6.6

    \brief The object used to capture a window.

    Record a window by adding a window capture object
    to the capture session using this property.
*/

/*!
    \property QMediaCaptureSession::windowCapture
    \since 6.6

    \brief The object used to capture a window.

    Record a window by adding a window capture objet
    to the capture session using this property.
*/
QWindowCapture *QMediaCaptureSession::windowCapture() {
    return d_ptr ? d_ptr->windowCapture : nullptr;
}

void QMediaCaptureSession::setWindowCapture(QWindowCapture *windowCapture)
{
    // TODO: come up with an unification of the captures setup
    QWindowCapture *oldCapture = d_ptr->windowCapture;
    if (oldCapture == windowCapture)
        return;
    d_ptr->windowCapture = windowCapture;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setWindowCapture(nullptr);
    if (oldCapture) {
        if (oldCapture->captureSession() && oldCapture->captureSession() != this)
            oldCapture->captureSession()->setWindowCapture(nullptr);
        oldCapture->setCaptureSession(nullptr);
    }
    if (windowCapture) {
        if (windowCapture->captureSession())
            windowCapture->captureSession()->setWindowCapture(nullptr);
        if (d_ptr->captureSession)
            d_ptr->captureSession->setWindowCapture(windowCapture->platformWindowCapture());
        windowCapture->setCaptureSession(this);
    }
    emit windowCaptureChanged();
}

/*!
    \qmlproperty ImageCapture QtMultimedia::CaptureSession::imageCapture

    \brief The object used to capture still images.

    Add an ImageCapture interface to the capture session to enable
    capturing of still images from the camera.
*/
/*!
    \property QMediaCaptureSession::imageCapture

    \brief the object used to capture still images.

    Add a QImageCapture object to the capture session to enable
    capturing of still images from the camera.
*/
QImageCapture *QMediaCaptureSession::imageCapture()
{
    return d_ptr->imageCapture;
}

void QMediaCaptureSession::setImageCapture(QImageCapture *imageCapture)
{
    // TODO: come up with an unification of the captures setup
    QImageCapture *oldImageCapture = d_ptr->imageCapture;
    if (oldImageCapture == imageCapture)
        return;
    d_ptr->imageCapture = imageCapture;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setImageCapture(nullptr);
    if (oldImageCapture) {
        if (oldImageCapture->captureSession() && oldImageCapture->captureSession() != this)
            oldImageCapture->captureSession()->setImageCapture(nullptr);
        oldImageCapture->setCaptureSession(nullptr);
    }
    if (imageCapture) {
        if (imageCapture->captureSession())
            imageCapture->captureSession()->setImageCapture(nullptr);
        if (d_ptr->captureSession)
            d_ptr->captureSession->setImageCapture(imageCapture->platformImageCapture());
        imageCapture->setCaptureSession(this);
    }
    emit imageCaptureChanged();
}
/*!
    \qmlproperty MediaRecorder QtMultimedia::CaptureSession::recorder

    \brief The recorder object used to capture audio/video.

    Add a MediaRcorder object to the capture session to enable
    recording of audio and/or video from the capture session.
*/
/*!
    \property QMediaCaptureSession::recorder

    \brief The recorder object used to capture audio/video.

    Add a QMediaRecorder object to the capture session to enable
    recording of audio and/or video from the capture session.
*/

QMediaRecorder *QMediaCaptureSession::recorder()
{
    return d_ptr->recorder;
}

void QMediaCaptureSession::setRecorder(QMediaRecorder *recorder)
{
    QMediaRecorder *oldRecorder = d_ptr->recorder;
    if (oldRecorder == recorder)
        return;
    d_ptr->recorder = recorder;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setMediaRecorder(nullptr);
    if (oldRecorder) {
        if (oldRecorder->captureSession() && oldRecorder->captureSession() != this)
            oldRecorder->captureSession()->setRecorder(nullptr);
        oldRecorder->setCaptureSession(nullptr);
    }
    if (recorder) {
        if (recorder->captureSession())
            recorder->captureSession()->setRecorder(nullptr);
        if (d_ptr->captureSession)
            d_ptr->captureSession->setMediaRecorder(recorder->platformRecoder());
        recorder->setCaptureSession(this);
    }
    emit recorderChanged();
}
/*!
    \qmlproperty VideoOutput QtMultimedia::CaptureSession::videoOutput

    \brief The VideoOutput that is the video preview for the capture session.

    A VideoOutput based preview is expected to have an invokable videoSink()
    method that returns a QVideoSink.

    The previously set preview is detached.

*/
/*!
    \property QMediaCaptureSession::videoOutput

    Returns the video output for the session.
*/
QObject *QMediaCaptureSession::videoOutput() const
{
    Q_D(const QMediaCaptureSession);
    return d->videoOutput;
}
/*!
    Sets a QObject, (\a output), to a video preview for the capture session.

    A QObject based preview is expected to have an invokable videoSink()
    method that returns a QVideoSink.

    The previously set preview is detached.
*/
void QMediaCaptureSession::setVideoOutput(QObject *output)
{
    Q_D(QMediaCaptureSession);
    if (d->videoOutput == output)
        return;
    QVideoSink *sink = qobject_cast<QVideoSink *>(output);
    if (!sink && output) {
        auto *mo = output->metaObject();
        mo->invokeMethod(output, "videoSink", Q_RETURN_ARG(QVideoSink *, sink));
    }
    d->videoOutput = output;
    d->setVideoSink(sink);
}

/*!
    Sets a QVideoSink, (\a sink), to a video preview for the capture session.

    A QObject based preview is expected to have an invokable videoSink()
    method that returns a QVideoSink.

    The previously set preview is detached.
*/
void QMediaCaptureSession::setVideoSink(QVideoSink *sink)
{
    Q_D(QMediaCaptureSession);
    d->videoOutput = nullptr;
    d->setVideoSink(sink);
}

/*!
    Returns the QVideoSink for the session.
*/
QVideoSink *QMediaCaptureSession::videoSink() const
{
    Q_D(const QMediaCaptureSession);
    return d->videoSink;
}
/*!
    Sets the audio output device to \a{output}.

    Setting an audio output device enables audio routing from an audio input device.
*/
void QMediaCaptureSession::setAudioOutput(QAudioOutput *output)
{
    QAudioOutput *oldOutput = d_ptr->audioOutput;
    if (oldOutput == output)
        return;
    d_ptr->audioOutput = output;
    if (d_ptr->captureSession)
        d_ptr->captureSession->setAudioOutput(nullptr);
    if (oldOutput)
        oldOutput->setDisconnectFunction({});
    if (output) {
        output->setDisconnectFunction([this](){ setAudioOutput(nullptr); });
        if (d_ptr->captureSession)
            d_ptr->captureSession->setAudioOutput(output->handle());
    }
    emit audioOutputChanged();
}
/*!
    \qmlproperty AudioOutput QtMultimedia::CaptureSession::audioOutput
    \brief The audio output device for the capture session.

    Add an AudioOutput device to the capture session to enable
    audio routing from an AudioInput device.
*/
/*!
    \property QMediaCaptureSession::audioOutput

    Returns the audio output for the session.
*/
QAudioOutput *QMediaCaptureSession::audioOutput() const
{
    Q_D(const QMediaCaptureSession);
    return d->audioOutput;
}

/*!
    \internal
*/
QPlatformMediaCaptureSession *QMediaCaptureSession::platformSession() const
{
    return d_ptr->captureSession;
}
/*!
    \qmlsignal QtMultimedia::CaptureSession::audioInputChanged()
    This signal is emitted when an audio input has changed.
    \sa CaptureSession::audioInput
*/

/*!
    \qmlsignal QtMultimedia::CaptureSession::cameraChanged()
    This signal is emitted when the selected camera has changed.
    \sa CaptureSession::camera
*/

/*!
    \qmlsignal QtMultimedia::CaptureSession::imageCaptureChanged()
    This signal is emitted when the selected interface has changed.
    \sa CaptureSession::camera
*/

/*!
    \qmlsignal QtMultimedia::CaptureSession::recorderChanged()
    This signal is emitted when the selected recorder has changed.
    \sa CaptureSession::recorder
*/

/*!
    \qmlsignal QtMultimedia::CaptureSession::videoOutputChanged()
    This signal is emitted when the selected video output has changed.
    \sa CaptureSession::videoOutput
*/

/*!
    \qmlsignal QtMultimedia::CaptureSession::audioOutputChanged()
    This signal is emitted when the selected audio output has changed.
    \sa CaptureSession::audioOutput
*/
QT_END_NAMESPACE

#include "moc_qmediacapturesession.cpp"
