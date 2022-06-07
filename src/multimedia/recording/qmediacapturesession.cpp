// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediacapturesession.h"
#include "qaudiodevice.h"
#include "qcamera.h"
#include "qmediarecorder.h"
#include "qimagecapture.h"
#include "qvideosink.h"

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
    QPlatformMediaCaptureSession *captureSession;
    QAudioInput *audioInput = nullptr;
    QAudioOutput *audioOutput = nullptr;
    QCamera *camera = nullptr;
    QImageCapture *imageCapture = nullptr;
    QMediaRecorder *recorder = nullptr;
    QVideoSink *videoSink = nullptr;
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

    You can connect a camera and a microphone to QMediaCaptureSession using setCamera() and setAudioInput().
    A preview of the captured media can be seen by setting a QVideoSink of QVideoWidget using setVideoOutput()
    and heard by routing the audio to an output device using setAudioOutput().

    You can capture still images from a camera by setting a QImageCapture object on the capture session,
    and record audio/video using a QMediaRecorder.

    \sa QCamera, QAudioDevice, QMediaRecorder, QImageCapture, QMediaRecorder
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

    \sa Camera, MediaDevices, MediaRecorder, ImageCapture, AudioInput, VideoOutput
*/

/*!
    Creates a session for media capture from the \a parent object.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject(parent),
    d_ptr(new QMediaCaptureSessionPrivate)
{
    d_ptr->q = this;
    d_ptr->captureSession = QPlatformMediaIntegration::instance()->createCaptureSession();
    d_ptr->captureSession->setCaptureSession(this);
    Q_ASSERT(d_ptr->captureSession);
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
    setCamera(nullptr);
    setRecorder(nullptr);
    setImageCapture(nullptr);
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
    d_ptr->captureSession->setAudioInput(nullptr);
    if (oldInput)
        oldInput->setDisconnectFunction({});
    if (input) {
        input->setDisconnectFunction([this](){ setAudioInput(nullptr); });
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
    using this property,
*/
QCamera *QMediaCaptureSession::camera() const
{
    return d_ptr->camera;
}

void QMediaCaptureSession::setCamera(QCamera *camera)
{
    QCamera *oldCamera = d_ptr->camera;
    if (oldCamera == camera)
        return;
    d_ptr->camera = camera;
    d_ptr->captureSession->setCamera(nullptr);
    if (oldCamera) {
        if (oldCamera->captureSession() && oldCamera->captureSession() != this)
            oldCamera->captureSession()->setCamera(nullptr);
        oldCamera->setCaptureSession(nullptr);
    }
    if (camera) {
        if (camera->captureSession())
            camera->captureSession()->setCamera(nullptr);
        d_ptr->captureSession->setCamera(camera->platformCamera());
        camera->setCaptureSession(this);
    }
    emit cameraChanged();
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
    QImageCapture *oldImageCapture = d_ptr->imageCapture;
    if (oldImageCapture == imageCapture)
        return;
    d_ptr->imageCapture = imageCapture;
    d_ptr->captureSession->setImageCapture(nullptr);
    if (oldImageCapture) {
        if (oldImageCapture->captureSession() && oldImageCapture->captureSession() != this)
            oldImageCapture->captureSession()->setImageCapture(nullptr);
        oldImageCapture->setCaptureSession(nullptr);
    }
    if (imageCapture) {
        if (imageCapture->captureSession())
            imageCapture->captureSession()->setImageCapture(nullptr);
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
    d_ptr->captureSession->setMediaRecorder(nullptr);
    if (oldRecorder) {
        if (oldRecorder->captureSession() && oldRecorder->captureSession() != this)
            oldRecorder->captureSession()->setRecorder(nullptr);
        oldRecorder->setCaptureSession(nullptr);
    }
    if (recorder) {
        if (recorder->captureSession())
            recorder->captureSession()->setRecorder(nullptr);
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
QVideoSink *QMediaCaptureSession::videoSink() const
{
    Q_D(const QMediaCaptureSession);
    return d->videoSink;
}
/*!
    Sets the audio output device to \a{output}.
*/
void QMediaCaptureSession::setAudioOutput(QAudioOutput *output)
{
    QAudioOutput *oldOutput = d_ptr->audioOutput;
    if (oldOutput == output)
        return;
    d_ptr->audioOutput = output;
    d_ptr->captureSession->setAudioOutput(nullptr);
    if (oldOutput)
        oldOutput->setDisconnectFunction({});
    if (output) {
        output->setDisconnectFunction([this](){ setAudioOutput(nullptr); });
        d_ptr->captureSession->setAudioOutput(output->handle());
    }
    emit audioOutputChanged();
}
/*!
    \qmlproperty AudioOutput QtMultimedia::CaptureSession::audioOutput
    \brief The audio output device for the capture session.
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
