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

#ifndef CAMERABINEXPOSURECONTROL_H
#define CAMERABINEXPOSURECONTROL_H

#include <qcamera.h>
#include <qcameraexposurecontrol.h>

#include <gst/gst.h>
#include <glib.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinExposure : public QCameraExposureControl
{
    Q_OBJECT

public:
    CameraBinExposure(CameraBinSession *session);
    virtual ~CameraBinExposure();

    bool isParameterSupported(ExposureParameter parameter) const override;
    QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const override;

    QVariant requestedValue(ExposureParameter parameter) const override;
    QVariant actualValue(ExposureParameter parameter) const override;
    bool setValue(ExposureParameter parameter, const QVariant &value) override;

private:
    CameraBinSession *m_session;
    QHash<ExposureParameter, QVariant> m_requestedValues;
};

QT_END_NAMESPACE

#endif // CAMERABINEXPOSURECONTROL_H
