/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#ifndef CAMERABINV4LIMAGEPROCESSINGCONTROL_H
#define CAMERABINV4LIMAGEPROCESSINGCONTROL_H

#include <qcamera.h>
#include <qcameraimageprocessingcontrol.h>

QT_BEGIN_NAMESPACE

class CameraBinSession;

class CameraBinV4LImageProcessing : public QCameraImageProcessingControl
{
    Q_OBJECT

public:
    CameraBinV4LImageProcessing(CameraBinSession *session);
    virtual ~CameraBinV4LImageProcessing();

    bool isParameterSupported(ProcessingParameter) const override;
    bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const override;
    QVariant parameter(ProcessingParameter parameter) const override;
    void setParameter(ProcessingParameter parameter, const QVariant &value) override;

public slots:
    void updateParametersInfo(QCamera::Status cameraStatus);

private:
    struct SourceParameterValueInfo {
        SourceParameterValueInfo()
            : cid(0)
        {
        }

        qint32 defaultValue;
        qint32 minimumValue;
        qint32 maximumValue;
        quint32 cid; // V4L control id
    };

    static qreal scaledImageProcessingParameterValue(
            qint32 sourceValue, const SourceParameterValueInfo &sourceValueInfo);
    static qint32 sourceImageProcessingParameterValue(
            qreal scaledValue, const SourceParameterValueInfo &valueRange);
private:
    CameraBinSession *m_session;
    QMap<ProcessingParameter, SourceParameterValueInfo> m_parametersInfo;
};

QT_END_NAMESPACE

#endif // CAMERABINV4LIMAGEPROCESSINGCONTROL_H
