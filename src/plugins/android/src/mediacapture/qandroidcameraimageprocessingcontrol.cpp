/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidcameraimageprocessingcontrol.h"

#include "qandroidcamerasession.h"
#include "androidcamera.h"

QT_BEGIN_NAMESPACE

QAndroidCameraImageProcessingControl::QAndroidCameraImageProcessingControl(QAndroidCameraSession *session)
    : QCameraImageProcessingControl()
    , m_session(session)
{
    connect(m_session, SIGNAL(opened()),
            this, SLOT(onCameraOpened()));
}

bool QAndroidCameraImageProcessingControl::isParameterSupported(ProcessingParameter parameter) const
{
    return (parameter == QCameraImageProcessingControl::WhiteBalancePreset);
}

bool QAndroidCameraImageProcessingControl::isParameterValueSupported(ProcessingParameter parameter,
                                                                     const QVariant &value) const
{
    if (parameter != QCameraImageProcessingControl::WhiteBalancePreset)
        return false;

    if (!m_session->camera())
        return false;

    return m_supportedWhiteBalanceModes.contains(value.value<QCameraImageProcessing::WhiteBalanceMode>());
}

QVariant QAndroidCameraImageProcessingControl::parameter(ProcessingParameter parameter) const
{
    if (parameter != QCameraImageProcessingControl::WhiteBalancePreset)
        return QVariant();

    if (!m_session->camera())
        return QVariant();

    QString wb = m_session->camera()->getWhiteBalance();
    QCameraImageProcessing::WhiteBalanceMode mode = m_supportedWhiteBalanceModes.key(wb, QCameraImageProcessing::WhiteBalanceAuto);

    return QVariant::fromValue(mode);
}

void QAndroidCameraImageProcessingControl::setParameter(ProcessingParameter parameter, const QVariant &value)
{
    if (parameter != QCameraImageProcessingControl::WhiteBalancePreset)
        return;

    if (!m_session->camera())
        return;

    QString wb = m_supportedWhiteBalanceModes.value(value.value<QCameraImageProcessing::WhiteBalanceMode>(), QString());
    if (!wb.isEmpty())
        m_session->camera()->setWhiteBalance(wb);
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
}

QT_END_NAMESPACE
