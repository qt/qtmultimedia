/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef MOCKCAMERAEXPOSURECONTROL_H
#define MOCKCAMERAEXPOSURECONTROL_H

#include "qcameraexposurecontrol.h"

class MockCameraExposureControl : public QCameraExposureControl
{
    Q_OBJECT
public:
    MockCameraExposureControl(QObject *parent = 0):
        QCameraExposureControl(parent),
        m_aperture(2.8),
        m_shutterSpeed(0.01),
        m_isoSensitivity(100),
        m_meteringMode(QCameraExposure::MeteringMatrix),
        m_exposureCompensation(0),
        m_exposureMode(QCameraExposure::ExposureAuto),
        m_flashMode(QCameraExposure::FlashAuto),
        m_spot(0.5, 0.5)
    {
        m_isoRanges << 100 << 200 << 400 << 800;
        m_apertureRanges << 2.8 << 4.0 << 5.6 << 8.0 << 11.0 << 16.0;
        m_shutterRanges << 0.001 << 0.01 << 0.1 << 1.0;
        m_exposureRanges << -2.0 << 2.0;
    }

    ~MockCameraExposureControl() {}

    QCameraExposure::FlashModes flashMode() const {return m_flashMode;}

    void setFlashMode(QCameraExposure::FlashModes mode)
    {
        if (isFlashModeSupported(mode)) {
            m_flashMode = mode;
        }
    }

    bool isFlashModeSupported(QCameraExposure::FlashModes mode) const
    {
        return mode & (QCameraExposure::FlashAuto | QCameraExposure::FlashOff | QCameraExposure::FlashOn);
    }

    bool isFlashReady() const { return true;}

    QCameraExposure::ExposureMode exposureMode() const  { return m_exposureMode; }

    void setExposureMode(QCameraExposure::ExposureMode mode)
    {
        if (isExposureModeSupported(mode))
            m_exposureMode = mode;
    }

    //Setting the Exposure Mode Supported Enum values
    bool isExposureModeSupported(QCameraExposure::ExposureMode mode) const
    {
        return ( mode == QCameraExposure::ExposureAuto || mode == QCameraExposure::ExposureManual || mode == QCameraExposure::ExposureBacklight ||
                mode == QCameraExposure::ExposureNight || mode == QCameraExposure::ExposureSpotlight ||mode == QCameraExposure::ExposureSports ||
                mode == QCameraExposure::ExposureSnow || mode == QCameraExposure:: ExposureLargeAperture ||mode == QCameraExposure::ExposureSmallAperture ||
                mode == QCameraExposure::ExposurePortrait || mode == QCameraExposure::ExposureModeVendor ||mode == QCameraExposure::ExposureBeach );
    }

    bool isParameterSupported(ExposureParameter parameter) const
    {
        switch (parameter) {
        case QCameraExposureControl::ExposureCompensation:
        case QCameraExposureControl::ISO:
        case QCameraExposureControl::Aperture:
        case QCameraExposureControl::ShutterSpeed:
        case QCameraExposureControl::SpotMeteringPoint:
            return true;
        default:
            return false;
        }
    }

    QVariant exposureParameter(ExposureParameter parameter) const
    {
        switch (parameter) {
        case QCameraExposureControl::ExposureCompensation:
            return QVariant(m_exposureCompensation);
        case QCameraExposureControl::ISO:
            return QVariant(m_isoSensitivity);
        case QCameraExposureControl::Aperture:
            return QVariant(m_aperture);
        case QCameraExposureControl::ShutterSpeed:
            return QVariant(m_shutterSpeed);
        case QCameraExposureControl::SpotMeteringPoint:
            return QVariant(m_spot);
        default:
            return QVariant();
        }
    }

    QVariantList supportedParameterRange(ExposureParameter parameter) const
    {
        QVariantList res;
        switch (parameter) {
        case QCameraExposureControl::ExposureCompensation:
            return m_exposureRanges;
        case QCameraExposureControl::ISO:
            return m_isoRanges;
        case QCameraExposureControl::Aperture:
            return m_apertureRanges;
        case QCameraExposureControl::ShutterSpeed:
            return m_shutterRanges;
        default:
            break;
        }

        return res;
    }

    ParameterFlags exposureParameterFlags(ExposureParameter parameter) const
    {
        ParameterFlags res = 0;
        switch (parameter) {
        case QCameraExposureControl::ExposureCompensation:
        case QCameraExposureControl::Aperture:
        case QCameraExposureControl::ShutterSpeed:
            res |= ContinuousRange;
        default:
            break;
        }

        return res;
    }

    // Added exposureParameterChanged  and exposureParameterRangeChanged signal
    bool setExposureParameter(ExposureParameter parameter, const QVariant& value)
    {
        switch (parameter) {
        case QCameraExposureControl::ExposureCompensation:
        {
            m_res.clear();
            m_res << -4.0 << 4.0;
            qreal exposureCompensationlocal = qBound<qreal>(-2.0, value.toReal(), 2.0);
            if (exposureParameter(parameter).toReal() !=  exposureCompensationlocal) {
                m_exposureCompensation = exposureCompensationlocal;
                emit exposureParameterChanged(parameter);
            }

            if (m_exposureRanges.last().toReal() != m_res.last().toReal()) {
                m_exposureRanges.clear();
                m_exposureRanges = m_res;
                emit exposureParameterRangeChanged(parameter);
            }
        }
            break;
        case QCameraExposureControl::ISO:
        {
            m_res.clear();
            m_res << 20 << 50;
            qreal exposureCompensationlocal = 100*qRound(qBound(100, value.toInt(), 800)/100.0);
            if (exposureParameter(parameter).toReal() !=  exposureCompensationlocal) {
                m_isoSensitivity = exposureCompensationlocal;
                emit exposureParameterChanged(parameter);
            }

            if (m_isoRanges.last().toInt() != m_res.last().toInt()) {
                m_isoRanges.clear();
                m_isoRanges = m_res;
                emit exposureParameterRangeChanged(parameter);
            }
        }
            break;
        case QCameraExposureControl::Aperture:
        {
            m_res.clear();
            m_res << 12.0 << 18.0 << 20.0;
            qreal exposureCompensationlocal = qBound<qreal>(2.8, value.toReal(), 16.0);
            if (exposureParameter(parameter).toReal() !=  exposureCompensationlocal) {
                m_aperture = exposureCompensationlocal;
                emit exposureParameterChanged(parameter);
            }

            if (m_apertureRanges.last().toReal() != m_res.last().toReal()) {
                m_apertureRanges.clear();
                m_apertureRanges = m_res;
                emit exposureParameterRangeChanged(parameter);
            }
        }
            break;
        case QCameraExposureControl::ShutterSpeed:
        {
            m_res.clear();
            m_res << 0.12 << 1.0 << 2.0;
            qreal exposureCompensationlocal = qBound<qreal>(0.001, value.toReal(), 1.0);
            if (exposureParameter(parameter).toReal() !=  exposureCompensationlocal) {
                m_shutterSpeed = exposureCompensationlocal;
                emit exposureParameterChanged(parameter);
            }

            if (m_shutterRanges.last().toReal() != m_res.last().toReal()) {
                m_shutterRanges.clear();
                m_shutterRanges = m_res;
                emit exposureParameterRangeChanged(parameter);
            }
        }
            break;

        case QCameraExposureControl::SpotMeteringPoint:
        {
            static QRectF valid(0, 0, 1, 1);
            if (valid.contains(value.toPointF())) {
                m_spot = value.toPointF();
                return true;
            }
            return false;
        }

        default:
            return false;
        }

        emit flashReady(true); // depends on Flashcontrol

        return true;
    }

    QString extendedParameterName(ExposureParameter)
    {
        return QString();
    }

    QCameraExposure::MeteringMode meteringMode() const
    {
        return m_meteringMode;
    }

    void setMeteringMode(QCameraExposure::MeteringMode mode)
    {
        if (isMeteringModeSupported(mode))
            m_meteringMode = mode;
    }

    //Setting the values for metering mode
    bool isMeteringModeSupported(QCameraExposure::MeteringMode mode) const
    {
        return mode == QCameraExposure::MeteringAverage
                || mode == QCameraExposure::MeteringMatrix
                || mode == QCameraExposure::MeteringSpot;
    }

private:
    qreal m_aperture;
    qreal m_shutterSpeed;
    int m_isoSensitivity;
    QCameraExposure::MeteringMode m_meteringMode;
    qreal m_exposureCompensation;
    QCameraExposure::ExposureMode m_exposureMode;
    QCameraExposure::FlashModes m_flashMode;
    QVariantList m_isoRanges,m_apertureRanges, m_shutterRanges, m_exposureRanges, m_res;
    QPointF m_spot;
};

#endif // MOCKCAMERAEXPOSURECONTROL_H
