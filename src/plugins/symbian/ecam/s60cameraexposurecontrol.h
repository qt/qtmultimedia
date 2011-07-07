/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
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

#ifndef S60CAMERAEXPOSURECONTROL_H
#define S60CAMERAEXPOSURECONTROL_H

#include <qcameraexposurecontrol.h>

#include "s60camerasettings.h"

QT_USE_NAMESPACE

class S60CameraService;
class S60ImageCaptureSession;

/*
 * Control for exposure related camera operation.
 */
class S60CameraExposureControl : public QCameraExposureControl
{
    Q_OBJECT

public: // Constructors & Destructor

    S60CameraExposureControl(QObject *parent = 0);
    S60CameraExposureControl(S60ImageCaptureSession *session, QObject *parent = 0);
    ~S60CameraExposureControl();

public: // QCameraExposureControl

    // Exposure Mode
    QCameraExposure::ExposureMode exposureMode() const;
    void setExposureMode(QCameraExposure::ExposureMode mode);
    bool isExposureModeSupported(QCameraExposure::ExposureMode mode) const;

    // Metering Mode
    QCameraExposure::MeteringMode meteringMode() const;
    void setMeteringMode(QCameraExposure::MeteringMode mode);
    bool isMeteringModeSupported(QCameraExposure::MeteringMode mode) const;

    // Exposure Parameter
    bool isParameterSupported(ExposureParameter parameter) const;
    QVariant exposureParameter(ExposureParameter parameter) const;
    QCameraExposureControl::ParameterFlags exposureParameterFlags(ExposureParameter parameter) const;
    QVariantList supportedParameterRange(ExposureParameter parameter) const;
    bool setExposureParameter(ExposureParameter parameter, const QVariant& value);

    QString extendedParameterName(ExposureParameter parameter);

/*
Q_SIGNALS: // QCameraExposureControl
    void exposureParameterChanged(int parameter);
    void exposureParameterRangeChanged(int parameter);
*/

private slots: // Internal Slots

    void resetAdvancedSetting();
    void apertureChanged();
    void apertureRangeChanged();
    void shutterSpeedChanged();
    void isoSensitivityChanged();
    void evChanged();

private: // Internal - Implementing ExposureParameter

    // ISO Sensitivity
    int isoSensitivity() const;
    bool setManualIsoSensitivity(int iso);
    void setAutoIsoSensitivity();
    bool isIsoSensitivitySupported(const int iso) const;

    // Aperture
    qreal aperture() const;
    bool setManualAperture(qreal aperture);
    void setAutoAperture();
    bool isApertureSupported(const qreal aperture) const;

    // Shutter Speed
    qreal shutterSpeed() const;
    bool setManualShutterSpeed(qreal seconds);
    void setAutoShutterSpeed();
    bool isShutterSpeedSupported(const qreal seconds) const;

    // Exposure Compensation
    qreal exposureCompensation() const;
    bool setManualExposureCompensation(qreal ev);
    void setAutoExposureCompensation();
    bool isExposureCompensationSupported(const qreal ev) const;

private: // Data

    S60ImageCaptureSession          *m_session;
    S60CameraService                *m_service;
    S60CameraSettings               *m_advancedSettings;
    QCameraExposure::ExposureMode   m_exposureMode;
    QCameraExposure::MeteringMode   m_meteringMode;
};

#endif // S60CAMERAEXPOSURECONTROL_H
