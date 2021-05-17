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
    , m_requestedExposureMode(QCamera::ExposureAuto)
    , m_actualExposureMode(QCamera::ExposureAuto)
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
        QCamera::ExposureMode expMode = value.value<QCamera::ExposureMode>();
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
            case QCamera::ExposureAuto:
                sceneMode = QLatin1String("auto");
                break;
            case QCamera::ExposureSports:
                sceneMode = QLatin1String("sports");
                break;
            case QCamera::ExposurePortrait:
                sceneMode = QLatin1String("portrait");
                break;
            case QCamera::ExposureBeach:
                sceneMode = QLatin1String("beach");
                break;
            case QCamera::ExposureSnow:
                sceneMode = QLatin1String("snow");
                break;
            case QCamera::ExposureNight:
                sceneMode = QLatin1String("night");
                break;
            case QCamera::ExposureAction:
                sceneMode = QLatin1String("action");
                break;
            case QCamera::ExposureLandscape:
                sceneMode = QLatin1String("landscape");
                break;
            case QCamera::ExposureNightPortrait:
                sceneMode = QLatin1String("night-portrait");
                break;
            case QCamera::ExposureTheatre:
                sceneMode = QLatin1String("theatre");
                break;
            case QCamera::ExposureSunset:
                sceneMode = QLatin1String("sunset");
                break;
            case QCamera::ExposureSteadyPhoto:
                sceneMode = QLatin1String("steadyphoto");
                break;
            case QCamera::ExposureFireworks:
                sceneMode = QLatin1String("fireworks");
                break;
            case QCamera::ExposureParty:
                sceneMode = QLatin1String("party");
                break;
            case QCamera::ExposureCandlelight:
                sceneMode = QLatin1String("candlelight");
                break;
            case QCamera::ExposureBarcode:
                sceneMode = QLatin1String("barcode");
                break;
            default:
                sceneMode = QLatin1String("auto");
                m_actualExposureMode = QCamera::ExposureAuto;
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
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureAuto);
            else if (sceneMode == QLatin1String("beach"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureBeach);
            else if (sceneMode == QLatin1String("night"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureNight);
            else if (sceneMode == QLatin1String("portrait"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposurePortrait);
            else if (sceneMode == QLatin1String("snow"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureSnow);
            else if (sceneMode == QLatin1String("sports"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureSports);
            else if (sceneMode == QLatin1String("action"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureAction);
            else if (sceneMode == QLatin1String("landscape"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureLandscape);
            else if (sceneMode == QLatin1String("night-portrait"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureNightPortrait);
            else if (sceneMode == QLatin1String("theatre"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureTheatre);
            else if (sceneMode == QLatin1String("sunset"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureSunset);
            else if (sceneMode == QLatin1String("steadyphoto"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureSteadyPhoto);
            else if (sceneMode == QLatin1String("fireworks"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureFireworks);
            else if (sceneMode == QLatin1String("party"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureParty);
            else if (sceneMode == QLatin1String("candlelight"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureCandlelight);
            else if (sceneMode == QLatin1String("barcode"))
                m_supportedExposureModes << QVariant::fromValue(QCamera::ExposureBarcode);
        }
        emit parameterRangeChanged(QPlatformCameraExposure::ExposureMode);
    }

    setValue(QPlatformCameraExposure::ExposureCompensation, QVariant::fromValue(m_requestedExposureCompensation));
    setValue(QPlatformCameraExposure::ExposureMode, QVariant::fromValue(m_requestedExposureMode));

    m_supportedFlashModes.clear();
    torchModeSupported = false;

    QStringList flashModes = m_session->camera()->getSupportedFlashModes();
    for (int i = 0; i < flashModes.size(); ++i) {
        const QString &flashMode = flashModes.at(i);
        if (flashMode == QLatin1String("off"))
            m_supportedFlashModes << QCamera::FlashOff;
        else if (flashMode == QLatin1String("auto"))
            m_supportedFlashModes << QCamera::FlashAuto;
        else if (flashMode == QLatin1String("on"))
            m_supportedFlashModes << QCamera::FlashOn;
        else if (flashMode == QLatin1String("torch"))
            torchModeSupported = true;
    }

    if (!m_supportedFlashModes.contains(m_flashMode))
        m_flashMode = QCamera::FlashOff;

    setFlashMode(m_flashMode);
}


QCamera::FlashMode QAndroidCameraExposureControl::flashMode() const
{
    return m_flashMode;
}

void QAndroidCameraExposureControl::setFlashMode(QCamera::FlashMode mode)
{
    if (!m_session->camera()) {
        m_flashMode = QCamera::FlashOff;
        return;
    }

    if (!isFlashModeSupported(mode))
        return;


    m_flashMode = mode;

    QString flashMode;
    if (mode == QCamera::FlashAuto)
        flashMode = QLatin1String("auto");
    else if (mode == QCamera::FlashOn)
        flashMode = QLatin1String("on");
    else // FlashOff
        flashMode = QLatin1String("off");

    m_session->camera()->setFlashMode(flashMode);
}

bool QAndroidCameraExposureControl::isFlashModeSupported(QCamera::FlashMode mode) const
{
    return m_session->camera() ? m_supportedFlashModes.contains(mode) : false;
}

bool QAndroidCameraExposureControl::isFlashReady() const
{
    // Android doesn't have an API for that
    return true;
}

QCamera::TorchMode QAndroidCameraExposureControl::torchMode() const
{
    return torchEnabled ? QCamera::TorchOn : QCamera::TorchOff;
}

void QAndroidCameraExposureControl::setTorchMode(QCamera::TorchMode mode)
{
    auto *camera = m_session->camera();
    if (!camera || !torchModeSupported)
        return;

    if (mode == QCamera::TorchOn) {
        camera->setFlashMode(QLatin1String("torch"));
        torchEnabled = true;
    } else if (mode == QCamera::TorchOff) {
        // if torch was enabled, it first needs to be turned off before restoring the flash mode
        camera->setFlashMode(QLatin1String("off"));
        setFlashMode(m_flashMode);
        torchEnabled = false;
    }
}

bool QAndroidCameraExposureControl::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (mode == QCamera::TorchAuto)
        return false;
    else if (mode == QCamera::TorchOn)
        return torchModeSupported;
    else
        return true;
}

QT_END_NAMESPACE
