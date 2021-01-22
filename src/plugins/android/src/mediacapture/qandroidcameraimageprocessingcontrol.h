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

#ifndef QANDROIDCAMERAIMAGEPROCESSINGCONTROL_H
#define QANDROIDCAMERAIMAGEPROCESSINGCONTROL_H

#include <qcameraimageprocessingcontrol.h>

QT_BEGIN_NAMESPACE

class QAndroidCameraSession;

class QAndroidCameraImageProcessingControl : public QCameraImageProcessingControl
{
    Q_OBJECT
public:
    explicit QAndroidCameraImageProcessingControl(QAndroidCameraSession *session);

    bool isParameterSupported(ProcessingParameter) const override;
    bool isParameterValueSupported(ProcessingParameter parameter, const QVariant &value) const override;
    QVariant parameter(ProcessingParameter parameter) const override;
    void setParameter(ProcessingParameter parameter, const QVariant &value) override;

private Q_SLOTS:
    void onCameraOpened();

private:
    void setWhiteBalanceModeHelper(QCameraImageProcessing::WhiteBalanceMode mode);

    QAndroidCameraSession *m_session;

    QCameraImageProcessing::WhiteBalanceMode m_whiteBalanceMode;

    QMap<QCameraImageProcessing::WhiteBalanceMode, QString> m_supportedWhiteBalanceModes;
};

QT_END_NAMESPACE

#endif // QANDROIDCAMERAIMAGEPROCESSINGCONTROL_H
