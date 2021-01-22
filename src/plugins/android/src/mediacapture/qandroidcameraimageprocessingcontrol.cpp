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

#include "qandroidcameraimageprocessingcontrol.h"

#include "qandroidcamerasession.h"
#include "androidcamera.h"

QT_BEGIN_NAMESPACE

QAndroidCameraImageProcessingControl::QAndroidCameraImageProcessingControl(QAndroidCameraSession *session)
    : QCameraImageProcessingControl()
    , m_session(session)
    , m_whiteBalanceMode(QCameraImageProcessing::WhiteBalanceAuto)
{
    connect(m_session, SIGNAL(opened()),
            this, SLOT(onCameraOpened()));
}

bool QAndroidCameraImageProcessingControl::isParameterSupported(ProcessingParameter parameter) const
{
    return parameter == QCameraImageProcessingControl::WhiteBalancePreset
            && m_session->camera()
            && !m_supportedWhiteBalanceModes.isEmpty();
}

bool QAndroidCameraImageProcessingControl::isParameterValueSupported(ProcessingParameter parameter,
                                                                     const QVariant &value) const
{
    return parameter == QCameraImageProcessingControl::WhiteBalancePreset
            && m_session->camera()
            && m_supportedWhiteBalanceModes.contains(value.value<QCameraImageProcessing::WhiteBalanceMode>());
}

QVariant QAndroidCameraImageProcessingControl::parameter(ProcessingParameter parameter) const
{
    if (parameter != QCameraImageProcessingControl::WhiteBalancePreset)
        return QVariant();

    return QVariant::fromValue(m_whiteBalanceMode);
}

void QAndroidCameraImageProcessingControl::setParameter(ProcessingParameter parameter, const QVariant &value)
{
    if (parameter != QCameraImageProcessingControl::WhiteBalancePreset)
        return;

    QCameraImageProcessing::WhiteBalanceMode mode = value.value<QCameraImageProcessing::WhiteBalanceMode>();

    if (m_session->camera())
        setWhiteBalanceModeHelper(mode);
    else
        m_whiteBalanceMode = mode;
}

void QAndroidCameraImageProcessingControl::setWhiteBalanceModeHelper(QCameraImageProcessing::WhiteBalanceMode mode)
{
    QString wb = m_supportedWhiteBalanceModes.value(mode, QString());
    if (!wb.isEmpty()) {
        m_session->camera()->setWhiteBalance(wb);
        m_whiteBalanceMode = mode;
    }
}

void QAndroidCameraImageProcessingControl::onCameraOpened()
{
    m_supportedWhiteBalanceModes.clear();
    QStringList whiteBalanceModes = m_session->camera()->getSupportedWhiteBalance();
    for (int i = 0; i < whiteBalanceModes.size(); ++i) {
        const QString &wb = whiteBalanceModes.at(i);
        if (wb == QLatin1String("auto")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceAuto,
                                                QStringLiteral("auto"));
        } else if (wb == QLatin1String("cloudy-daylight")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceCloudy,
                                                QStringLiteral("cloudy-daylight"));
        } else if (wb == QLatin1String("daylight")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceSunlight,
                                                QStringLiteral("daylight"));
        } else if (wb == QLatin1String("fluorescent")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceFluorescent,
                                                QStringLiteral("fluorescent"));
        } else if (wb == QLatin1String("incandescent")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceTungsten,
                                                QStringLiteral("incandescent"));
        } else if (wb == QLatin1String("shade")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceShade,
                                                QStringLiteral("shade"));
        } else if (wb == QLatin1String("twilight")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceSunset,
                                                QStringLiteral("twilight"));
        } else if (wb == QLatin1String("warm-fluorescent")) {
            m_supportedWhiteBalanceModes.insert(QCameraImageProcessing::WhiteBalanceFlash,
                                                QStringLiteral("warm-fluorescent"));
        }
    }

    if (!m_supportedWhiteBalanceModes.contains(m_whiteBalanceMode))
        m_whiteBalanceMode = QCameraImageProcessing::WhiteBalanceAuto;

    setWhiteBalanceModeHelper(m_whiteBalanceMode);
}

QT_END_NAMESPACE
