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

#include <qcameradevice.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
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

    You can use QCameraDevice to list available cameras and choose which one to use.

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

    if (cameraDevice.isNull())
        _q_error(QCamera::CameraError, QString::fromUtf8("Invalid camera specified"));
    control->setCamera(cameraDevice);
    q->connect(control, SIGNAL(activeChanged(bool)), q, SIGNAL(activeChanged(bool)));
    q->connect(control, SIGNAL(error(int,QString)), q, SLOT(_q_error(int,QString)));
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

    Construct a QCamera from a camera description \a cameraDevice and \a parent.
*/

QCamera::QCamera(const QCameraDevice &cameraDevice, QObject *parent)
    : QObject(*new QCameraPrivate, parent)
{
    Q_D(QCamera);

    d->cameraDevice = cameraDevice;
    d->init();
    setCameraDevice(cameraDevice);
}

/*!
    \since 5.3

    Construct a QCamera which uses a hardware camera located a the specified \a position.

    For example on a mobile phone it can be used to easily choose between front-facing and
    back-facing cameras.

    If no camera is available at the specified \a position or if \a position is
    QCameraDevice::UnspecifiedPosition, the default camera is used.
*/

QCamera::QCamera(QCameraDevice::Position position, QObject *parent)
    : QObject(*new QCameraPrivate, parent)
{
    Q_D(QCamera);

    QCameraDevice info;
    auto cameras = QMediaDevices::videoInputs();
    for (const auto &c : cameras) {
        if (c.position() == position) {
            info = c;
            break;
        }
    }
    if (info.isNull())
        info = QMediaDevices::defaultVideoInput();
    d->cameraDevice = info;
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
    return d->control && !d->cameraDevice.isNull();
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

QCamera::Features QCamera::supportedFeatures() const
{
    Q_D(const QCamera);
    return d->control ? d->control->supportedFeatures() : QCamera::Features{};
}

/*! \fn void QCamera::start()

    Starts the camera.

    Same as setActive(true).

    If the camera can't be started for some reason, the errorOccurred() signal is emitted.
*/

/*! \fn void QCamera::stop()

    Stops the camera.

    \sa unload()
*/

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

    if (d->captureSession == session)
        return;

    if (d->captureInterface)
        d->captureInterface->setCamera(nullptr);

    d->captureSession = session;
    d->captureInterface = session ? session->platformSession() : nullptr;
    if (d->captureInterface && d->control)
        d->captureInterface->setCamera(d->control);
}

/*!
    Returns the QCameraDevice object associated with this camera.
 */
QCameraDevice QCamera::cameraDevice() const
{
    Q_D(const QCamera);
    return d->cameraDevice;
}

/*!
    Sets the camera object to use the physical camera described by
    \a cameraDevice.
*/
void QCamera::setCameraDevice(const QCameraDevice &cameraDevice)
{
    Q_D(QCamera);
    if (d->cameraDevice == cameraDevice)
        return;
    d->cameraDevice = cameraDevice;
    if (d->cameraDevice.isNull())
        d->_q_error(QCamera::CameraError, QString::fromUtf8("Invalid camera specified"));
    else
        d->_q_error(QCamera::NoError, QString());
    if (d->control)
        d->control->setCamera(d->cameraDevice);
    emit cameraDeviceChanged();
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
    \enum QCamera::Error

    This enum holds the last error code.

    \value  NoError      No errors have occurred.
    \value  CameraError  An error has occurred.
*/

/*!
    \fn void QCamera::errorOccurred(QCamera::Error error, const QString &errorString)

    This signal is emitted when error state changes to \a error. A description
    of the error is  provided as \a errorString.
*/

/*!
    \enum QCameraDevice::Position
    \since 5.3

    This enum specifies the physical position of the camera on the system hardware.

    \value UnspecifiedPosition  The camera position is unspecified or unknown.

    \value BackFace  The camera is on the back face of the system hardware. For example on a
    mobile device, it means it is on the opposite side to that of the screen.

    \value FrontFace  The camera is on the front face of the system hardware. For example on a
    mobile device, it means it is on the same side as that of the screen. Viewfinder frames of
    front-facing cameras are mirrored horizontally, so the users can see themselves as looking
    into a mirror. Captured images or videos are not mirrored.

    \sa QCameraDevice::position()
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
    \property QCamera::flashMode
    \brief The flash mode being used.

    Enables a certain flash mode if the camera has a flash.

    \sa QCamera::isFlashModeSupported(), QCamera::isFlashReady()
*/
QCamera::FlashMode QCamera::flashMode() const
{
    Q_D(const QCamera);
    return d->control ? d->control->flashMode() : QCamera::FlashOff;
}

void QCamera::setFlashMode(QCamera::FlashMode mode)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setFlashMode(mode);
}

/*!
    Returns true if the flash \a mode is supported.
*/

bool QCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    Q_D(const QCamera);
    return d->control ? d->control->isFlashModeSupported(mode) : (mode == FlashOff);
}

/*!
    Returns true if flash is charged.
*/

bool QCamera::isFlashReady() const
{
    Q_D(const QCamera);
    return d->control ? d->control->isFlashReady() : false;
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
    Q_D(const QCamera);
    return d->control ? d->control->torchMode() : TorchOff;
}

void QCamera::setTorchMode(QCamera::TorchMode mode)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setTorchMode(mode);
}

/*!
    Returns true if the torch \a mode is supported.
*/
bool QCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    Q_D(const QCamera);
    return d->control ? d->control->isTorchModeSupported(mode) : (mode == TorchOff);
}

/*!
  \property QCamera::exposureMode
  \brief The exposure mode being used.

  \sa QCamera::isExposureModeSupported()
*/

QCamera::ExposureMode QCamera::exposureMode() const
{
    Q_D(const QCamera);
    return d->control ? d->control->exposureMode() : QCamera::ExposureAuto;
}

void QCamera::setExposureMode(QCamera::ExposureMode mode)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setExposureMode(mode);
}

/*!
    Returns true if the exposure \a mode is supported.
*/

bool QCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    Q_D(const QCamera);
    if (!d->control)
        return false;

    return d->control->isExposureModeSupported(mode);
}

/*!
  \property QCamera::exposureCompensation
  \brief Exposure compensation in EV units.

  Exposure compensation property allows to adjust the automatically calculated exposure.
*/

qreal QCamera::exposureCompensation() const
{
    Q_D(const QCamera);
    return d->control ? d->control->exposureCompensation() : 0.;
}

void QCamera::setExposureCompensation(qreal ev)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setExposureCompensation(ev);
}

int QCamera::isoSensitivity() const
{
    Q_D(const QCamera);
    return d->control ? d->control->isoSensitivity() : -1;
}

/*!
    \fn QCamera::setManualIsoSensitivity(int iso)
    Sets the manual sensitivity to \a iso
*/

void QCamera::setManualIsoSensitivity(int iso)
{
    Q_D(QCamera);
    if (iso <= 0)
        iso = -1;
    if (d->control)
        d->control->setManualIsoSensitivity(iso);
}

int QCamera::manualIsoSensitivity() const
{
    Q_D(const QCamera);
    return d->control ? d->control->manualIsoSensitivity() : 100;
}

/*!
     \fn QCamera::setAutoIsoSensitivity()
     Turn on auto sensitivity
*/

void QCamera::setAutoIsoSensitivity()
{
    Q_D(QCamera);
    if (d->control)
        d->control->setManualIsoSensitivity(-1);
}

int QCamera::minimumIsoSensitivity() const
{
    Q_D(const QCamera);
    return d->control ? d->control->minIso() : -1;
}

int QCamera::maximumIsoSensitivity() const
{
    Q_D(const QCamera);
    return d->control ? d->control->maxIso() : -1;
}

/*!
    The minimal exposure time in seconds.
*/
float QCamera::minimumExposureTime() const
{
    Q_D(const QCamera);
    return d->control ? d->control->minExposureTime() : -1.;
}

/*!
    The maximal exposure time in seconds.
*/
float QCamera::maximumExposureTime() const
{
    Q_D(const QCamera);
    return d->control ? d->control->maxExposureTime() : -1.;
}

/*!
    \property QCamera::exposureTime
    \brief Camera's exposure time in seconds.

    \sa minimumExposureTime(), maximumExposureTime(), setManualExposureTime()
*/

/*!
    \fn QCamera::exposureTimeChanged(float time)

    Signals that a camera's exposure \a time has changed.
*/

/*!
    \property QCamera::isoSensitivity
    \brief The sensor ISO sensitivity.

    \sa supportedIsoSensitivities(), setAutoIsoSensitivity(), setManualIsoSensitivity()
*/

/*!
    Returns the current exposure time in seconds.
*/

float QCamera::exposureTime() const
{
    Q_D(const QCamera);
    return d->control ? d->control->exposureTime() : -1;
}

/*!
    Set the manual exposure time to \a seconds
*/

void QCamera::setManualExposureTime(float seconds)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setManualExposureTime(seconds);
}

/*!
    Returns the manual exposure time in \a seconds, or -1
    if the camera is using automatic exposure times.
*/
float QCamera::manualExposureTime() const
{
    Q_D(const QCamera);
    return d->control ? d->control->manualExposureTime() : -1;
}

/*!
    Use automatically calculated exposure time
*/
void QCamera::setAutoExposureTime()
{
    Q_D(QCamera);
    if (d->control)
        d->control->setManualExposureTime(-1);
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
    return d->control ? d->control->whiteBalanceMode() : QCamera::WhiteBalanceAuto;
}

/*!
    Sets the white balance to \a mode.
*/

void QCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    Q_D(QCamera);
    if (!d->control)
        return;
    if (!d->control->isWhiteBalanceModeSupported(mode))
        return;
    d->control->setWhiteBalanceMode(mode);
    if (mode == QCamera::WhiteBalanceManual)
        d->control->setColorTemperature(5600);
}

/*!
    Returns true if the white balance \a mode is supported.
*/

bool QCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    Q_D(const QCamera);
    if (!d->control)
        return false;
    return d->control->isWhiteBalanceModeSupported(mode);
}

/*!
    Returns the current color temperature if the
    current white balance mode is \c WhiteBalanceManual.  For other modes the
    return value is undefined.
*/

int QCamera::colorTemperature() const
{
    Q_D(const QCamera);
    return d->control ? d->control->colorTemperature() : 0;
}

/*!
    Sets manual white balance to \a colorTemperature.  This is used
    when whiteBalanceMode() is set to \c WhiteBalanceManual.  The units are Kelvin.

    Setting a color temperature will only have an effect if WhiteBalanceManual is
    supported. In this case, setting a temperature greater 0 will automatically set the
    white balance mode to WhiteBalanceManual. Setting the temperature to 0 will reset
    the white balance mode to WhiteBalanceAuto.
*/

void QCamera::setColorTemperature(int colorTemperature)
{
    Q_D(QCamera);
    if (!d->control)
        return;
    if (colorTemperature < 0)
        colorTemperature = 0;
    if (colorTemperature == 0) {
        d->control->setWhiteBalanceMode(WhiteBalanceAuto);
    } else if (!isWhiteBalanceModeSupported(WhiteBalanceManual)) {
        return;
    } else {
        d->control->setWhiteBalanceMode(WhiteBalanceManual);
    }
    d->control->setColorTemperature(colorTemperature);
}

/*!
    \enum QCamera::WhiteBalanceMode

    \value WhiteBalanceAuto         Auto white balance mode.
    \value WhiteBalanceManual       Manual white balance. In this mode the white balance should be set with
                                    setColorTemperature()
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
