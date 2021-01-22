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

#ifndef QCAMERAIMAGEPROCESSINGCONTROL_H
#define QCAMERAIMAGEPROCESSINGCONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraImageProcessingControl : public QMediaControl
{
    Q_OBJECT
    Q_ENUMS(ProcessingParameter)

public:
    ~QCameraImageProcessingControl();

    enum ProcessingParameter {
        WhiteBalancePreset,
        ColorTemperature,
        Contrast,
        Saturation,
        Brightness,
        Sharpening,
        Denoising,
        ContrastAdjustment,
        SaturationAdjustment,
        BrightnessAdjustment,
        SharpeningAdjustment,
        DenoisingAdjustment,
        ColorFilter,
        ExtendedParameter = 1000
    };

    virtual bool isParameterSupported(ProcessingParameter) const = 0;
    virtual bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const = 0;
    virtual QVariant parameter(ProcessingParameter parameter) const = 0;
    virtual void setParameter(ProcessingParameter parameter, const QVariant &value) = 0;

protected:
    explicit QCameraImageProcessingControl(QObject *parent = nullptr);
};

#define QCameraImageProcessingControl_iid "org.qt-project.qt.cameraimageprocessingcontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraImageProcessingControl, QCameraImageProcessingControl_iid)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCameraImageProcessingControl::ProcessingParameter)

Q_MEDIA_ENUM_DEBUG(QCameraImageProcessingControl, ProcessingParameter)

#endif

