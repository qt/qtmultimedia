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

#ifndef QANDROIDCAMERAEXPOSURECONTROL_H
#define QANDROIDCAMERAEXPOSURECONTROL_H

#include <qcameraexposurecontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidCameraExposureControl : public QCameraExposureControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraExposureControl(QAndroidCameraSession *session);

    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const override;

    QVariant requestedValue(ExposureParameter parameter) const override;
    QVariant actualValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant& value) override;

private Q_SLOTS:
    void onCameraOpened();

private:
    QAndroidCameraSession *m_session;

    QVariantList m_supportedExposureCompensations;
    QVariantList m_supportedExposureModes;

    int m_minExposureCompensationIndex;
    int m_maxExposureCompensationIndex;
    qreal m_exposureCompensationStep;

    qreal m_requestedExposureCompensation;
    qreal m_actualExposureCompensation;
    QCameraExposure::ExposureMode m_requestedExposureMode;
    QCameraExposure::ExposureMode m_actualExposureMode;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAEXPOSURECONTROL_H
