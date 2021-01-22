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

#ifndef DSCAMERAIMAGEPROCESSINGCONTROL_H
#define DSCAMERAIMAGEPROCESSINGCONTROL_H

#include <qcamera.h>
#include <qcameraimageprocessingcontrol.h>

QT_BEGIN_NAMESPACE

class DSCameraSession;

class DSCameraImageProcessingControl : public QCameraImageProcessingControl
{
    Q_OBJECT

public:
    DSCameraImageProcessingControl(DSCameraSession *session);
     ~DSCameraImageProcessingControl() override;

    bool isParameterSupported(ProcessingParameter) const override;
    bool isParameterValueSupported(ProcessingParameter parameter,
                                   const QVariant &value) const override;
    QVariant parameter(ProcessingParameter parameter) const override;
    void setParameter(ProcessingParameter parameter, const QVariant &value) override;

private:
    DSCameraSession *m_session;
};

QT_END_NAMESPACE

#endif // DSCAMERAIMAGEPROCESSINGCONTROL_H
