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

#ifndef AVFCAMERAEXPOSURECONTROL_H
#define AVFCAMERAEXPOSURECONTROL_H

#include <QtMultimedia/qcameraexposurecontrol.h>
#include <QtMultimedia/qcameraexposure.h>

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class AVFCameraSession;
class AVFCameraService;

class AVFCameraExposureControl : public QCameraExposureControl
{
    Q_OBJECT

public:
    AVFCameraExposureControl(AVFCameraService *service);

    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariantList supportedParameterRange(ExposureParameter parameter,
                                         bool *continuous) const override;

    QVariant requestedValue(ExposureParameter parameter) const override;
    QVariant actualValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant &value) override;

private Q_SLOTS:
    void cameraStateChanged();

private:
    AVFCameraService *m_service;
    AVFCameraSession *m_session;

    QVariant m_requestedMode;
    QVariant m_requestedCompensation;
    QVariant m_requestedShutterSpeed;
    QVariant m_requestedISO;

    // Aux. setters:
    bool setExposureMode(const QVariant &value);
    bool setExposureCompensation(const QVariant &value);
    bool setShutterSpeed(const QVariant &value);
    bool setISO(const QVariant &value);
};

QT_END_NAMESPACE

#endif
