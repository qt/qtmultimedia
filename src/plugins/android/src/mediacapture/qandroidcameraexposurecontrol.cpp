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

#include "qandroidcameraexposurecontrol.h"

#include "qandroidcamerasession.h"
#include "androidcamera.h"

QT_BEGIN_NAMESPACE

QAndroidCameraExposureControl::QAndroidCameraExposureControl(QAndroidCameraSession *session)
    : QCameraExposureControl()
    , m_session(session)
    , m_minExposureCompensationIndex(0)
    , m_maxExposureCompensationIndex(0)
    , m_exposureCompensationStep(0.0)
    , m_requestedExposureCompensation(0.0)
    , m_actualExposureCompensation(0.0)
    , m_requestedExposureMode(QCameraExposure::ExposureAuto)
    , m_actualExposureMode(QCameraExposure::ExposureAuto)
{
    connect(m_session, SIGNAL(opened()),
            this, SLOT(onCameraOpened()));
}

bool QAndroidCameraExposureControl::isParameterSupported(ExposureParameter parameter) const
{
    switch (parameter) {
    case QCameraExposureControl::ISO:
        return false;
    case QCameraExposureControl::Aperture:
        return false;
    case QCameraExposureControl::ShutterSpeed:
        return false;
    case QCameraExposureControl::ExposureCompensation:
        return true;
    case QCameraExposureControl::FlashPower:
        return false;
    case QCameraExposureControl::FlashCompensation:
        return false;
    case QCameraExposureControl::TorchPower:
        return false;
    case QCameraExposureControl::SpotMeteringPoint:
        return false;
    case QCameraExposureControl::ExposureMode:
        return true;
    case QCameraExposureControl::MeteringMode:
        return false;
    default:
        return false;
    }
}

QVariantList QAndroidCameraExposureControl::supportedParameterRange(ExposureParameter parameter, bool *continuous) const
{
    if (!m_session->camera())
        return QVariantList();

    if (continuous)
        *continuous = false;

    if (parameter == QCameraExposureControl::ExposureCompensation)
        return m_supportedExposureCompensations;
    else if (parameter == QCameraExposureControl::ExposureMode)
        return m_supportedExposureModes;

    return QVariantList();
}

QVariant QAndroidCameraExposureControl::requestedValue(ExposureParameter parameter) const
{
    if (parameter == QCameraExposureControl::ExposureCompensation)
        return QVariant::fromValue(m_requestedExposureCompensation);
    else if (parameter == QCameraExposureControl::ExposureMode)
        return QVariant::fromValue(m_requestedExposureMode);

    return QVariant();
}

QVariant QAndroidCameraExposureControl::actualValue(ExposureParameter parameter) const
{
    if (parameter == QCameraExposureControl::ExposureCompensation)
        return QVariant::fromValue(m_actualExposureCompensation);
    else if (parameter == QCameraExposureControl::ExposureMode)
        return QVariant::fromValue(m_actualExposureMode);

    return QVariant();
}

bool QAndroidCameraExposureControl::setValue(ExposureParameter parameter, const QVariant& value)
{
    if (!m_session->camera() || !value.isValid())
        return false;

    if (parameter == QCameraExposureControl::ExposureCompensation) {
        m_requestedExposureCompensation = value.toReal();
        emit requestedValueChanged(QCameraExposureControl::ExposureCompensation);

        int expCompIndex = qRound(m_requestedExposureCompensation / m_exposureCompensationStep);
        if (expCompIndex >= m_minExposureCompensationIndex
                && expCompIndex <= m_maxExposureCompensationIndex) {
            m_session->camera()->setExposureCompensation(expCompIndex);

            m_actualExposureCompensation = expCompIndex * m_exposureCompensationStep;
            emit actualValueChanged(QCameraExposureControl::ExposureCompensation);

            return true;
        }

    } else if (parameter == QCameraExposureControl::ExposureMode) {
        m_requestedExposureMode = value.value<QCameraExposure::ExposureMode>();
        emit requestedValueChanged(QCameraExposureControl::ExposureMode);

        if (!m_supportedExposureModes.isEmpty()) {
            m_actualExposureMode = m_requestedExposureMode;

            QString sceneMode;
            switch (m_requestedExposureMode) {
            case QCameraExposure::ExposureAuto:
                sceneMode = QLatin1String("auto");
                break;
            case QCameraExposure::ExposureSports:
                sceneMode = QLatin1String("sports");
                break;
            case QCameraExposure::ExposurePortrait:
                sceneMode = QLatin1String("portrait");
                break;
            case QCameraExposure::ExposureBeach:
                sceneMode = QLatin1String("beach");
                break;
            case QCameraExposure::ExposureSnow:
                sceneMode = QLatin1String("snow");
                break;
            case QCameraExposure::ExposureNight:
                sceneMode = QLatin1String("night");
                break;
            default:
                sceneMode = QLatin1String("auto");
                m_actualExposureMode = QCameraExposure::ExposureAuto;
                break;
            }

            m_session->camera()->setSceneMode(sceneMode);
            emit actualValueChanged(QCameraExposureControl::ExposureMode);

            return true;
        }
    }

    return false;
}

void QAndroidCameraExposureControl::onCameraOpened()
{
    m_requestedExposureCompensation = m_actualExposureCompensation = 0.0;
    m_requestedExposureMode = m_actualExposureMode = QCameraExposure::ExposureAuto;
    emit requestedValueChanged(QCameraExposureControl::ExposureCompensation);
    emit actualValueChanged(QCameraExposureControl::ExposureCompensation);
    emit requestedValueChanged(QCameraExposureControl::ExposureMode);
    emit actualValueChanged(QCameraExposureControl::ExposureMode);

    m_minExposureCompensationIndex = m_session->camera()->getMinExposureCompensation();
    m_maxExposureCompensationIndex = m_session->camera()->getMaxExposureCompensation();
    m_exposureCompensationStep = m_session->camera()->getExposureCompensationStep();
    for (int i = m_minExposureCompensationIndex; i <= m_maxExposureCompensationIndex; ++i)
        m_supportedExposureCompensations.append(i * m_exposureCompensationStep);
    emit parameterRangeChanged(QCameraExposureControl::ExposureCompensation);

    m_supportedExposureModes.clear();
    QStringList sceneModes = m_session->camera()->getSupportedSceneModes();
    for (int i = 0; i < sceneModes.size(); ++i) {
        const QString &sceneMode = sceneModes.at(i);
        if (sceneMode == QLatin1String("auto"))
            m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureAuto);
        else if (sceneMode == QLatin1String("beach"))
            m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureBeach);
        else if (sceneMode == QLatin1String("night"))
            m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureNight);
        else if (sceneMode == QLatin1String("portrait"))
            m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposurePortrait);
        else if (sceneMode == QLatin1String("snow"))
            m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureSnow);
        else if (sceneMode == QLatin1String("sports"))
            m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureSports);
    }
    emit parameterRangeChanged(QCameraExposureControl::ExposureMode);
}

QT_END_NAMESPACE
