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
#include "qaudiodeviceinfo.h"
#include "qcamera.h"
#include "qmediaencoder.h"
#include "qcameraimagecapture.h"

#include "qplatformmediaintegration_p.h"
#include "qplatformmediacapture_p.h"

QT_BEGIN_NAMESPACE

class QMediaCaptureSessionPrivate
{
public:
    QPlatformMediaCaptureSession *captureSession;
    QAudioDeviceInfo audioInput;
    QCamera *camera = nullptr;
    QCameraImageCapture *imageCapture = nullptr;
    QMediaEncoder *encoder = nullptr;
};


/*!
    Creates a session for media capture.
 */
QMediaCaptureSession::QMediaCaptureSession(QObject *parent)
    : QObject(parent),
    d_ptr(new QMediaCaptureSessionPrivate)
{
    d_ptr->captureSession = QPlatformMediaIntegration::instance()->createCaptureSession(QMediaRecorder::AudioAndVideo);
}

/*!
    Destroys the session.
 */
QMediaCaptureSession::~QMediaCaptureSession()
{
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
    Creates a session for media capture.
 */
QAudioDeviceInfo QMediaCaptureSession::audioInput() const
{
    return d_ptr->audioInput;
}

void QMediaCaptureSession::setAudioInput(const QAudioDeviceInfo &device)
{
    d_ptr->audioInput = device;
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

QMediaEncoder *QMediaCaptureSession::encoder()
{
    return d_ptr->encoder;
}

void QMediaCaptureSession::setEncoder(QMediaEncoder *recorder)
{
    if (d_ptr->encoder == recorder)
        return;
    if (d_ptr->encoder)
        d_ptr->encoder->setCaptureSession(nullptr);

    d_ptr->encoder = recorder;
    if (d_ptr->encoder)
        d_ptr->encoder->setCaptureSession(this);
    emit encoderChanged();
}

/*!
    Sets a QObject based video preview for the capture session.

    A QObject based preview is expected to have an invokable videoSurface()
    method that returns a QAbstractVideoSurface.

    The previously set preview is detached.
*/
void QMediaCaptureSession::setVideoPreview(QObject *preview)
{
    auto *mo = preview->metaObject();
    QAbstractVideoSurface *surface = nullptr;
    if (preview && !mo->invokeMethod(preview, "videoSurface", Q_RETURN_ARG(QAbstractVideoSurface *, surface))) {
        qWarning() << "QCamera::setViewFinder: Object" << preview->metaObject()->className() << "does not have a videoSurface()";
        return;
    }
    setVideoPreview(surface);
}

/*!
    Sets a video \a surface as the preview for the capture session.

    If a preview has already been set on the session, the new surface
    will replace it.
*/
void QMediaCaptureSession::setVideoPreview(QAbstractVideoSurface *preview)
{
    d_ptr->captureSession->setVideoPreview(preview);
}

QPlatformMediaCaptureSession *QMediaCaptureSession::platformSession() const
{
    return d_ptr->captureSession;
}


QT_END_NAMESPACE

#include "moc_qmediacapturesession.cpp"
