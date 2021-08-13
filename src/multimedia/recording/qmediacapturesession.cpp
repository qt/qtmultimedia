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

    The QMediaCaptureSession is the central class that manages capturing of media on the local device.

    You can connect a camera and a microphone to QMediaCaptureSession using setCamera() and setAudioInput().
    A preview of the captured media can be seen by setting a QVideoSink of QVideoWidget using setVideoOutput()
    and heard by routing the audio to an output device using setAudioOutput().

    You can capture still images from a camera by setting a QImageCapture object on the capture session,
    and record audio/video using a QMediaRecorder.

    If you need a simple class that records media from the default camera and microphone, you can use QMediaRecorder.
    That class uses a QMediaCaptureSession behind the scene to support audio and video capture.

    \sa QCamera, QAudioDevice, QMediaRecorder, QImageCapture, QMediaRecorder
*/

/*!
    Creates a session for media capture.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject(parent),
    d_ptr(new QMediaCaptureSessionPrivate)
{
    d_ptr->q = this;
    d_ptr->captureSession = QPlatformMediaIntegration::instance()->createCaptureSession();
    Q_ASSERT(d_ptr->captureSession);
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
    if (d_ptr->camera)
        d_ptr->camera->setCaptureSession(nullptr);
    if (d_ptr->recorder)
        d_ptr->recorder->setCaptureSession(nullptr);
    if (d_ptr->imageCapture)
        d_ptr->imageCapture->setCaptureSession(nullptr);
    d_ptr->setVideoSink(nullptr);
    delete d_ptr->captureSession;
    delete d_ptr;
}

/*!
    Returns the device that is being used to capture audio.
*/
QAudioInput *QMediaCaptureSession::audioInput() const
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
void QMediaCaptureSession::setAudioInput(QAudioInput *device)
{
    if (d_ptr->audioInput == device)
        return;
    d_ptr->audioInput = device;
    d_ptr->captureSession->setAudioInput(device ? device->handle() : nullptr);
    emit audioInputChanged();
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

    Add a QImageCapture object to the capture session to enable
    capturing of still images from the camera.
*/
QImageCapture *QMediaCaptureSession::imageCapture()
{
    return d_ptr->imageCapture;
}

void QMediaCaptureSession::setImageCapture(QImageCapture *imageCapture)
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
    \property QMediaCaptureSession::recorder

    \brief the recorder object used to capture audio/video.

    Add a QMediaRecorder object to the capture session to enable
    recording of audio and/or video from the capture session.
*/

QMediaRecorder *QMediaCaptureSession::recorder()
{
    return d_ptr->recorder;
}

void QMediaCaptureSession::setRecorder(QMediaRecorder *recorder)
{
    if (d_ptr->recorder == recorder)
        return;
    if (d_ptr->recorder)
        d_ptr->recorder->setCaptureSession(nullptr);

    d_ptr->recorder = recorder;
    if (d_ptr->recorder)
        d_ptr->recorder->setCaptureSession(this);
    emit recorderChanged();
}

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

void QMediaCaptureSession::setAudioOutput(QAudioOutput *output)
{
    Q_D(QMediaCaptureSession);
    if (d->audioOutput == output)
        return;
    d->audioOutput = output;
    d->captureSession->setAudioOutput(output ? output->handle() : nullptr);
    emit audioOutputChanged();
}

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


QT_END_NAMESPACE

#include "moc_qmediacapturesession.cpp"
