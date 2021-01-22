/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#ifndef BBCAMERAEXPOSURECONTROL_H
#define BBCAMERAEXPOSURECONTROL_H

#include <qcameraexposurecontrol.h>

QT_BEGIN_NAMESPACE

class BbCameraSession;

class BbCameraExposureControl : public QCameraExposureControl
{
    Q_OBJECT
public:
    explicit BbCameraExposureControl(BbCameraSession *session, QObject *parent = 0);

    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const override;

    QVariant requestedValue(ExposureParameter parameter) const override;
    QVariant actualValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant& value) override;

private Q_SLOTS:
    void statusChanged(QCamera::Status status);

private:
    BbCameraSession *m_session;
    QCameraExposure::ExposureMode m_requestedExposureMode;
};

QT_END_NAMESPACE

#endif
