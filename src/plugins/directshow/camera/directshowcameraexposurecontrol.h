/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef DIRECTSHOWCAMERAEXPOSURECONTROL_H
#define DIRECTSHOWCAMERAEXPOSURECONTROL_H

#include <QtMultimedia/qcameraexposurecontrol.h>

struct IAMCameraControl;

QT_BEGIN_NAMESPACE

class DSCameraSession;

class DirectShowCameraExposureControl : public QCameraExposureControl
{
    Q_OBJECT
public:
    DirectShowCameraExposureControl(DSCameraSession *session);

    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const override;
    QVariant requestedValue(ExposureParameter parameter) const override;
    QVariant actualValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant &value) override;

private Q_SLOTS:
    void onStatusChanged(QCamera::Status status);

private:
    DSCameraSession *m_session;

    struct ExposureValues
    {
        long caps;
        long minValue;
        long maxValue;
        long stepping;
        long defaultValue;
    } m_shutterSpeedValues, m_apertureValues;

    qreal m_requestedShutterSpeed;
    qreal m_currentShutterSpeed;

    qreal m_requestedAperture;
    qreal m_currentAperture;

    QVariantList m_supportedShutterSpeeds;
    QVariantList m_supportedApertureValues;
    QVariantList m_supportedExposureModes;

    QCameraExposure::ExposureMode m_requestedExposureMode;
    QCameraExposure::ExposureMode m_currentExposureMode;

    void updateExposureSettings();

    bool setShutterSpeed(IAMCameraControl *cameraControl, qreal shutterSpeed);
    bool setAperture(IAMCameraControl *cameraControl, qreal aperture);
    bool setExposureMode(IAMCameraControl *cameraControl, QCameraExposure::ExposureMode mode);
};

QT_END_NAMESPACE

#endif // DIRECTSHOWCAMERAEXPOSURECONTROL_H
