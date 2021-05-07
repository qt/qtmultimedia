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


#include "qcamera_p.h"

#include <qcamerainfo.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraexposure_p.h>
#include <private/qplatformcamerafocus_p.h>
#include <private/qplatformcameraimageprocessing_p.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformmediacapture_p.h>
#include <qmediadevices.h>
#include <qmediacapturesession.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCamera


    \brief The QCamera class provides interface for system camera devices.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    QCamera can be used within a QMediaCaptureSession for video recording and image taking.

    You can use QCameraInfo to list available cameras and choose which one to use.

    \snippet multimedia-snippets/camera.cpp Camera selection

    See the \l{Camera Overview}{camera overview} for more information.
*/

void QCameraPrivate::_q_error(int error, const QString &errorString)
{
    Q_Q(QCamera);

    this->error = QCamera::Error(error);
    this->errorString = errorString;

    emit q->errorChanged();
    emit q->errorOccurred(this->error, errorString);
}

void QCameraPrivate::init()
{
    Q_Q(QCamera);

    control = QPlatformMediaIntegration::instance()->createCamera(q);
    if (!control) {
        _q_error(QCamera::CameraError, QString::fromUtf8("Camera not supported"));
        return;
    }

    if (cameraInfo.isNull())
        _q_error(QCamera::CameraError, QString::fromUtf8("Invalid camera specified"));
    control->setCamera(cameraInfo);
    q->connect(control, SIGNAL(activeChanged(bool)), q, SIGNAL(activeChanged(bool)));
    q->connect(control, SIGNAL(error(int,QString)), q, SLOT(_q_error(int,QString)));
    cameraExposure = new QCameraExposure(q, control);
    cameraFocus = new QCameraFocus(q, control);
    imageProcessing = new QCameraImageProcessing(q, control);
}

/*!
    Construct a QCamera with a \a parent.

    Selects the default camera on the system if more than one camera is available.
*/

QCamera::QCamera(QObject *parent)
    : QCamera(QMediaDevices::defaultVideoInput(), parent)
{
}

/*!
    \since 5.3

    Construct a QCamera from a camera description \a cameraInfo and \a parent.
*/

QCamera::QCamera(const QCameraInfo &cameraInfo, QObject *parent)
    : QObject(*new QCameraPrivate, parent)
{
    Q_D(QCamera);

    d->cameraInfo = cameraInfo;
    d->init();
    setCameraInfo(cameraInfo);
}

/*!
    \since 5.3

    Construct a QCamera which uses a hardware camera located a the specified \a position.

    For example on a mobile phone it can be used to easily choose between front-facing and
    back-facing cameras.

    If no camera is available at the specified \a position or if \a position is
    QCameraInfo::UnspecifiedPosition, the default camera is used.
*/

QCamera::QCamera(QCameraInfo::Position position, QObject *parent)
    : QObject(*new QCameraPrivate, parent)
{
    Q_D(QCamera);

    QCameraInfo info;
    auto cameras = QMediaDevices::videoInputs();
    for (const auto &c : cameras) {
        if (c.position() == position) {
            info = c;
            break;
        }
    }
    if (info.isNull())
        info = QMediaDevices::defaultVideoInput();
    d->cameraInfo = info;
    d->init();
}

/*!
    Destroys the camera object.
*/

QCamera::~QCamera()
{
    Q_D(QCamera);
    if (d->captureSession) {
        d->captureInterface->setCamera(nullptr);
        d->captureSession->setCamera(nullptr);
    }
    Q_ASSERT(!d->captureSession);
}

/*!
    Returns true if the camera can be used.
*/
bool QCamera::isAvailable() const
{
    Q_D(const QCamera);
    return d->control && !d->cameraInfo.isNull();
}

/*!
    Returns true if the camera is currently active.
*/
bool QCamera::isActive() const
{
    Q_D(const QCamera);
    return d->control && d->control->isActive();
}

/*!
    Turns the camera on or off.
*/
void QCamera::setActive(bool active)
{
    Q_D(const QCamera);
    if (d->control)
        d->control->setActive(active);
}

/*!
    Returns the camera exposure control object.
*/
QCameraExposure *QCamera::exposure() const
{
    return d_func()->cameraExposure;
}

/*!
    Returns the camera focus control object.
*/
QCameraFocus *QCamera::focus() const
{
    return d_func()->cameraFocus;
}

/*!
    Returns the camera image processing control object.
*/
QCameraImageProcessing *QCamera::imageProcessing() const
{
    return d_func()->imageProcessing;
}

/*!
    Returns the error state of the object.
*/

QCamera::Error QCamera::error() const
{
    return d_func()->error;
}

/*!
    Returns a human readable string describing a camera's error state.
*/
QString QCamera::errorString() const
{
    return d_func()->errorString;
}

/*! \fn void QCamera::start()

    Starts the camera.

    Same as setActive(true).

    If the camera can't be started for some reason, the errorOccurred() signal is emitted.

    While the camera state is changed to QCamera::ActiveState,
    starting the camera service can be asynchronous with the actual
    status reported with QCamera::status property.
*/

/*! \fn void QCamera::stop()

    Stops the camera.

    \sa unload(), QCamera::InactiveStatus
*/

/*!
    Returns the current status of the camers.
*/
QCamera::Status QCamera::status() const
{
    if(d_func()->control)
        return (QCamera::Status)d_func()->control->status();

    return QCamera::UnavailableStatus;
}


/*!
    Returns the capture session this camera is connected to, or
    a nullptr if the camera is not connected to a capture session.

    use QMediaCaptureSession::setCamera() to connect the camera to
    a session.
*/
QMediaCaptureSession *QCamera::captureSession() const
{
    Q_D(const QCamera);
    return d->captureSession;
}

/*!
    \internal
*/
void QCamera::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QCamera);

    d->captureSession = session;
    d->captureInterface = session ? session->platformSession() : nullptr;
    if (d->captureInterface && d->control)
        d->captureInterface->setCamera(d->control);
}

/*!
    Returns the QCameraInfo object associated with this camera.
 */
QCameraInfo QCamera::cameraInfo() const
{
    Q_D(const QCamera);
    return d->cameraInfo;
}

/*!
    Sets the camera object to use the physical camera described by
    \a cameraInfo.
*/
void QCamera::setCameraInfo(const QCameraInfo &cameraInfo)
{
    Q_D(QCamera);
    if (d->cameraInfo == cameraInfo)
        return;
    d->cameraInfo = cameraInfo;
    if (d->cameraInfo.isNull())
        d->_q_error(QCamera::CameraError, QString::fromUtf8("Invalid camera specified"));
    else
        d->_q_error(QCamera::NoError, QString());
    if (d->control)
        d->control->setCamera(d->cameraInfo);
    emit cameraInfoChanged();
    setCameraFormat({});
}

/*!
    Returns the camera format currently used by the camera.
*/
QCameraFormat QCamera::cameraFormat() const
{
    Q_D(const QCamera);
    return d->cameraFormat;
}

/*!
    Tells the camera to use the format described by \a format. This can be used to define
    as specific resolution and frame rate to be used for recording and image capture.
*/
void QCamera::setCameraFormat(const QCameraFormat &format)
{
    Q_D(QCamera);
    if (!d->control || !d->control->setCameraFormat(format))
        return;

    d->cameraFormat = format;
    emit cameraFormatChanged();
}

/*!
    \enum QCamera::Status

    This enum holds the current status of the camera.

    \value ActiveStatus
           The camera has been started and can produce data.
           The viewfinder displays video frames in active state.
           Depending on backend, changing some camera settings like
           capture mode, codecs or resolution in ActiveState may lead
           to changing the camera status to LoadedStatus and StartingStatus while
           the settings are applied and back to ActiveStatus when the camera is ready.
    \value StartingStatus
           The camera is starting in result of state transition to QCamera::ActiveState.
           The camera service is not ready to capture yet.
    \value StoppingStatus
           The camera is stopping in result of state transition from QCamera::ActiveState
           to QCamera::LoadedState or QCamera::UnloadedState.
    \value InactiveStatus
           The camera is not currently active.
    \value UnavailableStatus
           The camera or camera backend is not available.
*/


/*!
    \property QCamera::status
    \brief The current status of the camera object.
*/

/*!
    \enum QCamera::Error

    This enum holds the last error code.

    \value  NoError      No errors have occurred.
    \value  CameraError  An error has occurred.
*/

/*!
    \fn void QCamera::error(QCamera::Error value)
    \obsolete

    Use errorOccurred() instead.
*/

/*!
    \fn void QCamera::errorOccurred(QCamera::Error value)
    \since 5.15

    Signal emitted when error state changes to \a value.
*/

/*!
    \enum QCameraInfo::Position
    \since 5.3

    This enum specifies the physical position of the camera on the system hardware.

    \value UnspecifiedPosition  The camera position is unspecified or unknown.

    \value BackFace  The camera is on the back face of the system hardware. For example on a
    mobile device, it means it is on the opposite side to that of the screen.

    \value FrontFace  The camera is on the front face of the system hardware. For example on a
    mobile device, it means it is on the same side as that of the screen. Viewfinder frames of
    front-facing cameras are mirrored horizontally, so the users can see themselves as looking
    into a mirror. Captured images or videos are not mirrored.

    \sa QCameraInfo::position()
*/

/*!
  \fn QCamera::stateChanged(QCamera::State state)

  Signals the camera \a state has changed.

  Usually the state changes is caused by calling
  load(), unload(), start() and stop(),
  but the state can also be changed change as a result of camera error.
*/

/*!
  \fn QCamera::statusChanged(QCamera::Status status)

  Signals the camera \a status has changed.

*/

QT_END_NAMESPACE

#include "moc_qcamera.cpp"
