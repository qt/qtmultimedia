/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qdeclarativecamera_p.h"
#include "qdeclarativecameraexposure_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass CameraExposure QDeclarativeCameraExposure
    \brief The CameraExposure element provides interface for exposure related camera settings.
    \ingroup multimedia_qml


    This element is part of the \bold{QtMultimedia 5.0} module.

    It should not be constructed separately but provided by Camera.exposure.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Camera {
        id: camera

        exposure.exposureCompensation: -1.0
        exposure.exposureMode: Camera.ExposurePortrait
    }

    \endqml
*/

/*!
    \internal
    \class QDeclarativeCameraExposure
    \brief The CameraExposure element provides interface for exposure related camera settings.

*/

/*!
    Construct a declarative camera exposure object using \a parent object.
 */
QDeclarativeCameraExposure::QDeclarativeCameraExposure(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_exposure = camera->exposure();

    connect(m_exposure, SIGNAL(isoSensitivityChanged(int)), this, SIGNAL(isoSensitivityChanged(int)));
    connect(m_exposure, SIGNAL(apertureChanged(qreal)), this, SIGNAL(apertureChanged(qreal)));
    connect(m_exposure, SIGNAL(shutterSpeedChanged(qreal)), this, SIGNAL(shutterSpeedChanged(qreal)));

    connect(m_exposure, SIGNAL(exposureCompensationChanged(qreal)), this, SIGNAL(exposureCompensationChanged(qreal)));
}

QDeclarativeCameraExposure::~QDeclarativeCameraExposure()
{
}

/*!
    \qmlproperty real CameraExposure::exposureCompensation
    \property QDeclarativeCameraExposure::exposureCompensation

    Adjustment for the automatically calculated exposure.  The value is
    in EV units.
 */
qreal QDeclarativeCameraExposure::exposureCompensation() const
{
    return m_exposure->exposureCompensation();
}

void QDeclarativeCameraExposure::setExposureCompensation(qreal ev)
{
    m_exposure->setExposureCompensation(ev);
}

/*!
    \qmlproperty real CameraExposure::isoSensitivity
    \property QDeclarativeCameraExposure::iso

    The sensor's ISO sensitivity.
 */
int QDeclarativeCameraExposure::isoSensitivity() const
{
    return m_exposure->isoSensitivity();
}

/*!
    \qmlproperty real CameraExposure::shutterSpeed
    \property QDeclarativeCameraExposure::shutterSpeed

    The camera's shutter speed, in seconds.
*/
qreal QDeclarativeCameraExposure::shutterSpeed() const
{
    return m_exposure->shutterSpeed();
}

/*!
    \qmlproperty real CameraExposure::aperture
    \property QDeclarativeCameraExposure::aperture

    The lens aperture as an F number (the ratio of the focal length to effective aperture diameter).
*/
qreal QDeclarativeCameraExposure::aperture() const
{
    return m_exposure->aperture();
}

int QDeclarativeCameraExposure::manualIsoSensitivity() const
{
    return m_manualIso;
}

void QDeclarativeCameraExposure::setManualIsoSensitivity(int iso)
{
    m_manualIso = iso;
    if (iso > 0)
        m_exposure->setManualIsoSensitivity(iso);
    else
        m_exposure->setAutoIsoSensitivity();

    emit manualIsoSensitivityChanged(iso);
}

qreal QDeclarativeCameraExposure::manualShutterSpeed() const
{
    return m_manualShutterSpeed;
}

void QDeclarativeCameraExposure::setManualShutterSpeed(qreal speed)
{
    m_manualShutterSpeed = speed;
    if (speed > 0)
        m_exposure->setManualShutterSpeed(speed);
    else
        m_exposure->setAutoShutterSpeed();

    emit manualShutterSpeedChanged(speed);
}

qreal QDeclarativeCameraExposure::manualAperture() const
{
    return m_manualAperture;
}

void QDeclarativeCameraExposure::setManualAperture(qreal aperture)
{
    m_manualAperture = aperture;
    if (aperture > 0)
        m_exposure->setManualAperture(aperture);
    else
        m_exposure->setAutoAperture();

    emit manualApertureChanged(aperture);
}

/*!
  Turn on auto aperture. The manual aperture value is reset to -1.0
 */
void QDeclarativeCameraExposure::setAutoAperture()
{
    setManualAperture(-1.0);
}

/*!
  Turn on auto shutter speed. The manual shutter speed value is reset to -1.0
 */
void QDeclarativeCameraExposure::setAutoShutterSpeed()
{
    setManualShutterSpeed(-1.0);
}

/*!
  Turn on auto ISO sensitivity. The manual ISO value is reset to -1.
 */
void QDeclarativeCameraExposure::setAutoIsoSensitivity()
{
    setManualIsoSensitivity(-1);
}

/*!
    \qmlproperty enumeration CameraExposure::exposureMode
    \property QDeclarativeCameraExposure::exposureMode

    \table
    \header \o Value \o Description
    \row \o Camera.ExposureManual        \o Manual mode.
    \row \o Camera.ExposureAuto          \o Automatic mode.
    \row \o Camera.ExposureNight         \o Night mode.
    \row \o Camera.ExposureBacklight     \o Backlight exposure mode.
    \row \o Camera.ExposureSpotlight     \o Spotlight exposure mode.
    \row \o Camera.ExposureSports        \o Spots exposure mode.
    \row \o Camera.ExposureSnow          \o Snow exposure mode.
    \row \o Camera.ExposureBeach         \o Beach exposure mode.
    \row \o Camera.ExposureLargeAperture \o Use larger aperture with small depth of field.
    \row \o Camera.ExposureSmallAperture \o Use smaller aperture.
    \row \o Camera.ExposurePortrait      \o Portrait exposure mode.
    \row \o Camera.ExposureModeVendor    \o The base value for device specific exposure modes.
    \endtable
*/

QDeclarativeCamera::ExposureMode QDeclarativeCameraExposure::exposureMode() const
{
    return QDeclarativeCamera::ExposureMode(m_exposure->exposureMode());
}

void QDeclarativeCameraExposure::setExposureMode(QDeclarativeCamera::ExposureMode mode)
{
    if (exposureMode() != mode) {
        m_exposure->setExposureMode(QCameraExposure::ExposureMode(mode));
        emit exposureModeChanged(exposureMode());
    }
}

/*!
    \qmlsignal CameraExposure::exposureModeChanged(CameraExposure::ExposureMode)
    \fn void QDeclarativeCameraExposure::exposureModeChanged(QDeclarativeCamera::ExposureMode)
*/

/*!
    \qmlproperty QPointF CameraExposure::spotMeteringPoint
    \property QDeclarativeCameraExposure::spotMeteringPoint

    The relative frame coordinates of the point to use for exposure metering (in relative
    frame coordinates).  This point is only used in spot metering mode, and typically defaults
    to the center \c (0.5, 0.5).
 */

QPointF QDeclarativeCameraExposure::spotMeteringPoint() const
{
    return m_exposure->spotMeteringPoint();
}

void QDeclarativeCameraExposure::setSpotMeteringPoint(const QPointF &point)
{
    QPointF oldPoint(spotMeteringPoint());
    m_exposure->setSpotMeteringPoint(point);

    if (oldPoint != spotMeteringPoint())
        emit spotMeteringPointChanged(spotMeteringPoint());
}

QDeclarativeCamera::MeteringMode QDeclarativeCameraExposure::meteringMode() const
{
    return QDeclarativeCamera::MeteringMode(m_exposure->meteringMode());
}

void QDeclarativeCameraExposure::setMeteringMode(QDeclarativeCamera::MeteringMode mode)
{
    QDeclarativeCamera::MeteringMode oldMode = meteringMode();
    m_exposure->setMeteringMode(QCameraExposure::MeteringMode(mode));
    if (oldMode != meteringMode())
        emit meteringModeChanged(meteringMode());
}

QT_END_NAMESPACE

#include "moc_qdeclarativecameraexposure_p.cpp"
