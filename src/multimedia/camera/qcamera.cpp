// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


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
    and zoom. This also includes functionality such as a
    "Macro" mode for close up work (e.g. reading barcodes, or
    recognizing letters), or "touch to focus" - indicating an
    interesting area of the image for the hardware to attempt
    to focus on.

    \snippet multimedia-snippets/camera.cpp Camera custom focus

    The \l minimumZoomFactor() and \l maximumZoomFactor() methods provide the
    range of supported zoom factors. The \l zoomTo() method allows changing
    the zoom factor.

    \snippet multimedia-snippets/camera.cpp Camera zoom


    After capturing the raw data for a camera frame, the camera hardware and
    software performs various image processing tasks to produce the final
    image.  This includes compensating for ambient light color, reducing
    noise, as well as making some other adjustments to the image.

    You can control many of these processing steps through the Camera properties.
    For example, you can set the white balance (or color temperature) used
    for processing images:

    \snippet multimedia-snippets/camera.cpp Camera image whitebalance

    For more information on image processing of camera frames, see
    \l {camera_image_processing}{Camera Image Processing}.

    See the \l{Camera Overview}{camera overview} for more information.
*/

/*!
    \qmltype Camera
    \instantiates QCamera
    \inqmlmodule QtMultimedia
    \brief An interface for camera settings related to focus and zoom.
    \ingroup multimedia_qml
    \ingroup camera_qml

    The Camera element can be used within a \l CaptureSession for video recording
    and image taking.

    You can use \l MediaDevices to list available cameras and choose which one to use.

    \qml
    MediaDevices {
        id: mediaDevices
    }
    CaptureSession {
        camera: Camera {
            cameraDevice: mediaDevices.defaultVideoInput
        }
    }
    \endqml

    On hardware that supports it, QCamera lets you adjust the focus
    and zoom. This also includes functionality such as a
    "Macro" mode for close up work (e.g. reading barcodes, or
    recognizing letters), or "touch to focus" - indicating an
    interesting area of the image for the hardware to attempt
    to focus on.

    \qml

    Item {
        width: 640
        height: 360

        CaptureSession {
            camera: Camera {
                id: camera

                focusMode: Camera.FocusModeAutoNear
                customFocusPoint: Qt.point(0.2, 0.2) // Focus relative to top-left corner
            }
            videoOutput: videoOutput
        }

        VideoOutput {
            id: videoOutput
            anchors.fill: parent
        }
    }

    \endqml

    The \l minimumZoomFactor and \l maximumZoomFactor properties provide the
    range of supported zoom factors. The \l zoomFactor property allows changing
    the zoom factor.

    \qml
    Camera {
        zoomFactor: maximumZoomFactor // zoom in as much as possible
    }
    \endqml

    After capturing the raw data for a camera frame, the camera hardware and
    software performs various image processing tasks to produce the final
    image.  This includes compensating for ambient light color, reducing
    noise, as well as making some other adjustments to the image.

    You can control many of these processing steps through the Camera properties.
    For example, you can set the white balance (or color temperature) used
    for processing images:

    \qml
    Camera {
        whiteBalanceMode: Camera.WhiteBalanceManual
        colorTemperature: 5600
    }
    \endqml

    For more information on image processing of camera frames, see
    \l {camera_image_processing}{Camera Image Processing}.

    See the \l{Camera Overview}{camera overview} for more information.
*/


void QCameraPrivate::_q_error(int error, const QString &errorString)
{
    Q_Q(QCamera);

    this->error.setAndNotify(QCamera::Error(error), errorString, *q);
}

void QCameraPrivate::init(const QCameraDevice &device)
{
    Q_Q(QCamera);

    auto maybeControl = QPlatformMediaIntegration::instance()->createCamera(q);
    if (!maybeControl) {
        qWarning() << "Failed to initialize QCamera" << maybeControl.error();
        error = { QCamera::CameraError, maybeControl.error() };
        return;
    }
    control = maybeControl.value();
    cameraDevice = !device.isNull() ? device : QMediaDevices::defaultVideoInput();
    if (cameraDevice.isNull())
        _q_error(QCamera::CameraError, QString::fromUtf8("No camera detected"));
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
    d->init(cameraDevice);
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

    QCameraDevice device;
    auto cameras = QMediaDevices::videoInputs();
    for (const auto &c : cameras) {
        if (c.position() == position) {
            device = c;
            break;
        }
    }
    d->init(device);
}

/*!
    Destroys the camera object.
*/

QCamera::~QCamera()
{
    Q_D(QCamera);
    if (d->captureSession)
        d->captureSession->setCamera(nullptr);
}

/*!
    Returns true if the camera can be used.
*/
bool QCamera::isAvailable() const
{
    Q_D(const QCamera);
    return d->control && !d->cameraDevice.isNull();
}

/*! \qmlproperty bool QtMultimedia::Camera::active

    Describes whether the camera is currently active.
*/

/*! \property QCamera::active

    Describes whether the camera is currently active.
*/

/*!
    Returns true if the camera is currently active.
*/
bool QCamera::isActive() const
{
    Q_D(const QCamera);
    return d->control && d->control->isActive();
}

/*!
    Turns the camera on if \a active is \c{true}, or off if it's \c{false}.
*/
void QCamera::setActive(bool active)
{
    Q_D(const QCamera);
    if (d->control)
        d->control->setActive(active);
}

/*!
    \qmlproperty enumeration QtMultimedia::Camera::error

    Returns the error state of the camera.

    \sa QCamera::Error
*/

/*!
    \property QCamera::error

    Returns the error state of the camera.
*/

QCamera::Error QCamera::error() const
{
    return d_func()->error.code();
}

/*!
    \qmlproperty string QtMultimedia::Camera::errorString

    Returns a human readable string describing a camera's error state.
*/

/*!
    \property QCamera::errorString

    Returns a human readable string describing a camera's error state.
*/
QString QCamera::errorString() const
{
    return d_func()->error.description();
}

/*! \enum QCamera::Feature

    Describes a set of features supported by the camera. The returned value can be a
    combination of:

    \value ColorTemperature
        The Camera supports setting a custom \l{colorTemperature}.
    \value ExposureCompensation
        The Camera supports setting a custom \l{exposureCompensation}.
    \value IsoSensitivity
        The Camera supports setting a custom \l{isoSensitivity}.
    \value ManualExposureTime
        The Camera supports setting a \l{QCamera::manualExposureTime}{manual exposure Time}.
    \value CustomFocusPoint
        The Camera supports setting a \l{QCamera::customFocusPoint}{custom focus point}.
    \value FocusDistance
        The Camera supports setting the \l{focusDistance} property.
*/

/*!
    \qmlproperty Features QtMultimedia::Camera::supportedFeatures
    Returns the features supported by this camera.

    \sa QCamera::Feature
*/

/*!
    \property QCamera::supportedFeatures

    Returns the features supported by this camera.

    \sa QCamera::Feature
*/
QCamera::Features QCamera::supportedFeatures() const
{
    Q_D(const QCamera);
    return d->control ? d->control->supportedFeatures() : QCamera::Features{};
}

/*! \qmlmethod void Camera::start()

    Starts the camera.

    Same as setting the active property to true.

    If the camera can't be started for some reason, the errorOccurred() signal is emitted.
*/

/*! \fn void QCamera::start()

    Starts the camera.

    Same as setActive(true).

    If the camera can't be started for some reason, the errorOccurred() signal is emitted.
*/

/*! \qmlmethod void Camera::stop()

    Stops the camera.
    Same as setting the active property to false.
*/

/*! \fn void QCamera::stop()

    Stops the camera.
    Same as setActive(false).
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
    d->captureSession = session;
}

/*!
    \internal
*/
QPlatformCamera *QCamera::platformCamera()
{
    Q_D(const QCamera);
    return d->control;
}

/*! \qmlproperty cameraDevice QtMultimedia::Camera::cameraDevice

    Gets or sets the currently active camera device.
*/

/*!
    \property QCamera::cameraDevice

    Returns the QCameraDevice object associated with this camera.
 */
QCameraDevice QCamera::cameraDevice() const
{
    Q_D(const QCamera);
    return d->cameraDevice;
}

/*!
    Connects the camera object to the physical camera device described by
    \a cameraDevice. Using a default constructed QCameraDevice object as
    \a cameraDevice will connect the camera to the system default camera device.
*/
void QCamera::setCameraDevice(const QCameraDevice &cameraDevice)
{
    Q_D(QCamera);
    auto dev = cameraDevice;
    if (dev.isNull())
        dev = QMediaDevices::defaultVideoInput();
    if (d->cameraDevice == dev)
        return;
    d->cameraDevice = dev;
    if (d->control)
        d->control->setCamera(d->cameraDevice);
    emit cameraDeviceChanged();
    setCameraFormat({});
}

/*! \qmlproperty cameraFormat QtMultimedia::Camera::cameraFormat

    Gets or sets the currently active camera format.

    \note When using the FFMPEG backend on an Android target device if you request
    \b YUV420P format, you will receive either a fully planar 4:2:0 YUV420P or a
    semi-planar NV12/NV21. This depends on the codec implemented by the device
    OEM.

    \sa cameraDevice::videoFormats
*/

/*!
    \property QCamera::cameraFormat

    Returns the camera format currently used by the camera.

    \note When using the FFMPEG backend on an Android target device if you request
    \b YUV420P format, you will receive either a fully planar 4:2:0 YUV420P or a
    semi-planar NV12/NV21. This depends on the codec implemented by the device
    OEM.

    \sa QCameraDevice::videoFormats
*/
QCameraFormat QCamera::cameraFormat() const
{
    Q_D(const QCamera);
    return d->cameraFormat;
}

/*!
    Tells the camera to use the format described by \a format. This can be used to define
    a specific resolution and frame rate to be used for recording and image capture.

    \note When using the FFMPEG backend on an Android target device if you request
    \b YUV420P format, you will receive either a fully planar 4:2:0 YUV420P or a
    semi-planar NV12/NV21. This depends on the codec implemented by the device
    OEM.
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
    \qmlsignal void Camera::errorOccurred(Camera::Error error, string errorString)

    This signal is emitted when error state changes to \a error. A description
    of the error is  provided as \a errorString.
*/

/*!
    \fn void QCamera::errorOccurred(QCamera::Error error, const QString &errorString)

    This signal is emitted when error state changes to \a error. A description
    of the error is  provided as \a errorString.
*/

/*!
    \qmlproperty enumeration Camera::focusMode

    This property holds the current camera focus mode.

    \note In automatic focusing modes and where supported, the \l focusPoint property provides
    information and control over the area of the image that is being focused.

    \value Camera.FocusModeAuto Continuous auto focus mode.
    \value Camera.FocusModeAutoNear Continuous auto focus, preferring objects near to
        the camera.
    \value Camera.FocusModeAutoFar Continuous auto focus, preferring objects far away
        from the camera.
    \value Camera.FocusModeHyperfocal Focus to hyperfocal distance, with the maximum
        depth of field achieved. All objects at distances from half of this
        distance out to infinity will be acceptably sharp.
    \value Camera.FocusModeInfinity Focus strictly to infinity.
    \value Camera.FocusModeManual Manual or fixed focus mode.

    If a certain focus mode is not supported, setting it will have no effect.

    \sa isFocusModeSupported
*/

/*!
    \property QCamera::focusMode
    \brief the current camera focus mode.

    Sets up different focus modes for the camera. All auto focus modes will focus continuously.
    Locking the focus is possible by setting the focus mode to \l FocusModeManual. This will keep
    the current focus and stop any automatic focusing.

    \sa isFocusModeSupported
*/
QCamera::FocusMode QCamera::focusMode() const
{
    Q_D(const QCamera);
    return d->control ? d->control->focusMode() : QCamera::FocusModeAuto;
}

/*!
    \fn void QCamera::focusModeChanged()

    Signals when the focusMode changes.
*/
void QCamera::setFocusMode(QCamera::FocusMode mode)
{
    Q_D(QCamera);
    if (!d->control || d->control->focusMode() == mode)
        return;
    d->control->setFocusMode(mode);
    emit focusModeChanged();
}

/*!
    \qmlmethod bool Camera::isFocusModeSupported(FocusMode mode)

    Returns true if the focus \a mode is supported by the camera.
*/

/*!
    Returns true if the focus \a mode is supported by the camera.
*/
bool QCamera::isFocusModeSupported(FocusMode mode) const
{
    Q_D(const QCamera);
    return d->control ? d->control->isFocusModeSupported(mode) : false;
}

/*!
    \qmlproperty point QtMultimedia::Camera::focusPoint
    Returns the point currently used by the auto focus system to focus onto.
*/

/*!
    \property QCamera::focusPoint

    Returns the point currently used by the auto focus system to focus onto.
 */
QPointF QCamera::focusPoint() const
{
    Q_D(const QCamera);
    return d->control ? d->control->focusPoint() : QPointF(-1., -1.);

}

/*!
    \qmlproperty point QtMultimedia::Camera::customFocusPoint

    This property holds the position of custom focus point, in relative frame
    coordinates. This means that QPointF(0,0) points to the top-left corner
    of the frame, and QPointF(0.5,0.5) points to the center of the frame.

    Custom focus point is used only in \c FocusPointCustom focus mode.

    You can check whether custom focus points are supported by querying
    supportedFeatures() with the Feature.CustomFocusPoint flag.
*/

/*!
    \property QCamera::customFocusPoint

    This property represents the position of the custom focus point, in relative frame coordinates:
    QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center.

    The custom focus point property is used only in \c FocusPointCustom focus mode.

    You can check whether custom focus points are supported by querying
    supportedFeatures() with the Feature.CustomFocusPoint flag.
*/
QPointF QCamera::customFocusPoint() const
{
    Q_D(const QCamera);
    return d->control ? d->control->customFocusPoint() : QPointF{-1., -1.};
}

void QCamera::setCustomFocusPoint(const QPointF &point)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setCustomFocusPoint(point);
}

/*!
    \qmlproperty float QtMultimedia::Camera::focusDistance

    This property return an approximate focus distance of the camera. The value reported
    is between 0 and 1, 0 being the closest possible focus distance, 1 being as far away
    as possible. Note that 1 is often, but not always infinity.

    Setting the focus distance will be ignored unless the focus mode is set to
    \l {focusMode}{FocusModeManual}.
*/

/*!
    \property QCamera::focusDistance

    This property return an approximate focus distance of the camera. The value reported
    is between 0 and 1, 0 being the closest possible focus distance, 1 being as far away
    as possible. Note that 1 is often, but not always infinity.

    Setting the focus distance will be ignored unless the focus mode is set to
    \l FocusModeManual.
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
    \qmlproperty real QtMultimedia::Camera::maximumZoomFactor

    This property holds the maximum zoom factor supported.

    This will be \c 1.0 on cameras that do not support zooming.
*/


/*!
    \property QCamera::maximumZoomFactor

    Returns the maximum zoom factor.

    This will be \c 1.0 on cameras that do not support zooming.
*/

float QCamera::maximumZoomFactor() const
{
    Q_D(const QCamera);
    return d->control ? d->control->maxZoomFactor() : 1.f;
}

/*!
    \qmlproperty real QtMultimedia::Camera::minimumZoomFactor

    This property holds the minimum zoom factor supported.

    This will be \c 1.0 on cameras that do not support zooming.
*/

/*!
    \property QCamera::minimumZoomFactor

    Returns the minimum zoom factor.

    This will be \c 1.0 on cameras that do not support zooming.
*/

float QCamera::minimumZoomFactor() const
{
    Q_D(const QCamera);
    return d->control ? d->control->minZoomFactor() : 1.f;
}

/*!
    \qmlproperty real QtMultimedia::Camera::zoomFactor

    Gets or sets the current zoom factor. Values will be clamped between
    \l minimumZoomFactor and \l maximumZoomFactor.
*/

/*!
    \property QCamera::zoomFactor
    \brief The current zoom factor.

    Gets or sets the current zoom factor. Values will be clamped between
    \l minimumZoomFactor and \l maximumZoomFactor.
*/
float QCamera::zoomFactor() const
{
    Q_D(const QCamera);
    return d->control ? d->control->zoomFactor() : 1.f;
}
/*!
    Zooms to a zoom factor \a factor at a rate of 1 factor per second.
 */
void QCamera::setZoomFactor(float factor)
{
    zoomTo(factor, 0.f);
}

/*!
    \qmlmethod void QtMultimedia::Camera::zoomTo(factor, rate)

    Zooms to a zoom factor \a factor using \a rate.

    The \a rate is specified in powers of two per second. At a rate of 1
    it would take 2 seconds to go from a zoom factor of 1 to 4.

    \note Using a specific rate is not supported on all cameras. If not supported,
    zooming will happen as fast as possible.
*/

/*!
    Zooms to a zoom factor \a factor using \a rate.

    The \a rate is specified in powers of two per second. At a rate of 1
    it would take 2 seconds to go from a zoom factor of 1 to 4.

    \note Using a specific rate is not supported on all cameras. If not supported,
    zooming will happen as fast as possible.
*/
void QCamera::zoomTo(float factor, float rate)
{
    Q_ASSERT(rate >= 0.f);
    if (rate < 0.f)
        rate = 0.f;

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
    \qmlproperty enumeration QtMultimedia::Camera::flashMode

    Gets or sets a certain flash mode if the camera has a flash.

    \value Camera.FlashOff      Flash is Off.
    \value Camera.FlashOn       Flash is On.
    \value Camera.FlashAuto     Automatic flash.

    \sa isFlashModeSupported, isFlashReady
*/

/*!
    \property QCamera::flashMode
    \brief The flash mode being used.

    Enables a certain flash mode if the camera has a flash.

    \sa QCamera::FlashMode, QCamera::isFlashModeSupported, QCamera::isFlashReady
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
    \qmlmethod bool QtMultimedia::Camera::isFlashModeSupported(FlashMode mode)

    Returns true if the flash \a mode is supported.
*/

/*!
    Returns true if the flash \a mode is supported.
*/
bool QCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    Q_D(const QCamera);
    return d->control ? d->control->isFlashModeSupported(mode) : (mode == FlashOff);
}

/*!
    \qmlmethod bool QtMultimedia::Camera::isFlashReady()

    Returns true if flash is charged.
*/

/*!
    Returns true if flash is charged.
*/
bool QCamera::isFlashReady() const
{
    Q_D(const QCamera);
    return d->control ? d->control->isFlashReady() : false;
}

/*!
    \qmlproperty Camera::TorchMode Camera::torchMode

    Gets or sets the torch mode being used.

    A torch is a continuous source of light. It can be used during video recording in
    low light conditions. Enabling torch mode will usually override any currently set
    flash mode.

    \sa QCamera::TorchMode, Camera::isTorchModeSupported(), Camera::flashMode
*/

/*!
    \property QCamera::torchMode
    \brief The torch mode being used.

    A torch is a continuous source of light. It can be used during video recording in
    low light conditions. Enabling torch mode will usually override any currently set
    flash mode.

    \sa QCamera::TorchMode, QCamera::isTorchModeSupported, QCamera::flashMode
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
    \qmlmethod bool QtMultimedia::Camera::isTorchModeSupported(TorchMode mode)

    Returns true if the torch \a mode is supported.
*/

/*!
    Returns true if the torch \a mode is supported.
*/
bool QCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    Q_D(const QCamera);
    return d->control ? d->control->isTorchModeSupported(mode) : (mode == TorchOff);
}

/*!
    \qmlproperty ExposureMode QtMultimedia::Camera::exposureMode
    \brief The exposure mode being used.

    \sa QCamera::ExposureMode, Camera::isExposureModeSupported()
*/

/*!
  \property QCamera::exposureMode
  \brief The exposure mode being used.

  \sa QCamera::isExposureModeSupported
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
    \qmlmethod bool QtMultimedia::Camera::isExposureModeSupported(ExposureMode mode)

    Returns true if the exposure \a mode is supported.
*/

/*!
    Returns true if the exposure \a mode is supported.
*/
bool QCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    Q_D(const QCamera);
    return d->control && d->control->isExposureModeSupported(mode);
}

/*!
    \qmlproperty real QtMultimedia::Camera::exposureCompensation

    Gets or sets the exposure compensation in EV units.

    Exposure compensation property allows to adjust the automatically calculated
    exposure.
*/

/*!
  \property QCamera::exposureCompensation
  \brief Exposure compensation in EV units.

  Exposure compensation property allows to adjust the automatically calculated
  exposure.
*/
float QCamera::exposureCompensation() const
{
    Q_D(const QCamera);
    return d->control ? d->control->exposureCompensation() : 0.f;
}

void QCamera::setExposureCompensation(float ev)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setExposureCompensation(ev);
}

/*!
    \qmlproperty int QtMultimedia::Camera::isoSensitivity

    Describes the ISO sensitivity currently used by the camera.

*/

/*!
    \property QCamera::isoSensitivity
    \brief The sensor ISO sensitivity.

    Describes the ISO sensitivity currently used by the camera.

    \sa setAutoIsoSensitivity(), setManualIsoSensitivity()
*/
int QCamera::isoSensitivity() const
{
    Q_D(const QCamera);
    return d->control ? d->control->isoSensitivity() : -1;
}

/*!
    \qmlproperty int QtMultimedia::Camera::manualIsoSensitivity

    Describes a manually set ISO sensitivity

    Setting this property to -1 (the default), implies that the camera
    automatically adjusts the ISO sensitivity.
*/

/*!
    \property QCamera::manualIsoSensitivity
    \brief Describes a manually set ISO sensitivity

    Setting this property to -1 (the default), implies that the camera
    automatically adjusts the ISO sensitivity.
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

/*!
    Returns the minimum ISO sensitivity supported by the camera.
*/
int QCamera::minimumIsoSensitivity() const
{
    Q_D(const QCamera);
    return d->control ? d->control->minIso() : -1;
}

/*!
    Returns the maximum ISO sensitivity supported by the camera.
*/
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
    return d->control ? d->control->minExposureTime() : -1.f;
}

/*!
    The maximal exposure time in seconds.
*/
float QCamera::maximumExposureTime() const
{
    Q_D(const QCamera);
    return d->control ? d->control->maxExposureTime() : -1.f;
}

/*!
    \qmlproperty float QtMultimedia::Camera::exposureTime
    Returns the Camera's exposure time in seconds.

    \sa manualExposureTime
*/

/*!
    \property QCamera::exposureTime
    \brief Camera's exposure time in seconds.

    \sa minimumExposureTime(), maximumExposureTime(), setManualExposureTime()
*/

/*!
    \fn QCamera::exposureTimeChanged(float speed)

    Signals that a camera's exposure \a speed has changed.
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
    \qmlproperty real QtMultimedia::Camera::manualExposureTime

    Gets or sets a manual exposure time.

    Setting this property to -1 (the default) means that the camera
    automatically determines the exposure time.
*/

/*!
    \property QCamera::manualExposureTime

    Set the manual exposure time to \a seconds
*/

void QCamera::setManualExposureTime(float seconds)
{
    Q_D(QCamera);
    if (d->control)
        d->control->setManualExposureTime(seconds);
}

/*!
    Returns the manual exposure time in seconds, or -1
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
    \qmlproperty bool QtMultimedia::Camera::flashReady

    Indicates if the flash is charged and ready to use.
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
    \fn void QCamera::exposureCompensationChanged(float value)

    Signal emitted when the exposure compensation changes to \a value.
*/


/*!
    \qmlproperty WhiteBalanceMode QtMultimedia::Camera::whiteBalanceMode

    Gets or sets the white balance mode being used.

    \sa QCamera::WhiteBalanceMode
*/

/*!
    \property QCamera::whiteBalanceMode

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
    \qmlmethod bool QtMultimedia::Camera::isWhiteBalanceModeSupported(WhiteBalanceMode mode)

    Returns true if the white balance \a mode is supported.
*/

/*!
    Returns true if the white balance \a mode is supported.
*/
bool QCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    Q_D(const QCamera);
    return d->control && d->control->isWhiteBalanceModeSupported(mode);
}

/*!
    \qmlmethod QtMultimedia::Camera::colorTemperature

    Gets or sets the current color temperature.

    Setting a color temperature will only have an effect if WhiteBalanceManual is
    supported. In this case, setting a temperature greater 0 will automatically set the
    white balance mode to WhiteBalanceManual. Setting the temperature to 0 will reset
    the white balance mode to WhiteBalanceAuto.
*/

/*!
    \property QCamera::colorTemperature

    Returns the current color temperature if the
    current white balance mode is \c WhiteBalanceManual. For other modes the
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
    \value WhiteBalanceManual Manual white balance. In this mode the white
    balance should be set with setColorTemperature()
    \value WhiteBalanceSunlight     Sunlight white balance mode.
    \value WhiteBalanceCloudy       Cloudy white balance mode.
    \value WhiteBalanceShade        Shade white balance mode.
    \value WhiteBalanceTungsten     Tungsten (incandescent) white balance mode.
    \value WhiteBalanceFluorescent  Fluorescent white balance mode.
    \value WhiteBalanceFlash        Flash white balance mode.
    \value WhiteBalanceSunset       Sunset white balance mode.
*/

/*!
    \fn void QCamera::brightnessChanged()
    \internal
*/
/*!
    \fn void QCamera::contrastChanged()
    \internal
*/
/*!
    \fn void QCamera::hueChanged()
    \internal
*/
/*!
    \fn void QCamera::saturationChanged()
    \internal
*/
QT_END_NAMESPACE

#include "moc_qcamera.cpp"
