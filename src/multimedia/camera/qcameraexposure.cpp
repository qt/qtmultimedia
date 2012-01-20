/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qcameraexposure.h"
#include "qmediaobject_p.h"

#include <qcamera.h>
#include <qcameraexposurecontrol.h>
#include <qcameraflashcontrol.h>

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

#ifdef DEBUG_EXPOSURE_CHANGES
#define ENUM_NAME(c,e,v) (c::staticMetaObject.enumerator(c::staticMetaObject.indexOfEnumerator(e)).valueToKey((v)))
#endif

namespace
{
class CameraExposureRegisterMetaTypes
{
public:
    CameraExposureRegisterMetaTypes()
    {
        qRegisterMetaType<QCameraExposure::ExposureMode>("QCameraExposure::ExposureMode");
        qRegisterMetaType<QCameraExposure::FlashModes>("QCameraExposure::FlashModes");
        qRegisterMetaType<QCameraExposure::MeteringMode>("QCameraExposure::MeteringMode");
    }
} _registerCameraExposureMetaTypes;
}



class QCameraExposurePrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QCameraExposure)
public:
    void initControls();
    QCameraExposure *q_ptr;

    QCamera *camera;
    QCameraExposureControl *exposureControl;
    QCameraFlashControl *flashControl;

    void _q_exposureParameterChanged(int parameter);
    void _q_exposureParameterRangeChanged(int parameter);
};

void QCameraExposurePrivate::initControls()
{
    Q_Q(QCameraExposure);

    QMediaService *service = camera->service();
    exposureControl = 0;
    flashControl = 0;
    if (service) {
        exposureControl = qobject_cast<QCameraExposureControl *>(service->requestControl(QCameraExposureControl_iid));
        flashControl = qobject_cast<QCameraFlashControl *>(service->requestControl(QCameraFlashControl_iid));
    }
    if (exposureControl) {
        q->connect(exposureControl, SIGNAL(exposureParameterChanged(int)),
                   q, SLOT(_q_exposureParameterChanged(int)));
        q->connect(exposureControl, SIGNAL(exposureParameterRangeChanged(int)),
                   q, SLOT(_q_exposureParameterRangeChanged(int)));
    }

    if (flashControl)
        q->connect(flashControl, SIGNAL(flashReady(bool)), q, SIGNAL(flashReady(bool)));
}

void QCameraExposurePrivate::_q_exposureParameterChanged(int parameter)
{
    Q_Q(QCameraExposure);

#if DEBUG_EXPOSURE_CHANGES
    qDebug() << "Exposure parameter changed:"
             << ENUM_NAME(QCameraExposureControl, "ExposureParameter", parameter)
             << exposureControl->exposureParameter(QCameraExposureControl::ExposureParameter(parameter));
#endif

    switch (parameter) {
    case QCameraExposureControl::ISO:
        emit q->isoSensitivityChanged(q->isoSensitivity());
        break;
    case QCameraExposureControl::Aperture:
        emit q->apertureChanged(q->aperture());
        break;
    case QCameraExposureControl::ShutterSpeed:
        emit q->shutterSpeedChanged(q->shutterSpeed());
        break;
    case QCameraExposureControl::ExposureCompensation:
        emit q->exposureCompensationChanged(q->exposureCompensation());
        break;
    }
}

void QCameraExposurePrivate::_q_exposureParameterRangeChanged(int parameter)
{
    Q_Q(QCameraExposure);

    switch (parameter) {
    case QCameraExposureControl::Aperture:
        emit q->apertureRangeChanged();
        break;
    case QCameraExposureControl::ShutterSpeed:
        emit q->shutterSpeedRangeChanged();
        break;
    }
}

/*!
    Construct a QCameraExposure from service \a provider and \a parent.
*/

QCameraExposure::QCameraExposure(QCamera *parent):
    QObject(parent), d_ptr(new QCameraExposurePrivate)
{
    Q_D(QCameraExposure);
    d->camera = parent;
    d->q_ptr = this;
    d->initControls();
}


/*!
    Destroys the camera exposure object.
*/

QCameraExposure::~QCameraExposure()
{
    Q_D(QCameraExposure);
    if (d->exposureControl)
        d->camera->service()->releaseControl(d->exposureControl);
}

/*!
    Returns true if exposure settings are supported by this camera.
*/
bool QCameraExposure::isAvailable() const
{
    return d_func()->exposureControl != 0;
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
    return d_func()->flashControl ? d_func()->flashControl->flashMode() : QCameraExposure::FlashOff;
}

void QCameraExposure::setFlashMode(QCameraExposure::FlashModes mode)
{
    if (d_func()->flashControl)
        d_func()->flashControl->setFlashMode(mode);
}

/*!
    Returns true if the flash \a mode is supported.
*/

bool QCameraExposure::isFlashModeSupported(QCameraExposure::FlashModes mode) const
{
    return d_func()->flashControl ? d_func()->flashControl->isFlashModeSupported(mode) : false;
}

/*!
    Returns true if flash is charged.
*/

bool QCameraExposure::isFlashReady() const
{
    return d_func()->flashControl ? d_func()->flashControl->isFlashReady() : false;
}


/*!
  \property QCameraExposure::exposureMode
  \brief The exposure mode being used.

  \sa QCameraExposure::isExposureModeSupported()
*/

QCameraExposure::ExposureMode QCameraExposure::exposureMode() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->exposureMode() : QCameraExposure::ExposureAuto;
}

void QCameraExposure::setExposureMode(QCameraExposure::ExposureMode mode)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureMode(mode);
}

/*!
    Returns true if the exposure \a mode is supported.
*/

bool QCameraExposure::isExposureModeSupported(QCameraExposure::ExposureMode mode) const
{
    return d_func()->exposureControl ?
            d_func()->exposureControl->isExposureModeSupported(mode) : false;
}

/*!
  \property QCameraExposure::exposureCompensation
  \brief Exposure compensation in EV units.

  Exposure compensation property allows to adjust the automatically calculated exposure.
*/

qreal QCameraExposure::exposureCompensation() const
{
    if (d_func()->exposureControl)
        return d_func()->exposureControl->exposureParameter(QCameraExposureControl::ExposureCompensation).toReal();
    else
        return 0;
}

void QCameraExposure::setExposureCompensation(qreal ev)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::ExposureCompensation, QVariant(ev));
}

/*!
  \property QCameraExposure::meteringMode
  \brief The metering mode being used.

  \sa QCameraExposure::isMeteringModeSupported()
*/

QCameraExposure::MeteringMode QCameraExposure::meteringMode() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->meteringMode() : QCameraExposure::MeteringMatrix;
}

void QCameraExposure::setMeteringMode(QCameraExposure::MeteringMode mode)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setMeteringMode(mode);
}

/*!
  \property QCameraExposure::spotMeteringPoint

  When supported, this property is the (normalized) position of the point of the image
  where exposure metering will be performed.  This is typically used to indicate an
  "interesting" area of the image that should be exposed properly.

  The coordinates are relative frame coordinates:
  QPointF(0,0) points to the left top frame point, QPointF(0.5,0.5) points to the frame center,
  which is typically the default spot metering point.

  The spot metering point is only used with spot metering mode.
 */

QPointF QCameraExposure::spotMeteringPoint() const
{
    return d_func()->exposureControl ? d_func()->exposureControl->exposureParameter(QCameraExposureControl::SpotMeteringPoint).toPointF() : QPointF();
}

void QCameraExposure::setSpotMeteringPoint(const QPointF &point)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::SpotMeteringPoint, point);
}


/*!
    Returns true if the metering \a mode is supported.
*/
bool QCameraExposure::isMeteringModeSupported(QCameraExposure::MeteringMode mode) const
{
    return d_func()->exposureControl ? d_func()->exposureControl->isMeteringModeSupported(mode) : false;
}

int QCameraExposure::isoSensitivity() const
{
    if (d_func()->exposureControl)
        return d_func()->exposureControl->exposureParameter(QCameraExposureControl::ISO).toInt();

    return -1;
}

/*!
    Returns the list of ISO senitivities camera supports.

    If the camera supports arbitrary ISO sensitivities within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<int> QCameraExposure::supportedIsoSensitivities(bool *continuous) const
{
    QList<int> res;
    QCameraExposureControl *control = d_func()->exposureControl;

    if (!control)
        return res;

    foreach (const QVariant &value,
             control->supportedParameterRange(QCameraExposureControl::ISO)) {
        bool ok = false;
        int intValue = value.toInt(&ok);
        if (ok)
            res.append(intValue);
        else
            qWarning() << "Incompatible ISO value type, int is expected";
    }

    if (continuous)
        *continuous = control->exposureParameterFlags(QCameraExposureControl::ISO) &
                      QCameraExposureControl::ContinuousRange;

    return res;
}

/*!
    \fn QCameraExposure::setManualIsoSensitivity(int iso)
    Sets the manual sensitivity to \a iso
*/

void QCameraExposure::setManualIsoSensitivity(int iso)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::ISO, QVariant(iso));
}

/*!
     \fn QCameraExposure::setAutoIsoSensitivity()
     Turn on auto sensitivity
*/

void QCameraExposure::setAutoIsoSensitivity()
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::ISO, QVariant());
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

    \sa supportedApertures(), setAutoAperture(), setManualAperture()
*/


qreal QCameraExposure::aperture() const
{
    if (d_func()->exposureControl)
        return d_func()->exposureControl->exposureParameter(QCameraExposureControl::Aperture).toReal();

    return -1.0;
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
    QCameraExposureControl *control = d_func()->exposureControl;

    if (!control)
        return res;

    foreach (const QVariant &value,
             control->supportedParameterRange(QCameraExposureControl::Aperture)) {
        bool ok = false;
        qreal realValue = value.toReal(&ok);
        if (ok)
            res.append(realValue);
        else
            qWarning() << "Incompatible aperture value type, qreal is expected";
    }

    if (continuous)
        *continuous = control->exposureParameterFlags(QCameraExposureControl::Aperture) &
                      QCameraExposureControl::ContinuousRange;

    return res;
}

/*!
    \fn QCameraExposure::setManualAperture(qreal aperture)
    Sets the manual camera \a aperture value.
*/

void QCameraExposure::setManualAperture(qreal aperture)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::Aperture, QVariant(aperture));
}

/*!
    \fn QCameraExposure::setAutoAperture()
    Turn on auto aperture
*/

void QCameraExposure::setAutoAperture()
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::Aperture, QVariant());
}

/*!
    Returns the current shutter speed in seconds.
*/

qreal QCameraExposure::shutterSpeed() const
{
    if (d_func()->exposureControl)
        return d_func()->exposureControl->exposureParameter(QCameraExposureControl::ShutterSpeed).toReal();

    return -1.0;
}

/*!
    Returns the list of shutter speed values in seconds camera supports.

    If the camera supports arbitrary shutter speed values within the supported range,
    *\a continuous is set to true, otherwise *\a continuous is set to false.
*/
QList<qreal> QCameraExposure::supportedShutterSpeeds(bool *continuous) const
{
    QList<qreal> res;

    QCameraExposureControl *control = d_func()->exposureControl;
    if (!control)
        return res;

    foreach (const QVariant &value,
             control->supportedParameterRange(QCameraExposureControl::ShutterSpeed)) {
        bool ok = false;
        qreal realValue = value.toReal(&ok);
        if (ok)
            res.append(realValue);
        else
            qWarning() << "Incompatible shutter speed value type, qreal is expected";
    }

    if (continuous)
        *continuous = control->exposureParameterFlags(QCameraExposureControl::ShutterSpeed) &
                      QCameraExposureControl::ContinuousRange;

    return res;
}

/*!
    Set the manual shutter speed to \a seconds
*/

void QCameraExposure::setManualShutterSpeed(qreal seconds)
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::ShutterSpeed, QVariant(seconds));
}

/*!
    Turn on auto shutter speed
*/

void QCameraExposure::setAutoShutterSpeed()
{
    if (d_func()->exposureControl)
        d_func()->exposureControl->setExposureParameter(QCameraExposureControl::ShutterSpeed, QVariant());
}


/*!
    \enum QCameraExposure::FlashMode

    \value FlashOff             Flash is Off.
    \value FlashOn              Flash is On.
    \value FlashAuto            Automatic flash.
    \value FlashRedEyeReduction Red eye reduction flash.
    \value FlashFill            Use flash to fillin shadows.
    \value FlashTorch           Constant light source, useful for focusing and video capture.
    \value FlashSlowSyncFrontCurtain
                                Use the flash in conjunction with a slow shutter speed.
                                This mode allows better exposure of distant objects and/or motion blur effect.
    \value FlashSlowSyncRearCurtain
                                The similar mode to FlashSlowSyncFrontCurtain but flash is fired at the end of exposure.
    \value FlashManual          Flash power is manualy set.
*/

/*!
    \enum QCameraExposure::ExposureMode

    \value ExposureManual        Manual mode.
    \value ExposureAuto          Automatic mode.
    \value ExposureNight         Night mode.
    \value ExposureBacklight     Backlight exposure mode.
    \value ExposureSpotlight     Spotlight exposure mode.
    \value ExposureSports        Spots exposure mode.
    \value ExposureSnow          Snow exposure mode.
    \value ExposureBeach         Beach exposure mode.
    \value ExposureLargeAperture Use larger aperture with small depth of field.
    \value ExposureSmallAperture Use smaller aperture.
    \value ExposurePortrait      Portrait exposure mode.
    \value ExposureModeVendor    The base value for device specific exposure modes.
*/

/*!
    \enum QCameraExposure::MeteringMode

    \value MeteringAverage       Center weighted average metering mode.
    \value MeteringSpot          Spot metering mode.
    \value MeteringMatrix        Matrix metering mode.
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

#include "moc_qcameraexposure.cpp"
QT_END_NAMESPACE
