/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qandroidcameraexposurecontrol_p.h"

#include "qandroidcamerasession_p.h"
#include "androidcamera_p.h"

QT_BEGIN_NAMESPACE

QAndroidCameraExposureControl::QAndroidCameraExposureControl(QAndroidCameraSession *session)
    : QPlatformCameraExposure()
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
    if (!m_session->camera())
        return false;

    switch (parameter) {
    case QPlatformCameraExposure::ISO:
        return false;
    case QPlatformCameraExposure::Aperture:
        return false;
    case QPlatformCameraExposure::ShutterSpeed:
        return false;
    case QPlatformCameraExposure::ExposureCompensation:
        return !m_supportedExposureCompensations.isEmpty();
    case QPlatformCameraExposure::TorchPower:
        return false;
    case QPlatformCameraExposure::ExposureMode:
        return !m_supportedExposureModes.isEmpty();
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

    if (parameter == QPlatformCameraExposure::ExposureCompensation)
        return m_supportedExposureCompensations;
    else if (parameter == QPlatformCameraExposure::ExposureMode)
        return m_supportedExposureModes;

    return QVariantList();
}

QVariant QAndroidCameraExposureControl::requestedValue(ExposureParameter parameter) const
{
    if (parameter == QPlatformCameraExposure::ExposureCompensation)
        return QVariant::fromValue(m_requestedExposureCompensation);
    else if (parameter == QPlatformCameraExposure::ExposureMode)
        return QVariant::fromValue(m_requestedExposureMode);

    return QVariant();
}

QVariant QAndroidCameraExposureControl::actualValue(ExposureParameter parameter) const
{
    if (parameter == QPlatformCameraExposure::ExposureCompensation)
        return QVariant::fromValue(m_actualExposureCompensation);
    else if (parameter == QPlatformCameraExposure::ExposureMode)
        return QVariant::fromValue(m_actualExposureMode);

    return QVariant();
}

bool QAndroidCameraExposureControl::setValue(ExposureParameter parameter, const QVariant& value)
{
    if (!value.isValid())
        return false;

    if (parameter == QPlatformCameraExposure::ExposureCompensation) {
        qreal expComp = value.toReal();
        if (!qFuzzyCompare(m_requestedExposureCompensation, expComp)) {
            m_requestedExposureCompensation = expComp;
            emit requestedValueChanged(QPlatformCameraExposure::ExposureCompensation);
        }

        if (!m_session->camera())
            return true;

        int expCompIndex = qRound(m_requestedExposureCompensation / m_exposureCompensationStep);
        if (expCompIndex >= m_minExposureCompensationIndex
                && expCompIndex <= m_maxExposureCompensationIndex) {
            qreal comp = expCompIndex * m_exposureCompensationStep;
            m_session->camera()->setExposureCompensation(expCompIndex);
            if (!qFuzzyCompare(m_actualExposureCompensation, comp)) {
                m_actualExposureCompensation = expCompIndex * m_exposureCompensationStep;
                emit actualValueChanged(QPlatformCameraExposure::ExposureCompensation);
            }

            return true;
        }

    } else if (parameter == QPlatformCameraExposure::ExposureMode) {
        QCameraExposure::ExposureMode expMode = value.value<QCameraExposure::ExposureMode>();
        if (m_requestedExposureMode != expMode) {
            m_requestedExposureMode = expMode;
            emit requestedValueChanged(QPlatformCameraExposure::ExposureMode);
        }

        if (!m_session->camera())
            return true;

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
            case QCameraExposure::ExposureAction:
                sceneMode = QLatin1String("action");
                break;
            case QCameraExposure::ExposureLandscape:
                sceneMode = QLatin1String("landscape");
                break;
            case QCameraExposure::ExposureNightPortrait:
                sceneMode = QLatin1String("night-portrait");
                break;
            case QCameraExposure::ExposureTheatre:
                sceneMode = QLatin1String("theatre");
                break;
            case QCameraExposure::ExposureSunset:
                sceneMode = QLatin1String("sunset");
                break;
            case QCameraExposure::ExposureSteadyPhoto:
                sceneMode = QLatin1String("steadyphoto");
                break;
            case QCameraExposure::ExposureFireworks:
                sceneMode = QLatin1String("fireworks");
                break;
            case QCameraExposure::ExposureParty:
                sceneMode = QLatin1String("party");
                break;
            case QCameraExposure::ExposureCandlelight:
                sceneMode = QLatin1String("candlelight");
                break;
            case QCameraExposure::ExposureBarcode:
                sceneMode = QLatin1String("barcode");
                break;
            default:
                sceneMode = QLatin1String("auto");
                m_actualExposureMode = QCameraExposure::ExposureAuto;
                break;
            }

            m_session->camera()->setSceneMode(sceneMode);
            emit actualValueChanged(QPlatformCameraExposure::ExposureMode);

            return true;
        }
    }

    return false;
}

void QAndroidCameraExposureControl::onCameraOpened()
{
    m_supportedExposureCompensations.clear();
    m_minExposureCompensationIndex = m_session->camera()->getMinExposureCompensation();
    m_maxExposureCompensationIndex = m_session->camera()->getMaxExposureCompensation();
    m_exposureCompensationStep = m_session->camera()->getExposureCompensationStep();
    if (m_minExposureCompensationIndex != 0 || m_maxExposureCompensationIndex != 0) {
        for (int i = m_minExposureCompensationIndex; i <= m_maxExposureCompensationIndex; ++i)
            m_supportedExposureCompensations.append(i * m_exposureCompensationStep);
        emit parameterRangeChanged(QPlatformCameraExposure::ExposureCompensation);
    }

    m_supportedExposureModes.clear();
    QStringList sceneModes = m_session->camera()->getSupportedSceneModes();
    if (!sceneModes.isEmpty()) {
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
            else if (sceneMode == QLatin1String("action"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureAction);
            else if (sceneMode == QLatin1String("landscape"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureLandscape);
            else if (sceneMode == QLatin1String("night-portrait"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureNightPortrait);
            else if (sceneMode == QLatin1String("theatre"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureTheatre);
            else if (sceneMode == QLatin1String("sunset"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureSunset);
            else if (sceneMode == QLatin1String("steadyphoto"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureSteadyPhoto);
            else if (sceneMode == QLatin1String("fireworks"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureFireworks);
            else if (sceneMode == QLatin1String("party"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureParty);
            else if (sceneMode == QLatin1String("candlelight"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureCandlelight);
            else if (sceneMode == QLatin1String("barcode"))
                m_supportedExposureModes << QVariant::fromValue(QCameraExposure::ExposureBarcode);
        }
        emit parameterRangeChanged(QPlatformCameraExposure::ExposureMode);
    }

    setValue(QPlatformCameraExposure::ExposureCompensation, QVariant::fromValue(m_requestedExposureCompensation));
    setValue(QPlatformCameraExposure::ExposureMode, QVariant::fromValue(m_requestedExposureMode));

    m_supportedFlashModes.clear();

    QStringList flashModes = m_session->camera()->getSupportedFlashModes();
    for (int i = 0; i < flashModes.size(); ++i) {
        const QString &flashMode = flashModes.at(i);
        if (flashMode == QLatin1String("off"))
            m_supportedFlashModes << QCameraExposure::FlashOff;
        else if (flashMode == QLatin1String("auto"))
            m_supportedFlashModes << QCameraExposure::FlashAuto;
        else if (flashMode == QLatin1String("on"))
            m_supportedFlashModes << QCameraExposure::FlashOn;
        else if (flashMode == QLatin1String("red-eye"))
            m_supportedFlashModes << QCameraExposure::FlashRedEyeReduction;
        else if (flashMode == QLatin1String("torch"))
            m_supportedFlashModes << QCameraExposure::FlashVideoLight;
    }

    if (!m_supportedFlashModes.contains(m_flashMode))
        m_flashMode = QCameraExposure::FlashOff;

    setFlashMode(m_flashMode);
}


QCameraExposure::FlashModes QAndroidCameraExposureControl::flashMode() const
{
    return m_flashMode;
}

void QAndroidCameraExposureControl::setFlashMode(QCameraExposure::FlashModes mode)
{
    if (!m_session->camera()) {
        m_flashMode = mode;
        return;
    }

    if (!isFlashModeSupported(mode))
        return;

    // if torch was enabled, it first needs to be turned off before setting another mode
    if (m_flashMode == QCameraExposure::FlashVideoLight)
        m_session->camera()->setFlashMode(QLatin1String("off"));

    m_flashMode = mode;

    QString flashMode;
    if (mode.testFlag(QCameraExposure::FlashAuto))
        flashMode = QLatin1String("auto");
    else if (mode.testFlag(QCameraExposure::FlashOn))
        flashMode = QLatin1String("on");
    else if (mode.testFlag(QCameraExposure::FlashRedEyeReduction))
        flashMode = QLatin1String("red-eye");
    else if (mode.testFlag(QCameraExposure::FlashVideoLight))
        flashMode = QLatin1String("torch");
    else // FlashOff
        flashMode = QLatin1String("off");

    m_session->camera()->setFlashMode(flashMode);
}

bool QAndroidCameraExposureControl::isFlashModeSupported(QCameraExposure::FlashModes mode) const
{
    return m_session->camera() ? m_supportedFlashModes.contains(mode) : false;
}

bool QAndroidCameraExposureControl::isFlashReady() const
{
    // Android doesn't have an API for that
    return true;
}

QT_END_NAMESPACE
