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

#include "qmediacapturesession.h"
#include "qaudiodevice.h"
#include "qcamera.h"
#include "qmediaencoder.h"
#include "qcameraimagecapture.h"
#include "qvideosink.h"

#include <qpointer.h>

#include "qplatformmediaintegration_p.h"
#include "qplatformmediacapture_p.h"

QT_BEGIN_NAMESPACE

class QMediaCaptureSessionPrivate
{
public:
    QMediaCaptureSession *q = nullptr;
    QPlatformMediaCaptureSession *captureSession;
    QAudioDevice audioInput;
    QCamera *camera = nullptr;
    QCameraImageCapture *imageCapture = nullptr;
    QMediaEncoder *encoder = nullptr;
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

    The QMediaCaptureSession is the central class that manages capturing of media on the local device.

    You can connect a camera and a microphone to QMediaCaptureSession using setCamera() and setAudioInput().
    A preview of the captured media can be seen by setting a QVideoSink of QVideoWidget using setVideoOutput()
    and heard by routing the audio to an output device using setAudioOutput().

    You can capture still images from a camera by setting a QCameraImageCapture object on the capture session,
    and record audio/video using a QMediaEncoder.

    If you need a simple class that records media from the default camera and microphone, you can use QMediaRecorder.
    That class uses a QMediaCaptureSession behind the scene to support audio and video capture.

    \sa QCamera, QAudioDevice, QMediaEncoder, QCameraImageCapture, QMediaRecorder
*/

/*!
    Creates a session for media capture.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject(parent),
    d_ptr(new QMediaCaptureSessionPrivate)
{
    d_ptr->q = this;
    d_ptr->captureSession = QPlatformMediaIntegration::instance()->createCaptureSession(QMediaRecorder::AudioAndVideo);

    connect(d_ptr->captureSession, SIGNAL(mutedChanged(bool)), this, SIGNAL(mutedChanged(bool)));
    connect(d_ptr->captureSession, SIGNAL(volumeChanged(qreal)), this, SIGNAL(volumeChanged(qreal)));
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
    if (d_ptr->camera)
        d_ptr->camera->setCaptureSession(nullptr);
    if (d_ptr->encoder)
        d_ptr->encoder->setCaptureSession(nullptr);
    if (d_ptr->imageCapture)
        d_ptr->imageCapture->setCaptureSession(nullptr);
    d_ptr->setVideoSink(nullptr);
    delete d_ptr->captureSession;
    delete d_ptr;
}

/*!
    Returns false if media capture is not supported.
 */
bool QMediaCaptureSession::isAvailable() const
{
    return d_ptr->captureSession != nullptr;
}

/*!
    Returns the device that is being used to capture audio.
*/
QAudioDevice QMediaCaptureSession::audioInput() const
{
    return d_ptr->audioInput;
}

/*!
    Sets the audio input device to \a device. If setting it to an empty
    QAudioDevice the capture session will use the default input as
    defined by the operating system.

    Use setMuted(), if you want to disable audio input.

    \sa muted(), setMuted()
*/
void QMediaCaptureSession::setAudioInput(const QAudioDevice &device)
{
    d_ptr->audioInput = device;
    d_ptr->captureSession->setAudioInput(device);
    emit audioInputChanged();
}

/*!
    \property QMediaCaptureSession::muted

    \brief whether a recording audio stream is muted.
*/

bool QMediaCaptureSession::isMuted() const
{
    return d_ptr->captureSession->isMuted();
}

void QMediaCaptureSession::setMuted(bool muted)
{
    d_ptr->captureSession->setMuted(muted);
}

/*!
    \property QMediaCaptureSession::volume

    \brief the current recording audio volume.

    The volume is scaled linearly from \c 0.0 (silence) to \c 1.0 (full volume). Values outside this
    range will be clamped.

    The default volume is \c 1.0.

    UI volume controls should usually be scaled nonlinearly. For example, using a logarithmic scale
    will produce linear changes in perceived loudness, which is what a user would normally expect
    from a volume control. See QAudio::convertVolume() for more details.
*/

qreal QMediaCaptureSession::volume() const
{
    return d_ptr->captureSession->volume();
}

void QMediaCaptureSession::setVolume(qreal volume)
{
    d_ptr->captureSession->setVolume(volume);
}

/*!
    \property QMediaCaptureSession::camera

    \brief the camera used to capture video.

    Record the camera or take images by adding a camera t the capture session using this property,
*/
QCamera *QMediaCaptureSession::camera() const
{
    return d_ptr->camera;
}

void QMediaCaptureSession::setCamera(QCamera *camera)
{
    if (d_ptr->camera == camera)
        return;
    if (d_ptr->camera)
        d_ptr->camera->setCaptureSession(nullptr);

    d_ptr->camera = camera;
    if (d_ptr->camera)
        d_ptr->camera->setCaptureSession(this);
    emit cameraChanged();
}

/*!
    \property QMediaCaptureSession::imageCapture

    \brief the object used to capture still images.

    Add a QCameraImageCapture object to the capture session to enable
    capturing of still images from the camera.
*/
QCameraImageCapture *QMediaCaptureSession::imageCapture()
{
    return d_ptr->imageCapture;
}

void QMediaCaptureSession::setImageCapture(QCameraImageCapture *imageCapture)
{
    if (d_ptr->imageCapture == imageCapture)
        return;
    if (d_ptr->imageCapture)
        d_ptr->imageCapture->setCaptureSession(nullptr);

    d_ptr->imageCapture = imageCapture;
    if (d_ptr->imageCapture)
        d_ptr->imageCapture->setCaptureSession(this);
    emit imageCaptureChanged();
}

/*!
    \property QMediaCaptureSession::encoder

    \brief the encoder object used to capture audio/video.

    Add a QMediaEncoder object to the capture session to enable
    recording of audio and/or video from the capture session.
*/

QMediaEncoder *QMediaCaptureSession::encoder()
{
    return d_ptr->encoder;
}

void QMediaCaptureSession::setEncoder(QMediaEncoder *encoder)
{
    if (d_ptr->encoder == encoder)
        return;
    if (d_ptr->encoder)
        d_ptr->encoder->setCaptureSession(nullptr);

    d_ptr->encoder = encoder;
    if (d_ptr->encoder)
        d_ptr->encoder->setCaptureSession(this);
    emit encoderChanged();
}

QObject *QMediaCaptureSession::videoOutput() const
{
    Q_D(const QMediaCaptureSession);
    return d->videoOutput;
}

/*!
    Sets a QObject based video preview for the capture session.

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
    \internal
*/
QPlatformMediaCaptureSession *QMediaCaptureSession::platformSession() const
{
    return d_ptr->captureSession;
}


QT_END_NAMESPACE

#include "moc_qmediacapturesession.cpp"
