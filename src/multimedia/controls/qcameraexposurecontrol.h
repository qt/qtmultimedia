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

#ifndef QCAMERAEXPOSURECONTROL_H
#define QCAMERAEXPOSURECONTROL_H

#include <QtMultimedia/qmediacontrol.h>
#include <QtMultimedia/qmediaobject.h>

#include <QtMultimedia/qcameraexposure.h>
#include <QtMultimedia/qcamera.h>
#include <QtMultimedia/qmediaenumdebug.h>

QT_BEGIN_NAMESPACE

// Required for QDoc workaround
class QString;

class Q_MULTIMEDIA_EXPORT QCameraExposureControl : public QMediaControl
{
    Q_OBJECT
    Q_ENUMS(ExposureParameter)

public:
    ~QCameraExposureControl();

    enum ExposureParameter {
        ISO,
        Aperture,
        ShutterSpeed,
        ExposureCompensation,
        FlashPower,
        FlashCompensation,
        TorchPower,
        SpotMeteringPoint,
        ExposureMode,
        MeteringMode,
        ExtendedExposureParameter = 1000
    };

    virtual bool isParameterSupported(ExposureParameter parameter) const = 0;
    virtual QVariantList supportedParameterRange(ExposureParameter parameter, bool *continuous) const = 0;

    virtual QVariant requestedValue(ExposureParameter parameter) const = 0;
    virtual QVariant actualValue(ExposureParameter parameter) const = 0;
    virtual bool setValue(ExposureParameter parameter, const QVariant& value) = 0;

Q_SIGNALS:
    void requestedValueChanged(int parameter);
    void actualValueChanged(int parameter);
    void parameterRangeChanged(int parameter);

protected:
    explicit QCameraExposureControl(QObject *parent = nullptr);
};

#define QCameraExposureControl_iid "org.qt-project.qt.cameraexposurecontrol/5.0"
Q_MEDIA_DECLARE_CONTROL(QCameraExposureControl, QCameraExposureControl_iid)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QCameraExposureControl::ExposureParameter)

Q_MEDIA_ENUM_DEBUG(QCameraExposureControl, ExposureParameter)


#endif  // QCAMERAEXPOSURECONTROL_H

