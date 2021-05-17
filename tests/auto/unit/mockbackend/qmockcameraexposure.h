/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKCAMERAEXPOSURECONTROL_H
#define MOCKCAMERAEXPOSURECONTROL_H

#include "private/qplatformcameraexposure_p.h"

class QMockCameraExposure : public QPlatformCameraExposure
{
    Q_OBJECT
public:
    QMockCameraExposure(QObject *parent = 0):
        QPlatformCameraExposure(parent),
        m_shutterSpeed(0.01),
        m_isoSensitivity(100),
        m_exposureCompensation(0),
        m_exposureMode(QCamera::ExposureAuto),
        m_flashMode(QCamera::FlashAuto)
    {
        m_isoRanges << 100 << 200 << 400 << 800;
        m_shutterRanges << 0.001 << 0.01 << 0.1 << 1.0;
        m_exposureRanges << -2.0 << 2.0;

        const QCamera::ExposureMode exposureModes[] = {
            QCamera::ExposureAuto,
            QCamera::ExposureManual,
            QCamera::ExposureNight,
            QCamera::ExposureSports,
            QCamera::ExposureSnow,
            QCamera::ExposurePortrait,
            QCamera::ExposureBeach
        };

        for (QCamera::ExposureMode mode : exposureModes)
            m_exposureModes << QVariant::fromValue<QCamera::ExposureMode>(mode);
    }

    ~QMockCameraExposure() {}

    bool isParameterSupported(ExposureParameter parameter) const
    {
        switch (parameter) {
        case QPlatformCameraExposure::ExposureMode:
        case QPlatformCameraExposure::ExposureCompensation:
        case QPlatformCameraExposure::ISO:
        case QPlatformCameraExposure::ShutterSpeed:
            return true;
        default:
            return false;
        }
    }

    QVariant requestedValue(ExposureParameter param) const
    {
        return m_requestedParameters.value(param);
    }

    QVariant actualValue(ExposureParameter param) const
    {
        switch (param) {
        case QPlatformCameraExposure::ExposureMode:
            return QVariant::fromValue<QCamera::ExposureMode>(m_exposureMode);
        case QPlatformCameraExposure::ExposureCompensation:
            return QVariant(m_exposureCompensation);
        case QPlatformCameraExposure::ISO:
            return QVariant(m_isoSensitivity);
        case QPlatformCameraExposure::ShutterSpeed:
            return QVariant(m_shutterSpeed);
        default:
            return QVariant();
        }
    }

    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const
    {
        *continuous = false;

        QVariantList res;
        switch (parameter) {
        case QPlatformCameraExposure::ExposureCompensation:
            *continuous = true;
            return m_exposureRanges;
        case QPlatformCameraExposure::ISO:
            return m_isoRanges;
        case QPlatformCameraExposure::ShutterSpeed:
            *continuous = true;
            return m_shutterRanges;
        case QPlatformCameraExposure::ExposureMode:
            return m_exposureModes;
        default:
            break;
        }

        return res;
    }

    // Added valueChanged  and parameterRangeChanged signal
    bool setValue(ExposureParameter param, const QVariant& value)
    {
        if (!isParameterSupported(param))
            return false;

        if (m_requestedParameters.value(param) != value) {
            m_requestedParameters.insert(param, value);
            emit requestedValueChanged(param);
        }

        switch (param) {
        case QPlatformCameraExposure::ExposureMode:
        {
            QCamera::ExposureMode mode = value.value<QCamera::ExposureMode>();
            if (mode != m_exposureMode && m_exposureModes.contains(value)) {
                m_exposureMode = mode;
                emit actualValueChanged(param);
            }
        }
            break;
        case QPlatformCameraExposure::ExposureCompensation:
        {
            m_res.clear();
            m_res << -4.0 << 4.0;
            qreal exposureCompensationlocal = qBound<qreal>(-2.0, value.toReal(), 2.0);
            if (actualValue(param).toReal() !=  exposureCompensationlocal) {
                m_exposureCompensation = exposureCompensationlocal;
                emit actualValueChanged(param);
            }

            if (m_exposureRanges.last().toReal() != m_res.last().toReal()) {
                m_exposureRanges.clear();
                m_exposureRanges = m_res;
                emit parameterRangeChanged(param);
            }
        }
            break;
        case QPlatformCameraExposure::ISO:
        {
            m_res.clear();
            m_res << 20 << 50;
            qreal exposureCompensationlocal = 100*qRound(qBound(100, value.toInt(), 800)/100.0);
            if (actualValue(param).toReal() !=  exposureCompensationlocal) {
                m_isoSensitivity = exposureCompensationlocal;
                emit actualValueChanged(param);
            }

            if (m_isoRanges.last().toInt() != m_res.last().toInt()) {
                m_isoRanges.clear();
                m_isoRanges = m_res;
                emit parameterRangeChanged(param);
            }
        }
            break;
       case QPlatformCameraExposure::ShutterSpeed:
        {
            m_res.clear();
            m_res << 0.12 << 1.0 << 2.0;
            qreal exposureCompensationlocal = qBound<qreal>(0.001, value.toReal(), 1.0);
            if (actualValue(param).toReal() !=  exposureCompensationlocal) {
                m_shutterSpeed = exposureCompensationlocal;
                emit actualValueChanged(param);
            }

            if (m_shutterRanges.last().toReal() != m_res.last().toReal()) {
                m_shutterRanges.clear();
                m_shutterRanges = m_res;
                emit parameterRangeChanged(param);
            }
        }
            break;

        default:
            return false;
        }

        return true;
    }

    QCamera::FlashMode flashMode() const
    {
        return m_flashMode;
    }

    void setFlashMode(QCamera::FlashMode mode)
    {
        if (isFlashModeSupported(mode)) {
            m_flashMode = mode;
        }
        emit flashReady(true);
    }
    //Setting the values for Flash mode

    bool isFlashModeSupported(QCamera::FlashMode /*mode*/) const
    {
        return true;
    }

    bool isFlashReady() const
    {
        return true;
    }

private:
    qreal m_shutterSpeed;
    int m_isoSensitivity;
    qreal m_exposureCompensation;
    QCamera::ExposureMode m_exposureMode;
    QCamera::FlashMode m_flashMode;
    QVariantList m_isoRanges, m_shutterRanges, m_exposureRanges, m_res, m_exposureModes;

    QMap<QPlatformCameraExposure::ExposureParameter, QVariant> m_requestedParameters;
};

#endif // MOCKCAMERAEXPOSURECONTROL_H
