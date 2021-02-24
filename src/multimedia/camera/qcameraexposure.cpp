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

#include "qcameraexposure.h"
#include "private/qobject_p.h"

#include <qcamera.h>
#include <private/qplatformcameraexposure_p.h>
#include <private/qplatformcamera_p.h>

#include <QtCore/QMetaObject>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraExposure


    \brief The QCameraExposure class provides interface for exposure related camera settings.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

*/

//#define DEBUG_EXPOSURE_CHANGES 1

class QCameraExposurePrivate
{
    Q_DECLARE_PUBLIC(QCameraExposure)
public:
    void init(QPlatformCamera *cameraControl);
    QCameraExposure *q_ptr;

    template<typename T> T actualExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &defaultValue) const;
    template<typename T> T requestedExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &defaultValue) const;
    template<typename T> void setExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &value);
    void resetExposureParameter(QPlatformCameraExposure::ExposureParameter parameter);

    QCamera *camera;
    QPlatformCameraExposure *exposureControl;

    void _q_exposureParameterChanged(int parameter);
    void _q_exposureParameterRangeChanged(int parameter);
};

void QCameraExposurePrivate::init(QPlatformCamera *cameraControl)
{
    Q_Q(QCameraExposure);

    exposureControl = cameraControl->exposureControl();

    if (!exposureControl)
        return;

    q->connect(exposureControl, SIGNAL(actualValueChanged(int)),
               q, SLOT(_q_exposureParameterChanged(int)));
    q->connect(exposureControl, SIGNAL(parameterRangeChanged(int)),
               q, SLOT(_q_exposureParameterRangeChanged(int)));
    q->connect(exposureControl, SIGNAL(flashReady(bool)), q, SIGNAL(flashReady(bool)));
}

template<typename T>
T QCameraExposurePrivate::actualExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &defaultValue) const
{
    QVariant value = exposureControl ? exposureControl->actualValue(parameter) : QVariant();

    return value.isValid() ? value.value<T>() : defaultValue;
}

template<typename T>
T QCameraExposurePrivate::requestedExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &defaultValue) const
{
    QVariant value = exposureControl ? exposureControl->requestedValue(parameter) : QVariant();

    return value.isValid() ? value.value<T>() : defaultValue;
}

template<typename T>
void QCameraExposurePrivate::setExposureParameter(QPlatformCameraExposure::ExposureParameter parameter, const T &value)
{
    if (exposureControl)
        exposureControl->setValue(parameter, QVariant::fromValue<T>(value));
}

void QCameraExposurePrivate::resetExposureParameter(QPlatformCameraExposure::ExposureParameter parameter)
{
    if (exposureControl)
        exposureControl->setValue(parameter, QVariant());
}


void QCameraExposurePrivate::_q_exposureParameterChanged(int parameter)
{
    Q_Q(QCameraExposure);

#if DEBUG_EXPOSURE_CHANGES
    qDebug() << "Exposure parameter changed:"
             << QPlatformCameraExposure::ExposureParameter(parameter)
             << exposureControl->actualValue(QPlatformCameraExposure::ExposureParameter(parameter));
#endif

    switch (parameter) {
    case QPlatformCameraExposure::ISO:
        emit q->isoSensitivityChanged(q->isoSensitivity());
        break;
    case QPlatformCameraExposure::Aperture:
        emit q->apertureChanged(q->aperture());
        break;
    case QPlatformCameraExposure::ShutterSpeed:
        emit q->shutterSpeedChanged(q->shutterSpeed());
        break;
    case QPlatformCameraExposure::ExposureCompensation:
        emit q->exposureCompensationChanged(q->exposureCompensation());
        break;
    }
}

void QCameraExposurePrivate::_q_exposureParameterRangeChanged(int parameter)
{
    Q_Q(QCameraExposure);

    switch (parameter) {
    case QPlatformCameraExposure::Aperture:
        emit q->apertureRangeChanged();
        break;
    case QPlatformCameraExposure::ShutterSpeed:
        emit q->shutterSpeedRangeChanged();
        break;
    }
}

/*!
    Construct a QCameraExposure from service \a provider and \a parent.
*/

QCameraExposure::QCameraExposure(QCamera *parent, QPlatformCamera *cameraControl):
    QObject(parent), d_ptr(new QCameraExposurePrivate)
{
    Q_D(QCameraExposure);
    d->camera = parent;
    d->q_ptr = this;
    d->init(cameraControl);
}


/*!
    Destroys the camera exposure object.
*/

QCameraExposure::~QCameraExposure()
{
    Q_D(QCameraExposure);
    delete d;
}

/*!
    Returns true if exposure settings are supported by this camera.
*/
bool QCameraExposure::isAvailable() const
{
    return d_func()->exposureControl != nullptr;
}


/*!
  \property QCameraExposure::flashMode
  \brief The flash mode being used.

  Usually the single QCameraExposure::FlashMode flag is used,
  but some non conflicting flags combination are also allowed,
  like QCameraExposure::FlashManual | QCameraExposure::FlashSlowSyncRearCurtain.

  \sa QCameraExposure::isFlashModeSupported(), QCameraExposure::isFlashReady()
*/

QCameraExposure::FlashModes QCameraExposure::flashMode() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->flashMode() : QCameraExposure::FlashOff;
}

void QCameraExposure::setFlashMode(QCameraExposure::FlashModes mode)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setFlashMode(mode);
}

/*!
    Returns true if the flash \a mode is supported.
*/

bool QCameraExposure::isFlashModeSupported(QCameraExposure::FlashModes mode) const
{
    return d_func()->exposureControl ? d_func()->exposureControl->isFlashModeSupported(mode) : false;
}

/*!
    Returns true if flash is charged.
*/

bool QCameraExposure::isFlashReady() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->isFlashReady() : false;
}

/*!
  \property QCameraExposure::exposureMode
  \brief The exposure mode being used.

  \sa QCameraExposure::isExposureModeSupported()
*/

QCameraExposure::ExposureMode QCameraExposure::exposureMode() const
{
    return d_func()->actualExposureParameter<QCameraExposure::ExposureMode>(QPlatformCameraExposure::ExposureMode, QCameraExposure::ExposureAuto);
}

void QCameraExposure::setExposureMode(QCameraExposure::ExposureMode mode)
{
    d_func()->setExposureParameter<QCameraExposure::ExposureMode>(QPlatformCameraExposure::ExposureMode, mode);
}

/*!
    Returns true if the exposure \a mode is supported.
*/

bool QCameraExposure::isExposureModeSupported(QCameraExposure::ExposureMode mode) const
{
    if (!d_func()->exposureControl)
        return false;

    bool continuous = false;
    return d_func()->exposureControl->supportedParameterRange(QPlatformCameraExposure::ExposureMode, &continuous)
            .contains(QVariant::fromValue<QCameraExposure::ExposureMode>(mode));
}

/*!
  \property QCameraExposure::exposureCompensation
  \brief Exposure compensation in EV units.

  Exposure compensation property allows to adjust the automatically calculated exposure.
*/

qreal QCameraExposure::exposureCompensation() const
{
    return d_func()->actualExposureParameter<qreal>(QPlatformCameraExposure::ExposureCompensation, 0.0);
}

void QCameraExposure::setExposureCompensation(qreal ev)
{
    d_func()->setExposureParameter<qreal>(QPlatformCameraExposure::ExposureCompensation, ev);
}

int QCameraExposure::isoSensitivity() const
{
    return d_func()->actualExposureParameter<int>(QPlatformCameraExposure::ISO, -1);
}

/*!
    Returns the requested ISO sensitivity
    or -1 if automatic ISO is turned on.
*/
int QCameraExposure::requestedIsoSensitivity() const
{
    return d_func()->requestedExposureParameter<int>(QPlatformCameraExposure::ISO, -1);
}

/*!
    Returns the list of ISO senitivities camera supports.

    If the camera supports arbitrary ISO sensitivities within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<int> QCameraExposure::supportedIsoSensitivities(bool *continuous) const
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
    \fn QCameraExposure::setManualIsoSensitivity(int iso)
    Sets the manual sensitivity to \a iso
*/

void QCameraExposure::setManualIsoSensitivity(int iso)
{
    d_func()->setExposureParameter<int>(QPlatformCameraExposure::ISO, iso);
}

/*!
     \fn QCameraExposure::setAutoIsoSensitivity()
     Turn on auto sensitivity
*/

void QCameraExposure::setAutoIsoSensitivity()
{
    d_func()->resetExposureParameter(QPlatformCameraExposure::ISO);
}

/*!
    \property QCameraExposure::shutterSpeed
    \brief Camera's shutter speed in seconds.

    \sa supportedShutterSpeeds(), setAutoShutterSpeed(), setManualShutterSpeed()
*/

/*!
    \fn QCameraExposure::shutterSpeedChanged(qreal speed)

    Signals that a camera's shutter \a speed has changed.
*/

/*!
    \property QCameraExposure::isoSensitivity
    \brief The sensor ISO sensitivity.

    \sa supportedIsoSensitivities(), setAutoIsoSensitivity(), setManualIsoSensitivity()
*/

/*!
    \property QCameraExposure::aperture
    \brief Lens aperture is specified as an F number, the ratio of the focal length to effective aperture diameter.

    \sa supportedApertures(), setAutoAperture(), setManualAperture(), requestedAperture()
*/


qreal QCameraExposure::aperture() const
{
    return d_func()->actualExposureParameter<qreal>(QPlatformCameraExposure::Aperture, -1.0);
}

/*!
    Returns the requested manual aperture
    or -1.0 if automatic aperture is turned on.
*/
qreal QCameraExposure::requestedAperture() const
{
    return d_func()->requestedExposureParameter<qreal>(QPlatformCameraExposure::Aperture, -1.0);
}


/*!
    Returns the list of aperture values camera supports.
    The apertures list can change depending on the focal length,
    in such a case the apertureRangeChanged() signal is emitted.

    If the camera supports arbitrary aperture values within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<qreal> QCameraExposure::supportedApertures(bool * continuous) const
{
    QList<qreal> res;
    QPlatformCameraExposure *control = d_func()->exposureControl;

    bool tmp = false;
    if (!continuous)
        continuous = &tmp;

    if (!control)
        return res;

    const auto range = control->supportedParameterRange(QPlatformCameraExposure::Aperture, continuous);
    for (const QVariant &value : range) {
        bool ok = false;
        qreal realValue = value.toReal(&ok);
        if (ok)
            res.append(realValue);
        else
            qWarning() << "Incompatible aperture value type, qreal is expected";
    }

    return res;
}

/*!
    \fn QCameraExposure::setManualAperture(qreal aperture)
    Sets the manual camera \a aperture value.
*/

void QCameraExposure::setManualAperture(qreal aperture)
{
    d_func()->setExposureParameter<qreal>(QPlatformCameraExposure::Aperture, aperture);
}

/*!
    \fn QCameraExposure::setAutoAperture()
    Turn on auto aperture
*/

void QCameraExposure::setAutoAperture()
{
    d_func()->resetExposureParameter(QPlatformCameraExposure::Aperture);
}

/*!
    Returns the current shutter speed in seconds.
*/

qreal QCameraExposure::shutterSpeed() const
{
    return d_func()->actualExposureParameter<qreal>(QPlatformCameraExposure::ShutterSpeed, -1.0);
}

/*!
    Returns the requested manual shutter speed in seconds
    or -1.0 if automatic shutter speed is turned on.
*/
qreal QCameraExposure::requestedShutterSpeed() const
{
    return d_func()->requestedExposureParameter<qreal>(QPlatformCameraExposure::ShutterSpeed, -1.0);
}

/*!
    Returns the list of shutter speed values in seconds camera supports.

    If the camera supports arbitrary shutter speed values within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<qreal> QCameraExposure::supportedShutterSpeeds(bool *continuous) const
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

void QCameraExposure::setManualShutterSpeed(qreal seconds)
{
    d_func()->setExposureParameter<qreal>(QPlatformCameraExposure::ShutterSpeed, seconds);
}

/*!
    Turn on auto shutter speed
*/

void QCameraExposure::setAutoShutterSpeed()
{
    d_func()->resetExposureParameter(QPlatformCameraExposure::ShutterSpeed);
}


/*!
    \enum QCameraExposure::FlashMode

    \value FlashAuto            Automatic flash.
    \value FlashOff             Flash is Off.
    \value FlashOn              Flash is On.
    \value FlashRedEyeReduction Red eye reduction flash.
    \value FlashFill            Use flash to fillin shadows.
    \value FlashTorch           Constant light source. If supported,
                                torch can be enabled without loading the camera.
    \value FlashVideoLight      Constant light source, useful for video capture.
                                The light is turned on only while camera is active.
    \value FlashSlowSyncFrontCurtain
                                Use the flash in conjunction with a slow shutter speed.
                                This mode allows better exposure of distant objects and/or motion blur effect.
    \value FlashSlowSyncRearCurtain
                                The similar mode to FlashSlowSyncFrontCurtain but flash is fired at the end of exposure.
    \value FlashManual          Flash power is manualy set.
*/

/*!
    \enum QCameraExposure::ExposureMode

    \value ExposureAuto          Automatic mode.
    \value ExposureManual        Manual mode.
    \value ExposurePortrait      Portrait exposure mode.
    \value ExposureNight         Night mode.
    \value ExposureBacklight     Backlight exposure mode.
    \value ExposureSpotlight     Spotlight exposure mode.
    \value ExposureSports        Spots exposure mode.
    \value ExposureSnow          Snow exposure mode.
    \value ExposureBeach         Beach exposure mode.
    \value ExposureLargeAperture Use larger aperture with small depth of field.
    \value ExposureSmallAperture Use smaller aperture.
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
    \value ExposureModeVendor    The base value for device specific exposure modes.
*/

/*!
    \property QCameraExposure::flashReady
    \brief Indicates if the flash is charged and ready to use.
*/

/*!
    \fn void QCameraExposure::flashReady(bool ready)

    Signal the flash \a ready status has changed.
*/

/*!
    \fn void QCameraExposure::apertureChanged(qreal value)

    Signal emitted when aperature changes to \a value.
*/

/*!
    \fn void QCameraExposure::apertureRangeChanged()

    Signal emitted when aperature range has changed.
*/


/*!
    \fn void QCameraExposure::shutterSpeedRangeChanged()

    Signal emitted when the shutter speed range has changed.
*/


/*!
    \fn void QCameraExposure::isoSensitivityChanged(int value)

    Signal emitted when sensitivity changes to \a value.
*/

/*!
    \fn void QCameraExposure::exposureCompensationChanged(qreal value)

    Signal emitted when the exposure compensation changes to \a value.
*/

QT_END_NAMESPACE

#include "moc_qcameraexposure.cpp"
