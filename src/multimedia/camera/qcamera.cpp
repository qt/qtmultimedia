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

    On hardware that supports it, QCamera lets you adjust the focus
    and zoom. This also includes things
    like "Macro" mode for close up work (e.g. reading barcodes, or
    recognising letters), or "touch to focus" - indicating an
    interesting area of the viewfinder for the hardware to attempt
    to focus on.

    \snippet multimedia-snippets/camera.cpp Camera custom focus

    The \l minimumZoomFactor() and \l maximumZoomFactor() methods allows checking the
    range of allowed zoom factors. The \l zoomTo() method allows changing the zoom factor.

    \snippet multimedia-snippets/camera.cpp Camera zoom


    After capturing the data for a camera frame, the camera hardware and
    software performs various image processing tasks to produce a final
    image.  This includes compensating for ambient light color, reducing
    noise, as well as making some other adjustments to the image.

    For example, you can set the white balance (or color temperature) used
    for processing images:

    \snippet multimedia-snippets/camera.cpp Camera image whitebalance

    For more information on image processing of camera frames, see \l {camera_image_processing}{Camera Image Processing}.

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

    exposureControl = control->exposureControl();
    if (exposureControl) {
        q->connect(exposureControl, SIGNAL(actualValueChanged(int)),
                   q, SLOT(_q_exposureParameterChanged(int)));
        q->connect(exposureControl, SIGNAL(parameterRangeChanged(int)),
                   q, SLOT(_q_exposureParameterRangeChanged(int)));
        q->connect(exposureControl, SIGNAL(flashReady(bool)), q, SIGNAL(flashReady(bool)));
    }

    imageControl = control->imageProcessingControl();
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


/*!
    \property QCamera::focusMode
    \brief the current camera focus mode.

    Sets up different focus modes for the camera. All auto focus modes will focus continuously.
    Locking the focus is possible by setting the focus mode to \l FocusModeManual. This will keep
    the current focus and stop any automatic focusing.

    \sa QCamera::isFocusModeSupported()
*/

QCamera::FocusMode QCamera::focusMode() const
{
    Q_D(const QCamera);
    return d->control ? d->control->focusMode() : QCamera::FocusModeAuto;
}

void QCamera::setFocusMode(QCamera::FocusMode mode)
{
    Q_D(QCamera);
    if (!d->control || d->control->focusMode() == mode)
        return;
    d->control->setFocusMode(mode);
    emit focusModeChanged();
}

/*!
    Returns true if the focus \a mode is supported by camera.
*/

bool QCamera::isFocusModeSupported(FocusMode mode) const
{
    Q_D(const QCamera);
    return d->control ? d->control->isFocusModeSupported(mode) : false;
}

/*!
    Returns the point currently used by the auto focus system to focus onto.
 */
QPointF QCamera::focusPoint() const
{
    Q_D(const QCamera);
    return d->control ? d->control->focusPoint() : QPointF(-1., -1.);

}

/*!
  \property QCamera::customFocusPoint

  This property represents the position of the custom focus point, in relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.

  The custom focus point property is used only in \c FocusPointCustom focus mode.
 */

QPointF QCamera::customFocusPoint() const
{
    Q_D(const QCamera);
    return d->control ? d->control->customFocusPoint() : QPointF{-1., -1.};
}

void QCamera::setCustomFocusPoint(const QPointF &point)
{
    Q_D(QCamera);
    if (!d->control)
        return;
    d->control->setCustomFocusPoint(point);
}

bool QCamera::isCustomFocusPointSupported() const
{
    Q_D(const QCamera);
    return d->control ? d->control->isCustomFocusPointSupported() : false;
}

/*!
    \property QCamera::focusDistance

    This property return an approximate focus distance of the camera. The value reported is between 0 and 1, 0 being the closest
    possible focus distance, 1 being as far away as possible. Note that 1 is often, but not always infinity.

    Setting the focus distance will be ignored unless the focus mode is set to \l FocusModeManual.
 */
void QCamera::setFocusDistance(float d)
{
    if (!d_func()->control || focusMode() != FocusModeManual)
        return;
    d_func()->control->setFocusDistance(d);
}

float QCamera::focusDistance() const
{
    if (d_func()->control && focusMode() == FocusModeManual)
        return d_func()->control->focusDistance();
    return 0.;
}

/*!
    Returns the maximum zoom factor.

    This will be \c 1.0 on cameras that do not support zooming.
*/

float QCamera::maximumZoomFactor() const
{
    Q_D(const QCamera);
    return d->control ? d->control->maxZoomFactor() : 1.;
}

/*!
    Returns the minimum zoom factor.

    This will be \c 1.0 on cameras that do not support zooming.
*/

float QCamera::minimumZoomFactor() const
{
    Q_D(const QCamera);
    return d->control ? d->control->minZoomFactor() : 1.;
}

/*!
  \property QCamera::zoomFactor
  \brief The current zoom factor.
*/
float QCamera::zoomFactor() const
{
    Q_D(const QCamera);
    return d->control ? d->control->zoomFactor() : 1.;
}

void QCamera::setZoomFactor(float factor)
{
    zoomTo(factor, 0.);
}

/*!
    Zooms to a zoom factor \a factor using \a rate.

    The rate is specified in powers of two per second. A rate of 1
    would take two seconds to zoom from a zoom factor of 1 to a zoom factor of 4.
 */
void QCamera::zoomTo(float factor, float rate)
{
    Q_ASSERT(rate >= 0.);
    if (rate < 0.)
        rate = 0.;

    Q_D(QCamera);
    if (!d->control)
        return;
    factor = qBound(d->control->minZoomFactor(), factor, d->control->maxZoomFactor());
    d->control->zoomTo(factor, rate);
}

/*!
    \enum QCamera::FocusMode

    \value FocusModeAuto        Continuous auto focus mode.
    \value FocusModeAutoNear    Continuous auto focus mode on near objects.
    \value FocusModeAutoFar     Continuous auto focus mode on objects far away.
    \value FocusModeHyperfocal  Focus to hyperfocal distance, with the maximum depth of field achieved.
                                All objects at distances from half of this
                                distance out to infinity will be acceptably sharp.
    \value FocusModeInfinity    Focus strictly to infinity.
    \value FocusModeManual      Manual or fixed focus mode.
*/

/*!
    \fn void QCamera::opticalZoomChanged(qreal value)

    Signal emitted when optical zoom value changes to new \a value.
*/

/*!
    \fn void QCamera::digitalZoomChanged(qreal value)

    Signal emitted when digital zoom value changes to new \a value.
*/

/*!
    \fn void QCamera::maximumOpticalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported optical \a zoom value changed.
*/

/*!
    \fn void QCamera::maximumDigitalZoomChanged(qreal zoom)

    Signal emitted when the maximum supported digital \a zoom value changed.

    The maximum supported zoom value can depend on other camera settings,
    like capture mode or resolution.
*/


template<typename T>
T QCameraPrivate::actualExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &defaultValue) const
{
    QVariant value = exposureControl ? exposureControl->actualValue(parameter) : QVariant();

    return value.isValid() ? value.value<T>() : defaultValue;
}

template<typename T>
T QCameraPrivate::requestedExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &defaultValue) const
{
    QVariant value = exposureControl ? exposureControl->requestedValue(parameter) : QVariant();

    return value.isValid() ? value.value<T>() : defaultValue;
}

template<typename T>
void QCameraPrivate::setExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &value)
{
    if (exposureControl)
        exposureControl->setValue(parameter, QVariant::fromValue<T>(value));
}

void QCameraPrivate::resetExposureParameter(QPlatformCameraExposure::ExposureParameter parameter)
{
    if (exposureControl)
        exposureControl->setValue(parameter, QVariant());
}


void QCameraPrivate::_q_exposureParameterChanged(int parameter)
{
    Q_Q(QCamera);

#if DEBUG_EXPOSURE_CHANGES
    qDebug() << "Exposure parameter changed:"
             << QPlatformCameraExposure::ExposureParameter(parameter)
             << exposureControl->actualValue(QPlatformCameraExposure::ExposureParameter(parameter));
#endif

    switch (parameter) {
    case QPlatformCameraExposure::ISO:
        emit q->isoSensitivityChanged(q->isoSensitivity());
        break;
    case QPlatformCameraExposure::ShutterSpeed:
        emit q->shutterSpeedChanged(q->shutterSpeed());
        break;
    case QPlatformCameraExposure::ExposureCompensation:
        emit q->exposureCompensationChanged(q->exposureCompensation());
        break;
    }
}

void QCameraPrivate::_q_exposureParameterRangeChanged(int parameter)
{
    Q_Q(QCamera);

    switch (parameter) {
    case QPlatformCameraExposure::ShutterSpeed:
        emit q->shutterSpeedRangeChanged();
        break;
    }
}

/*!
    \property QCamera::flashMode
    \brief The flash mode being used.

    Enables a certain flash mode if the camera has a flash.

    \sa QCamera::isFlashModeSupported(), QCamera::isFlashReady()
*/
QCamera::FlashMode QCamera::flashMode() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->flashMode() : QCamera::FlashOff;
}

void QCamera::setFlashMode(QCamera::FlashMode mode)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setFlashMode(mode);
}

/*!
    Returns true if the flash \a mode is supported.
*/

bool QCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    return d_func()->exposureControl ? d_func()->exposureControl->isFlashModeSupported(mode) : (mode == FlashOff);
}

/*!
    Returns true if flash is charged.
*/

bool QCamera::isFlashReady() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->isFlashReady() : false;
}

/*!
    \property QCamera::torchMode
    \brief The torch mode being used.

    A torch is a continuous light source used for low light video recording. Enabling torch mode
    will usually override any currently set flash mode.

    \sa QCamera::isTorchModeSupported(), QCamera::flashMode
*/
QCamera::TorchMode QCamera::torchMode() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->torchMode() : TorchOff;
}

void QCamera::setTorchMode(QCamera::TorchMode mode)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setTorchMode(mode);
}

/*!
    Returns true if the torch \a mode is supported.
*/
bool QCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    return d_func()->exposureControl ? d_func()->exposureControl->isTorchModeSupported(mode) : (mode == TorchOff);
}

/*!
  \property QCamera::exposureMode
  \brief The exposure mode being used.

  \sa QCamera::isExposureModeSupported()
*/

QCamera::ExposureMode QCamera::exposureMode() const
{
    return d_func()->actualExposureParameter<QCamera::ExposureMode>(QPlatformCameraExposure::ExposureMode, QCamera::ExposureAuto);
}

void QCamera::setExposureMode(QCamera::ExposureMode mode)
{
    d_func()->setExposureParameter<QCamera::ExposureMode>(QPlatformCameraExposure::ExposureMode, mode);
}

/*!
    Returns true if the exposure \a mode is supported.
*/

bool QCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (!d_func()->exposureControl)
        return false;

    bool continuous = false;
    return d_func()->exposureControl->supportedParameterRange(QPlatformCameraExposure::ExposureMode, &continuous)
            .contains(QVariant::fromValue<QCamera::ExposureMode>(mode));
}

/*!
  \property QCamera::exposureCompensation
  \brief Exposure compensation in EV units.

  Exposure compensation property allows to adjust the automatically calculated exposure.
*/

qreal QCamera::exposureCompensation() const
{
    return d_func()->actualExposureParameter<qreal>(QPlatformCameraExposure::ExposureCompensation, 0.0);
}

void QCamera::setExposureCompensation(qreal ev)
{
    d_func()->setExposureParameter<qreal>(QPlatformCameraExposure::ExposureCompensation, ev);
}

int QCamera::isoSensitivity() const
{
    return d_func()->actualExposureParameter<int>(QPlatformCameraExposure::ISO, -1);
}

/*!
    Returns the requested ISO sensitivity
    or -1 if automatic ISO is turned on.
*/
int QCamera::requestedIsoSensitivity() const
{
    return d_func()->requestedExposureParameter<int>(QPlatformCameraExposure::ISO, -1);
}

/*!
    Returns the list of ISO senitivities camera supports.

    If the camera supports arbitrary ISO sensitivities within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<int> QCamera::supportedIsoSensitivities(bool *continuous) const
{
    QList<int> res;
    QPlatformCameraExposure *control = d_func()->exposureControl;

    bool tmp = false;
    if (!continuous)
        continuous = &tmp;

    if (!control)
        return res;

    const auto range = control->supportedParameterRange(QPlatformCameraExposure::ISO, continuous);
    for (const QVariant &value : range) {
        bool ok = false;
        int intValue = value.toInt(&ok);
        if (ok)
            res.append(intValue);
        else
            qWarning() << "Incompatible ISO value type, int is expected";
    }

    return res;
}

/*!
    \fn QCamera::setManualIsoSensitivity(int iso)
    Sets the manual sensitivity to \a iso
*/

void QCamera::setManualIsoSensitivity(int iso)
{
    d_func()->setExposureParameter<int>(QPlatformCameraExposure::ISO, iso);
}

/*!
     \fn QCamera::setAutoIsoSensitivity()
     Turn on auto sensitivity
*/

void QCamera::setAutoIsoSensitivity()
{
    d_func()->resetExposureParameter(QPlatformCameraExposure::ISO);
}

/*!
    \property QCamera::shutterSpeed
    \brief Camera's shutter speed in seconds.

    \sa supportedShutterSpeeds(), setAutoShutterSpeed(), setManualShutterSpeed()
*/

/*!
    \fn QCamera::shutterSpeedChanged(qreal speed)

    Signals that a camera's shutter \a speed has changed.
*/

/*!
    \property QCamera::isoSensitivity
    \brief The sensor ISO sensitivity.

    \sa supportedIsoSensitivities(), setAutoIsoSensitivity(), setManualIsoSensitivity()
*/

/*!
    Returns the current shutter speed in seconds.
*/

qreal QCamera::shutterSpeed() const
{
    return d_func()->actualExposureParameter<qreal>(QPlatformCameraExposure::ShutterSpeed, -1.0);
}

/*!
    Returns the requested manual shutter speed in seconds
    or -1.0 if automatic shutter speed is turned on.
*/
qreal QCamera::requestedShutterSpeed() const
{
    return d_func()->requestedExposureParameter<qreal>(QPlatformCameraExposure::ShutterSpeed, -1.0);
}

/*!
    Returns the list of shutter speed values in seconds camera supports.

    If the camera supports arbitrary shutter speed values within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<qreal> QCamera::supportedShutterSpeeds(bool *continuous) const
{
    QList<qreal> res;
    QPlatformCameraExposure *control = d_func()->exposureControl;

    bool tmp = false;
    if (!continuous)
        continuous = &tmp;

    if (!control)
        return res;

    const auto range = control->supportedParameterRange(QPlatformCameraExposure::ShutterSpeed, continuous);
    for (const QVariant &value : range) {
        bool ok = false;
        qreal realValue = value.toReal(&ok);
        if (ok)
            res.append(realValue);
        else
            qWarning() << "Incompatible shutter speed value type, qreal is expected";
    }

    return res;
}

/*!
    Set the manual shutter speed to \a seconds
*/

void QCamera::setManualShutterSpeed(qreal seconds)
{
    d_func()->setExposureParameter<qreal>(QPlatformCameraExposure::ShutterSpeed, seconds);
}

/*!
    Turn on auto shutter speed
*/

void QCamera::setAutoShutterSpeed()
{
    d_func()->resetExposureParameter(QPlatformCameraExposure::ShutterSpeed);
}


/*!
    \enum QCamera::FlashMode

    \value FlashOff             Flash is Off.
    \value FlashOn              Flash is On.
    \value FlashAuto            Automatic flash.
*/

/*!
    \enum QCamera::TorchMode

    \value TorchOff             Torch is Off.
    \value TorchOn              Torch is On.
    \value TorchAuto            Automatic torch.
*/

/*!
    \enum QCamera::ExposureMode

    \value ExposureAuto          Automatic mode.
    \value ExposureManual        Manual mode.
    \value ExposurePortrait      Portrait exposure mode.
    \value ExposureNight         Night mode.
    \value ExposureSports        Spots exposure mode.
    \value ExposureSnow          Snow exposure mode.
    \value ExposureBeach         Beach exposure mode.
    \value ExposureAction        Action mode. Since 5.5
    \value ExposureLandscape     Landscape mode. Since 5.5
    \value ExposureNightPortrait Night portrait mode. Since 5.5
    \value ExposureTheatre       Theatre mode. Since 5.5
    \value ExposureSunset        Sunset mode. Since 5.5
    \value ExposureSteadyPhoto   Steady photo mode. Since 5.5
    \value ExposureFireworks     Fireworks mode. Since 5.5
    \value ExposureParty         Party mode. Since 5.5
    \value ExposureCandlelight   Candlelight mode. Since 5.5
    \value ExposureBarcode       Barcode mode. Since 5.5
*/

/*!
    \property QCamera::flashReady
    \brief Indicates if the flash is charged and ready to use.
*/

/*!
    \fn void QCamera::flashReady(bool ready)

    Signal the flash \a ready status has changed.
*/

/*!
    \fn void QCamera::shutterSpeedRangeChanged()

    Signal emitted when the shutter speed range has changed.
*/


/*!
    \fn void QCamera::isoSensitivityChanged(int value)

    Signal emitted when sensitivity changes to \a value.
*/

/*!
    \fn void QCamera::exposureCompensationChanged(qreal value)

    Signal emitted when the exposure compensation changes to \a value.
*/



/*!
    Returns the white balance mode being used.
*/

QCamera::WhiteBalanceMode QCamera::whiteBalanceMode() const
{
    Q_D(const QCamera);
    return d->whiteBalance;
}

/*!
    Sets the white balance to \a mode.
*/

void QCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    Q_D(QCamera);
    if (d->whiteBalance == mode || !isWhiteBalanceModeSupported(mode))
        return;

    d->imageControl->setParameter(
        QPlatformCameraImageProcessing::WhiteBalancePreset,
        QVariant::fromValue<QCamera::WhiteBalanceMode>(mode));
    d->whiteBalance = mode;
    emit whiteBalanceModeChanged();
}

/*!
    Returns true if the white balance \a mode is supported.
*/

bool QCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    Q_D(const QCamera);
    if (!d->imageControl)
        return false;
    return d->imageControl->isParameterValueSupported(
        QPlatformCameraImageProcessing::WhiteBalancePreset,
        QVariant::fromValue<QCamera::WhiteBalanceMode>(mode));

}

/*!
    Returns the current color temperature if the
    current white balance mode is \c WhiteBalanceManual.  For other modes the
    return value is undefined.
*/

qreal QCamera::manualWhiteBalance() const
{
    Q_D(const QCamera);
    return d->colorTemperature;
}

/*!
    Sets manual white balance to \a colorTemperature.  This is used
    when whiteBalanceMode() is set to \c WhiteBalanceManual.  The units are Kelvin.
*/

void QCamera::setManualWhiteBalance(qreal colorTemperature)
{
    Q_D(QCamera);
    if (!d->imageControl)
        return;
    if (d->colorTemperature == colorTemperature)
        return;
    if (colorTemperature == 0) {
        setWhiteBalanceMode(WhiteBalanceAuto);
    } else if (!isWhiteBalanceModeSupported(WhiteBalanceManual)) {
        return;
    } else {
        setWhiteBalanceMode(WhiteBalanceManual);
        d->imageControl->setParameter(
            QPlatformCameraImageProcessing::ColorTemperature,
            QVariant(colorTemperature));
    }
    d->colorTemperature = colorTemperature;
    emit manualWhiteBalanceChanged();
}

/*!
    Returns the brightness adjustment setting.
 */
qreal QCamera::brightness() const
{
    Q_D(const QCamera);
    return d->brightness;
}

/*!
    Set the brightness adjustment to \a value.

    Valid brightness adjustment values range between -1.0 and 1.0, with a default of 0.
*/
void QCamera::setBrightness(qreal value)
{
    Q_D(QCamera);
    if (!d->imageControl || d->brightness == value)
        return;
    d->brightness = value;
    d->imageControl->setParameter(QPlatformCameraImageProcessing::BrightnessAdjustment,
                                  QVariant(value));
    emit brightnessChanged();
}

/*!
    Returns the contrast adjustment setting.
*/
qreal QCamera::contrast() const
{
    Q_D(const QCamera);
    return d->contrast;
}

/*!
    Set the contrast adjustment to \a value.

    Valid contrast adjustment values range between -1.0 and 1.0, with a default of 0.
*/
void QCamera::setContrast(qreal value)
{
    Q_D(QCamera);
    if (!d->imageControl || d->contrast == value)
        return;
    d->contrast = value;
    d->imageControl->setParameter(QPlatformCameraImageProcessing::ContrastAdjustment,
                                  QVariant(value));
    emit contrastChanged();
}

/*!
    Returns the saturation adjustment value.
*/
qreal QCamera::saturation() const
{
    Q_D(const QCamera);
    return d->saturation;
}

/*!
    Sets the saturation adjustment value to \a value.

    Valid saturation values range between -1.0 and 1.0, with a default of 0.
*/
void QCamera::setSaturation(qreal value)
{
    Q_D(QCamera);
    if (!d->imageControl || d->saturation == value)
        return;
    d->saturation = value;
    d->imageControl->setParameter(QPlatformCameraImageProcessing::SaturationAdjustment,
                                  QVariant(value));
    emit saturationChanged();
}

qreal QCamera::hue() const
{
    Q_D(const QCamera);
    return d->hue;
}

/*!
    Sets the hue adjustment value to \a value.

    Valid hue values range between -1.0 and 1.0, with a default of 0.
*/
void QCamera::setHue(qreal value)
{
    Q_D(QCamera);
    if (!d->imageControl || d->hue == value)
        return;
    d->hue = value;
    d->imageControl->setParameter(QPlatformCameraImageProcessing::HueAdjustment,
                                  QVariant(value));
    emit hueChanged();
}

/*!
    \enum QCamera::WhiteBalanceMode

    \value WhiteBalanceAuto         Auto white balance mode.
    \value WhiteBalanceManual       Manual white balance. In this mode the white balance should be set with
                                    setManualWhiteBalance()
    \value WhiteBalanceSunlight     Sunlight white balance mode.
    \value WhiteBalanceCloudy       Cloudy white balance mode.
    \value WhiteBalanceShade        Shade white balance mode.
    \value WhiteBalanceTungsten     Tungsten (incandescent) white balance mode.
    \value WhiteBalanceFluorescent  Fluorescent white balance mode.
    \value WhiteBalanceFlash        Flash white balance mode.
    \value WhiteBalanceSunset       Sunset white balance mode.
*/

QT_END_NAMESPACE

#include "moc_qcamera.cpp"
