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
#include <qmediadevicemanager.h>

#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCamera


    \brief The QCamera class provides interface for system camera devices.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    QCamera can be used with QCameraViewfinder for viewfinder display,
    QMediaRecorder for video recording and QCameraImageCapture for image taking.

    You can use QCameraInfo to list available cameras and choose which one to use.

    \snippet multimedia-snippets/camera.cpp Camera selection

    See the \l{Camera Overview}{camera overview} for more information.
*/

void QCameraPrivate::_q_error(int error, const QString &errorString)
{
    Q_Q(QCamera);

    this->error = QCamera::Error(error);
    this->errorString = errorString;

    emit q->errorOccurred(this->error);
}

void QCameraPrivate::setState(QCamera::State newState)
{
    unsetError();

    if (!control) {
        _q_error(QCamera::CameraError, QCamera::tr("The camera service is missing"));
        return;
    }

    restartPending = false;
    control->setState(newState);
}

void QCameraPrivate::_q_updateState(QCamera::State newState)
{
    Q_Q(QCamera);

    //omit changins state to Loaded when the camera is temporarily
    //stopped to apply shanges
    if (restartPending)
        return;

    if (newState != state) {
        state = newState;
        emit q->stateChanged(state);
    }
}

void QCameraPrivate::_q_preparePropertyChange(int changeType)
{
    if (!control)
        return;

    QCamera::Status status = control->status();

    //all the changes are allowed until the camera is starting
    if (control->state() != QCamera::ActiveState)
        return;

    if (control->canChangeProperty(QPlatformCamera::PropertyChangeType(changeType), status))
        return;

    restartPending = true;
    control->setState(QCamera::LoadedState);
    QMetaObject::invokeMethod(q_ptr, "_q_restartCamera", Qt::QueuedConnection);
}

void QCameraPrivate::_q_restartCamera()
{
    if (restartPending) {
        restartPending = false;
        control->setState(QCamera::ActiveState);
    }
}

void QCameraPrivate::init()
{
    Q_Q(QCamera);
    initControls();
    cameraExposure = new QCameraExposure(q, control);
    cameraFocus = new QCameraFocus(q, control);
    imageProcessing = new QCameraImageProcessing(q, control);
}

void QCameraPrivate::initControls()
{
    Q_Q(QCamera);

    captureInterface = QPlatformMediaIntegration::instance()->createCaptureInterface(QMediaRecorder::AudioAndVideo);
    if (captureInterface) {
        control = captureInterface->cameraControl();

        if (control) {
            q->connect(control, SIGNAL(stateChanged(QCamera::State)), q, SLOT(_q_updateState(QCamera::State)));
            q->connect(control, SIGNAL(statusChanged(QCamera::Status)), q, SIGNAL(statusChanged(QCamera::Status)));
            q->connect(control, SIGNAL(error(int,QString)), q, SLOT(_q_error(int,QString)));
        }

        error = QCamera::NoError;
    } else {
        control = nullptr;

        error = QCamera::CameraError;
        errorString = QCamera::tr("The camera captureInterface is missing");
    }
}

void QCameraPrivate::clear()
{
    delete cameraExposure;
    delete cameraFocus;
    delete imageProcessing;
    delete control;

    cameraExposure = nullptr;
    cameraFocus = nullptr;
    imageProcessing = nullptr;
    control = nullptr;
    captureInterface = nullptr;
}

/*!
    Construct a QCamera with a \a parent.

    Selects the default camera on the system if more than one camera are available.
*/

QCamera::QCamera(QObject *parent)
    : QCamera(QMediaDeviceManager::defaultVideoInput(), parent)
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

    d->init();
    QCameraInfo info;
    auto cameras = QMediaDeviceManager::videoInputs();
    for (const auto &c : cameras) {
        if (c.position() == position) {
            info = c;
            break;
        }
    }
    setCameraInfo(info);
}

/*!
    Destroys the camera object.
*/

QCamera::~QCamera()
{
    Q_D(QCamera);
    d->clear();
}

/*!
    Returns the availability state of the camera service.
*/
bool QCamera::isAvailable() const
{
    Q_D(const QCamera);
    return (d->control != nullptr);
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
    Sets a QObject based camera \a viewfinder.

    A QObject based viewfinder is expected to have an invokable videoSurface()
    method that returns a QAbstractVideoSurface.

    The previously set viewfinder is detached.
*/
void QCamera::setViewfinder(QObject *viewfinder)
{
    auto *mo = viewfinder->metaObject();
    QAbstractVideoSurface *surface = nullptr;
    if (viewfinder && !mo->invokeMethod(viewfinder, "videoSurface", Q_RETURN_ARG(QAbstractVideoSurface *, surface))) {
        qWarning() << "QCamera::setViewFinder: Object" << viewfinder->metaObject()->className() << "does not have a videoSurface()";
        return;
    }
    setViewfinder(surface);
}

/*!
    Sets a video \a surface as the viewfinder of a camera.

    If a viewfinder has already been set on the camera the new surface
    will replace it.
*/

void QCamera::setViewfinder(QAbstractVideoSurface *surface)
{
    Q_D(QCamera);
    d->control->setVideoSurface(surface);
}

/*!
    Returns the error state of the object.
*/

QCamera::Error QCamera::error() const
{
    return d_func()->error;
}

/*!
    Returns a string describing a camera's error state.
*/
QString QCamera::errorString() const
{
    return d_func()->errorString;
}

/*!
    \internal
 */
QPlatformMediaCapture *QCamera::captureInterface() const
{
    return d_func()->captureInterface;
}

/*!
    Starts the camera.

    State is changed to QCamera::ActiveState if camera is started
    successfully, otherwise errorOccurred() signal is emitted.

    While the camera state is changed to QCamera::ActiveState,
    starting the camera service can be asynchronous with the actual
    status reported with QCamera::status property.
*/
void QCamera::start()
{
    Q_D(QCamera);
    d->setState(QCamera::ActiveState);
}

/*!
    Stops the camera.
    The camera state is changed from QCamera::ActiveState to QCamera::LoadedState.

    In this state, the camera still consumes power.

    \sa unload(), QCamera::UnloadedState
*/
void QCamera::stop()
{
    Q_D(QCamera);
    d->setState(QCamera::LoadedState);
}

/*!
    Opens the camera device.
    The camera state is changed to QCamera::LoadedState.

    It's not necessary to explicitly load the camera, unless the application
    needs to read the supported camera settings and change the default values
    according to the camera capabilities.

    In all the other cases, it's possible to start the camera directly
    from the unloaded state.

    /sa QCamera::UnloadedState
*/
void QCamera::load()
{
    Q_D(QCamera);
    d->setState(QCamera::LoadedState);
}

/*!
    Closes the camera device and deallocates the related resources.
    The camera state is changed to QCamera::UnloadedState.
*/
void QCamera::unload()
{
    Q_D(QCamera);
    d->setState(QCamera::UnloadedState);
}

QCamera::State QCamera::state() const
{
    return d_func()->state;
}

QCamera::Status QCamera::status() const
{
    if(d_func()->control)
        return (QCamera::Status)d_func()->control->status();

    return QCamera::UnavailableStatus;
}

/*!
    Returns the QCameraInfo object associated with this camera.
 */
QCameraInfo QCamera::cameraInfo() const
{
    Q_D(const QCamera);
    return d->cameraInfo;
}

void QCamera::setCameraInfo(const QCameraInfo &cameraInfo)
{
    Q_D(QCamera);
    if (cameraInfo.isNull())
        d->cameraInfo = QMediaDeviceManager::defaultVideoInput();
    else
        d->cameraInfo = cameraInfo;
    if (d->control)
        d->control->setCamera(d->cameraInfo);
}

/*!
    \enum QCamera::State

    This enum holds the current state of the camera.

    \value UnloadedState
           The initial camera state, with camera not loaded.
           The camera capabilities, except supported capture modes,
           are unknown.
           While the supported settings are unknown in this state,
           it's allowed to set the camera capture settings like codec,
           resolution, or frame rate.
    \value LoadedState
           The camera is loaded and ready to be configured.
           In this state it's allowed to query camera capabilities,
           set capture resolution, codecs, etc.
           The viewfinder is not active in the loaded state.
           The camera consumes power in the loaded state.
    \value ActiveState
           In the active state as soon as camera is started
           the viewfinder displays video frames and the
           camera is ready for capture.
*/


/*!
    \property QCamera::state
    \brief The current state of the camera object.
*/

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
    \value LoadedStatus
           The camera is loaded and ready to be configured.
           This status indicates the camera device is opened and
           it's possible to query for supported image and video capture settings,
           like resolution, framerate and codecs.
    \value LoadingStatus
           The camera device loading in result of state transition from
           QCamera::UnloadedState to QCamera::LoadedState or QCamera::ActiveState.
    \value UnloadingStatus
           The camera device is unloading in result of state transition from
           QCamera::LoadedState or QCamera::ActiveState to QCamera::UnloadedState.
    \value UnloadedStatus
           The initial camera status, with camera not loaded.
           The camera capabilities including supported capture settings may be unknown.
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
