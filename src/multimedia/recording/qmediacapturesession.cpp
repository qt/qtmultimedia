// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediacapturesession.h"
#include "qmediacapturesession_p.h"

#include <QtMultimedia/qaudiobufferinput.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioinput.h>
#include <QtMultimedia/qaudiooutput.h>
#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qimagecapture.h>
#include <QtMultimedia/qmediarecorder.h>
#include <QtMultimedia/qscreencapture.h>
#include <QtMultimedia/qvideoframeinput.h>
#include <QtMultimedia/qvideosink.h>
#include <QtMultimedia/qwindowcapture.h>
#include <QtMultimedia/private/qplatformmediaintegration_p.h>
#include <QtMultimedia/private/qplatformmediacapture_p.h>

#if QT_CONFIG(gstreamer_qt_api)
#  include <QtMultimedia/spi/qgstreamervideosource.h>
#endif

QT_BEGIN_NAMESPACE

void QMediaCaptureSessionPrivate::setVideoSink(QVideoSink *sink)
{
    Q_Q(QMediaCaptureSession);

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

/*!
    \class QMediaCaptureSession

    \brief The QMediaCaptureSession class allows capturing of audio and video content.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_video
    \ingroup multimedia_audio

    The QMediaCaptureSession is the central class that manages capturing of media on the local
   device.

    You can connect a video input to QMediaCaptureSession using setCamera(),
    setScreenCapture(), setWindowCapture() or setVideoFrameInput().
    A preview of the captured media can be seen by setting a QVideoWidget or QGraphicsVideoItem
   using setVideoOutput().

    You can connect a microphone to QMediaCaptureSession using setAudioInput(), or set your
    custom audio input using setAudioBufferInput().
    The captured sound can be heard by routing the audio to an output device using setAudioOutput().

    You can capture still images from a camera by setting a QImageCapture object on the capture
   session, and record audio/video using a QMediaRecorder.

    \sa QCamera, QAudioDevice, QMediaRecorder, QImageCapture, QScreenCapture, QWindowCapture,
   QVideoFrameInput, QGStreamerVideoSource, QMediaRecorder, QGraphicsVideoItem
*/

/*!
    \qmltype CaptureSession
    \since 6.2
    \nativetype QMediaCaptureSession
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

        Component.onCompleted: {
            camera.start()
        }
    }
\endqml

    \sa Camera, MediaDevices, MediaRecorder, ImageCapture, ScreenCapture, WindowCapture,
    GStreamerVideoSource, AudioInput, VideoOutput
    \note To ensure the camera starts capturing video frames on all platforms, explicitly call camera.start(),
    typically in the Component.onCompleted handler.
*/

template <>
struct QMediaCaptureSession::ObjectTraits<QCamera>
{
    static constexpr bool IsCamera = true;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::camera;
    static constexpr auto Setter = &QMediaCaptureSession::setCamera;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setCamera;
    static constexpr auto PlatformObjectProvider = &QCamera::platformCamera;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::cameraChanged;
};

#if QT_CONFIG(gstreamer_qt_api)
template <>
struct QMediaCaptureSession::ObjectTraits<QGStreamerVideoSource>
{
    static constexpr bool IsCamera = true;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::nativeVideoSource;
    static constexpr auto Setter = &QMediaCaptureSession::setNativeVideoSource;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setCamera;
    static constexpr auto PlatformObjectProvider = &QGStreamerVideoSource::platformVideoSource;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::nativeVideoSourceChanged;
};
#endif

template <>
struct QMediaCaptureSession::ObjectTraits<QScreenCapture>
{
    static constexpr bool IsCamera = false;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::screenCapture;
    static constexpr auto Setter = &QMediaCaptureSession::setScreenCapture;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setScreenCapture;
    static constexpr auto PlatformObjectProvider = &QScreenCapture::platformScreenCapture;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::screenCaptureChanged;
};

template <>
struct QMediaCaptureSession::ObjectTraits<QWindowCapture>
{
    static constexpr bool IsCamera = false;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::windowCapture;
    static constexpr auto Setter = &QMediaCaptureSession::setWindowCapture;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setWindowCapture;
    static constexpr auto PlatformObjectProvider = &QWindowCapture::platformWindowCapture;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::windowCaptureChanged;
};

template <>
struct QMediaCaptureSession::ObjectTraits<QVideoFrameInput>
{
    static constexpr bool IsCamera = false;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::videoFrameInput;
    static constexpr auto Setter = &QMediaCaptureSession::setVideoFrameInput;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setVideoFrameInput;
    static constexpr auto PlatformObjectProvider = &QVideoFrameInput::platformVideoFrameInput;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::videoFrameInputChanged;
};

template <>
struct QMediaCaptureSession::ObjectTraits<QAudioBufferInput>
{
    static constexpr bool IsCamera = false;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::audioBufferInput;
    static constexpr auto Setter = &QMediaCaptureSession::setAudioBufferInput;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setAudioBufferInput;
    static constexpr auto PlatformObjectProvider = &QAudioBufferInput::platformAudioBufferInput;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::audioBufferInputChanged;
};

template <>
struct QMediaCaptureSession::ObjectTraits<QImageCapture>
{
    static constexpr bool IsCamera = false;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::imageCapture;
    static constexpr auto Setter = &QMediaCaptureSession::setImageCapture;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setImageCapture;
    static constexpr auto PlatformObjectProvider = &QImageCapture::platformImageCapture;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::imageCaptureChanged;
};

template <>
struct QMediaCaptureSession::ObjectTraits<QMediaRecorder>
{
    static constexpr bool IsCamera = false;
    static constexpr auto Member = &QMediaCaptureSessionPrivate::recorder;
    static constexpr auto Setter = &QMediaCaptureSession::setRecorder;
    static constexpr auto PlatformSetter = &QPlatformMediaCaptureSession::setMediaRecorder;
    static constexpr auto PlatformObjectProvider = &QMediaRecorder::platformRecoder;
    static constexpr auto ChangeNotifier = &QMediaCaptureSession::recorderChanged;
};

template <typename Object>
void QMediaCaptureSession::setObject(Object *object) {
    Q_D(QMediaCaptureSession);

    using Traits = QMediaCaptureSession::ObjectTraits<Object>;

    Object *oldObject = qobject_cast<Object *>(d->*Traits::Member);
    if (oldObject == object)
        return;

    if constexpr (Traits::IsCamera) {
        if (!QPlatformMediaIntegration::instance()->isCameraSwitchingDuringRecordingSupported()
            && recorder() && recorder()->recorderState() == QMediaRecorder::RecordingState) {
            qWarning("This media backend does not support camera switching during recording");
            return;
        }
    }

    d->*Traits::Member = object;

    if (d->captureSession)
        std::invoke(Traits::PlatformSetter, d->captureSession, nullptr);

    if (oldObject) {
        if (oldObject->captureSession() && oldObject->captureSession() != this)
            std::invoke(Traits::Setter, oldObject->captureSession(), nullptr);
        oldObject->setCaptureSession(nullptr);
    }

    if (object) {
        if (auto *otherSession = object->captureSession())
            std::invoke(Traits::Setter, otherSession, nullptr);
        if (d->captureSession)
            std::invoke(Traits::PlatformSetter, d->captureSession,
                        std::invoke(Traits::PlatformObjectProvider, object));
        object->setCaptureSession(this);
    }

    emit (this->*Traits::ChangeNotifier)();
}

/*!
    Creates a session for media capture from the \a parent object.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject{ *new QMediaCaptureSessionPrivate, parent }
{
    Q_D(QMediaCaptureSession);

    auto maybeCaptureSession = QPlatformMediaIntegration::instance()->createCaptureSession();
    if (maybeCaptureSession) {
        d->captureSession.reset(maybeCaptureSession.value());
        d->captureSession->setCaptureSession(this);
    } else {
        qWarning() << "Failed to initialize QMediaCaptureSession" << maybeCaptureSession.error();
    }
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
    Q_D(QMediaCaptureSession);

    setCamera(nullptr);
    setNativeVideoSource(nullptr);
    setRecorder(nullptr);
    setImageCapture(nullptr);
    setScreenCapture(nullptr);
    setWindowCapture(nullptr);
    setVideoFrameInput(nullptr);
    setAudioBufferInput(nullptr);
    setAudioInput(nullptr);
    setAudioOutput(nullptr);
    d->setVideoSink(nullptr);
    d->captureSession.reset();
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
    Q_D(const QMediaCaptureSession);
    return d->audioInput;
}

/*!
    Sets the audio input device to \a input. If setting it to an empty
    QAudioDevice the capture session will use the default input as
    defined by the operating system.
*/
void QMediaCaptureSession::setAudioInput(QAudioInput *input)
{
    Q_D(QMediaCaptureSession);

    QAudioInput *oldInput = d->audioInput;
    if (oldInput == input)
        return;

    // To avoid double emit of audioInputChanged
    // from recursive setAudioInput(nullptr) call.
    d->audioInput = nullptr;

    // TODO: check if it's possible to reuse setObject(input)
    if (d->captureSession)
        d->captureSession->setAudioInput(nullptr);
    if (oldInput)
        oldInput->setDisconnectFunction({});
    if (input) {
        input->setDisconnectFunction([this](){ setAudioInput(nullptr); });
        if (d->captureSession)
            d->captureSession->setAudioInput(input->handle());
    }
    d->audioInput = input;
    emit audioInputChanged();
}

/*!
    \property QMediaCaptureSession::audioBufferInput
    \since 6.8

    \brief The object used to send custom audio buffers to \l QMediaRecorder.
*/
QAudioBufferInput *QMediaCaptureSession::audioBufferInput() const
{
    Q_D(const QMediaCaptureSession);

    return d->audioBufferInput;
}

void QMediaCaptureSession::setAudioBufferInput(QAudioBufferInput *input)
{
    setObject(input);
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
    Q_D(const QMediaCaptureSession);
    return d->camera;
}

void QMediaCaptureSession::setCamera(QCamera *camera)
{
#if QT_CONFIG(gstreamer_qt_api)
    Q_D(QMediaCaptureSession);
    if (d->nativeVideoSource && camera) {
        // TODO: perhaps, we should relax the limitation
        qWarning("Setting camera, when gstreamer video source is connected, is not supported");
        return;
    }
#endif

    setObject(camera);
}

/*!
    \qmlproperty GStreamerVideoSource QtMultimedia::CaptureSession::nativeVideoSource
    \since 6.12

    \brief The native video source used to capture video.

    For now, it is only possible to set GStreamerVideoSource as the native video source.
    A capture session can have either \l Camera or \l GStreamerVideoSource set
    at a time.

    \sa GStreamerVideoSource, Camera
*/

/*!
    \property QMediaCaptureSession::nativeVideoSource
    \since 6.12

    \brief The native video source used to capture video.

    For now, it is only possible to set QGStreamerVideoSource as the native video source.
    A capture session can have either \l QCamera or \l QGStreamerVideoSource set
    at a time.

    \sa QGStreamerVideoSource, QCamera
*/

QObject *QMediaCaptureSession::nativeVideoSource() const
{
    Q_D(const QMediaCaptureSession);
    return d->nativeVideoSource;
}

void QMediaCaptureSession::setNativeVideoSource(QObject *videoSource)
{
#if QT_CONFIG(gstreamer_qt_api)
    Q_D(QMediaCaptureSession);

    auto *gstreamerVideoSource = qobject_cast<QGStreamerVideoSource *>(videoSource);

    if (videoSource && !gstreamerVideoSource) {
        qCritical() << "Unsupported video source type; QGStreamerVideoSource is expected.";
        return;
    }

    if (d->camera && gstreamerVideoSource) {
        // TODO: perhaps, we should relax the limitation
        qWarning("Setting GStreamer video source, when camera is connected, is not supported");
        return;
    }

    setObject(gstreamerVideoSource);
#else
    if (videoSource)
        qCritical() << "Only gstreamer video source is supported";

#endif
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
    Q_D(QMediaCaptureSession);
    return d->screenCapture;
}

void QMediaCaptureSession::setScreenCapture(QScreenCapture *screenCapture)
{
    setObject(screenCapture);
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
QWindowCapture *QMediaCaptureSession::windowCapture()
{
    Q_D(QMediaCaptureSession);
    return d->windowCapture;
}

void QMediaCaptureSession::setWindowCapture(QWindowCapture *windowCapture)
{
    setObject(windowCapture);
}

/*!
    \property QMediaCaptureSession::videoFrameInput
    \since 6.8

    \brief The object used to send custom video frames to
    \l QMediaRecorder or a video output.
*/
QVideoFrameInput *QMediaCaptureSession::videoFrameInput() const
{
    Q_D(const QMediaCaptureSession);
    return d->videoFrameInput;
}

void QMediaCaptureSession::setVideoFrameInput(QVideoFrameInput *input)
{
    setObject(input);
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
    Q_D(QMediaCaptureSession);
    return d->imageCapture;
}

void QMediaCaptureSession::setImageCapture(QImageCapture *imageCapture)
{
    setObject(imageCapture);
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
    Q_D(QMediaCaptureSession);
    return d->recorder;
}

void QMediaCaptureSession::setRecorder(QMediaRecorder *recorder)
{
    setObject(recorder);
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
    Q_D(QMediaCaptureSession);

    QAudioOutput *oldOutput = d->audioOutput;
    if (oldOutput == output)
        return;

    // We don't want to end up with signal emitted
    // twice (from recursive call setAudioInput(nullptr)
    // from oldOutput->setDisconnectFunction():
    d->audioOutput = nullptr;

    if (d->captureSession)
        d->captureSession->setAudioOutput(nullptr);
    if (oldOutput)
        oldOutput->setDisconnectFunction({});
    if (output) {
        output->setDisconnectFunction([this](){ setAudioOutput(nullptr); });
        if (d->captureSession)
            d->captureSession->setAudioOutput(output->handle());
    }
    d->audioOutput = output;
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
    Q_D(const QMediaCaptureSession);
    return d->captureSession.get();
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
    \qmlsignal QtMultimedia::CaptureSession::nativeVideoSourceChanged()
    This signal is emitted when the selected native video source has changed.
    \sa CaptureSession::nativeVideoSource
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
