/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>

#include "simulatorcameraexposurecontrol.h"
#include "simulatorcamerasession.h"

SimulatorCameraExposureControl::SimulatorCameraExposureControl(SimulatorCameraSession *session, QObject *parent) :
    QCameraExposureControl(parent),
    mExposureMode(QCameraExposure::ExposureAuto),
    mMeteringMode(QCameraExposure::MeteringAverage),
    mSession(session),
    mSettings(0)
{
    mSettings = mSession->settings();

    connect(mSettings, SIGNAL(apertureChanged()), this, SLOT(apertureChanged()));
    connect(mSettings, SIGNAL(apertureRangeChanged()), this, SLOT(apertureRangeChanged()));
    connect(mSettings, SIGNAL(shutterSpeedChanged()), this, SLOT(shutterSpeedChanged()));
    connect(mSettings, SIGNAL(isoSensitivityChanged()), this, SLOT(isoSensitivityChanged()));
}

SimulatorCameraExposureControl::~SimulatorCameraExposureControl()
{
}

void SimulatorCameraExposureControl::apertureChanged()
{
    emit exposureParameterChanged(QCameraExposureControl::Aperture);
}

void SimulatorCameraExposureControl::apertureRangeChanged()
{
    emit exposureParameterRangeChanged(QCameraExposureControl::Aperture);
}

void SimulatorCameraExposureControl::shutterSpeedChanged()
{
    emit exposureParameterChanged(QCameraExposureControl::ShutterSpeed);
}

void SimulatorCameraExposureControl::isoSensitivityChanged()
{
    emit exposureParameterChanged(QCameraExposureControl::ISO);
}

QCameraExposure::ExposureMode SimulatorCameraExposureControl::exposureMode() const
{
    return mExposureMode;
}

void SimulatorCameraExposureControl::setExposureMode(QCameraExposure::ExposureMode mode)
{
    if (isExposureModeSupported(mode))
        mExposureMode = mode;
}

bool SimulatorCameraExposureControl::isExposureModeSupported(QCameraExposure::ExposureMode mode) const
{
    switch (mode) {
    case QCameraExposure::ExposureAuto:
    case QCameraExposure::ExposureManual:
        return true;
    default:
        return false;
    }

    return false;
}

QCameraExposure::MeteringMode SimulatorCameraExposureControl::meteringMode() const
{
    return mMeteringMode;
}

void SimulatorCameraExposureControl::setMeteringMode(QCameraExposure::MeteringMode mode)
{
    if (isMeteringModeSupported(mode))
            mMeteringMode = mode;
}

bool SimulatorCameraExposureControl::isMeteringModeSupported(QCameraExposure::MeteringMode mode) const
{
    switch (mode) {
    case QCameraExposure::MeteringAverage:
    case QCameraExposure::MeteringSpot:
    case QCameraExposure::MeteringMatrix:
        return true;
    default:
        return false;
    }
    return false;
}

bool SimulatorCameraExposureControl::isParameterSupported(ExposureParameter parameter) const
{
    switch (parameter) {
    case QCameraExposureControl::ISO:
    case QCameraExposureControl::Aperture:
    case QCameraExposureControl::ShutterSpeed:
    case QCameraExposureControl::ExposureCompensation:
        return true;
    case QCameraExposureControl::FlashPower:
    case QCameraExposureControl::FlashCompensation:
        return false;

    default:
        return false;
    }

    return false;
}

QVariant SimulatorCameraExposureControl::exposureParameter(ExposureParameter parameter) const
{
    switch (parameter) {
        case QCameraExposureControl::ISO:
            return QVariant(isoSensitivity());
        case QCameraExposureControl::Aperture:
            return QVariant(aperture());
        case QCameraExposureControl::ShutterSpeed:
            return QVariant(shutterSpeed());
        case QCameraExposureControl::ExposureCompensation:
            return QVariant(exposureCompensation());
        case QCameraExposureControl::FlashPower:
        case QCameraExposureControl::FlashCompensation:
            // Not supported
            return QVariant();

        default:
            // Not supported
            return QVariant();
    }
}

QCameraExposureControl::ParameterFlags SimulatorCameraExposureControl::exposureParameterFlags(ExposureParameter parameter) const
{
    QCameraExposureControl::ParameterFlags flags;

    /*
     * ISO, Aperture, ShutterSpeed:
     *  - Automatic/Manual
     *  - Read/Write
     *  - Discrete range
     *
     * ExposureCompensation:
     *  - Automatic/Manual
     *  - Read/Write
     *  - Continuous range
     *
     * FlashPower, FlashCompensation:
     *  - Not supported
     */
    switch (parameter) {
        case QCameraExposureControl::ISO:
        case QCameraExposureControl::Aperture:
        case QCameraExposureControl::ShutterSpeed:
            flags |= QCameraExposureControl::AutomaticValue;
            break;
        case QCameraExposureControl::ExposureCompensation:
            flags |= QCameraExposureControl::AutomaticValue;
            flags |= QCameraExposureControl::ContinuousRange;
            break;
        case QCameraExposureControl::FlashPower:
        case QCameraExposureControl::FlashCompensation:
            // Do nothing - no flags
            break;

        default:
            // Do nothing - no flags
            break;
    }

    return flags;
}

QVariantList SimulatorCameraExposureControl::supportedParameterRange(ExposureParameter parameter) const
{
    QVariantList valueList;
    switch (parameter) {
    case QCameraExposureControl::ISO: {
        QList<int> exposureValues = mSettings->supportedIsoSensitivities();
        for (int i = 0; i < exposureValues.count(); ++i) {
            valueList.append(QVariant(exposureValues[i]));
        }
        break;
    }
    case QCameraExposureControl::Aperture: {
        QList<qreal> apertureValues = mSettings->supportedApertures();
        for (int i = 0; i < apertureValues.count(); ++i) {
            valueList.append(QVariant(apertureValues[i]));
        }
        break;
    }
    case QCameraExposureControl::ShutterSpeed: {
        QList<qreal> shutterSpeedValues = mSettings->supportedShutterSpeeds();
        for (int i = 0; i < shutterSpeedValues.count(); ++i) {
            valueList.append(QVariant(shutterSpeedValues[i]));
        }
        break;
    }
    case QCameraExposureControl::ExposureCompensation: {
        QList<qreal> evValues = mSettings->supportedExposureCompensationValues();
        for (int i = 0; i < evValues.count(); ++i) {
            valueList.append(QVariant(evValues[i]));
        }
        break;
    }
    case QCameraExposureControl::FlashPower:
    case QCameraExposureControl::FlashCompensation:
        // Not supported
        break;

    default:
        // Not supported
        return QVariantList();
    }

    return valueList;
}

bool SimulatorCameraExposureControl::setExposureParameter(ExposureParameter parameter, const QVariant& value)
{
    bool useDefaultValue = false;

    if (value.isNull())
        useDefaultValue = true;

    switch (parameter) {
        case QCameraExposureControl::ISO:
            if (useDefaultValue) {
                setAutoIsoSensitivity();
                return false;
            }
            else
                return setManualIsoSensitivity(value.toInt());

        case QCameraExposureControl::Aperture:
            if (useDefaultValue) {
                setAutoAperture();
                return false;
            }
            else
                return setManualAperture(value.toReal());

        case QCameraExposureControl::ShutterSpeed:
            if (useDefaultValue) {
                setAutoShutterSpeed();
                return false;
            }
            else
                return setManualShutterSpeed(value.toReal());

        case QCameraExposureControl::ExposureCompensation:
            if (useDefaultValue) {
                setAutoExposureCompensation();
                return false;
            }
            else
                return setManualExposureCompensation(value.toReal());

        case QCameraExposureControl::FlashPower:
            return false;
        case QCameraExposureControl::FlashCompensation:
            return false;

        default:
            // Not supported
            return false;
    }
}

QString SimulatorCameraExposureControl::extendedParameterName(ExposureParameter parameter)
{
    switch (parameter) {
        case QCameraExposureControl::ISO:
            return QString("ISO Sensitivity");
        case QCameraExposureControl::Aperture:
            return QString("Aperture");
        case QCameraExposureControl::ShutterSpeed:
            return QString("Shutter Speed");
        case QCameraExposureControl::ExposureCompensation:
            return QString("Exposure Compensation");
        case QCameraExposureControl::FlashPower:
            return QString("Flash Power");
        case QCameraExposureControl::FlashCompensation:
            return QString("Flash Compensation");

        default:
            return QString();
    }
}

int SimulatorCameraExposureControl::isoSensitivity() const
{
    return mSettings->isoSensitivity();
}

bool SimulatorCameraExposureControl::isIsoSensitivitySupported(const int iso) const
{
    return mSettings->supportedIsoSensitivities().contains(iso);
}

bool SimulatorCameraExposureControl::setManualIsoSensitivity(int iso)
{
    if (isIsoSensitivitySupported(iso)) {
        mSettings->setManualIsoSensitivity(iso);
        return true;
    } else {
        QList<int> supportedIsoValues = mSettings->supportedIsoSensitivities();
        int minIso = supportedIsoValues.first();
        int maxIso = supportedIsoValues.last();

        if (iso < minIso) { // Smaller than minimum
            iso = minIso;
        } else if (iso > maxIso) { // Bigger than maximum
            iso = maxIso;
        } else { // Find closest
            int indexOfClosest = 0;
            int smallestDiff = 10000000; // Sensible max diff
            for(int i = 0; i < supportedIsoValues.count(); ++i) {
                int currentDiff = qAbs(iso - supportedIsoValues[i]);
                if(currentDiff < smallestDiff) {
                    smallestDiff = currentDiff;
                    indexOfClosest = i;
                }
            }
            iso = supportedIsoValues[indexOfClosest];
        }
        mSettings->setManualIsoSensitivity(iso);
    }

    return false;
}

void SimulatorCameraExposureControl::setAutoIsoSensitivity()
{
    mSettings->setAutoIsoSensitivity();
}


qreal SimulatorCameraExposureControl::aperture() const
{
    return mSettings->aperture();
}

bool SimulatorCameraExposureControl::isApertureSupported(const qreal aperture) const
{
    return mSettings->supportedApertures().contains(aperture);
}

bool SimulatorCameraExposureControl::setManualAperture(qreal aperture)
{
    if (isApertureSupported(aperture)) {
        mSettings->setManualAperture(aperture);
        return true;
    } else {
        QList<qreal> supportedApertureValues = mSettings->supportedApertures();
        qreal minAperture = supportedApertureValues.first();
        qreal maxAperture = supportedApertureValues.last();

        if (aperture < minAperture) { // Smaller than minimum
            aperture = minAperture;
        } else if (aperture > maxAperture) { // Bigger than maximum
            aperture = maxAperture;
        } else { // Find closest
            int indexOfClosest = 0;
            qreal smallestDiff = 100000000; // Sensible max diff
            for(int i = 0; i < supportedApertureValues.count(); ++i) {
                qreal currentDiff = qAbs(aperture - supportedApertureValues[i]);
                if(currentDiff < smallestDiff) {
                    smallestDiff = currentDiff;
                    indexOfClosest = i;
                }
            }
            aperture = supportedApertureValues[indexOfClosest];
        }
        mSettings->setManualAperture(aperture);
    }

    return false;
}

void SimulatorCameraExposureControl::setAutoAperture()
{
    mSettings->setAutoAperture();
}

qreal SimulatorCameraExposureControl::shutterSpeed() const
{
    return mSettings->shutterSpeed();
}

bool SimulatorCameraExposureControl::isShutterSpeedSupported(const qreal seconds) const
{
    return mSettings->supportedShutterSpeeds().contains(seconds);
}

bool SimulatorCameraExposureControl::setManualShutterSpeed(qreal seconds)
{
    if (isShutterSpeedSupported(seconds)) {
        mSettings->setManualShutterSpeed(seconds);
        return true;
    } else {
        QList<qreal> supportedShutterSpeeds = mSettings->supportedShutterSpeeds();

        qreal minShutterSpeed = supportedShutterSpeeds.first();
        qreal maxShutterSpeed = supportedShutterSpeeds.last();

        if (seconds < minShutterSpeed) { // Smaller than minimum
            seconds = minShutterSpeed;
        } else if (seconds > maxShutterSpeed) { // Bigger than maximum
            seconds = maxShutterSpeed;
        } else { // Find closest
            int indexOfClosest = 0;
            qreal smallestDiff = 100000000; // Sensible max diff
            for(int i = 0; i < supportedShutterSpeeds.count(); ++i) {
                qreal currentDiff = qAbs(seconds - supportedShutterSpeeds[i]);
                if(currentDiff < smallestDiff) {
                    smallestDiff = currentDiff;
                    indexOfClosest = i;
                }
            }
            seconds = supportedShutterSpeeds[indexOfClosest];
        }
        mSettings->setManualShutterSpeed(seconds);
    }

    return false;
}

void SimulatorCameraExposureControl::setAutoShutterSpeed()
{
    mSettings->setAutoShutterSpeed();
}

qreal SimulatorCameraExposureControl::exposureCompensation() const
{
    return mSettings->exposureCompensation();
}

bool SimulatorCameraExposureControl::isExposureCompensationSupported(const qreal ev) const
{
    QList<qreal> supportedValues = mSettings->supportedExposureCompensationValues();
    return (ev >= supportedValues.first() && ev <= supportedValues.last());
}

bool SimulatorCameraExposureControl::setManualExposureCompensation(qreal ev)
{
    if (isExposureCompensationSupported(ev)) {
        mSettings->setExposureCompensation(ev);
        return true;
    }

    return false;
}

void SimulatorCameraExposureControl::setAutoExposureCompensation()
{
    mSettings->setAutoExposureCompensation();
}

// End of file
