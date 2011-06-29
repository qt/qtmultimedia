/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/qstring.h>

#include "s60cameraimageprocessingcontrol.h"
#include "s60cameraservice.h"
#include "s60imagecapturesession.h"

S60CameraImageProcessingControl::S60CameraImageProcessingControl(QObject *parent) :
    QCameraImageProcessingControl(parent)
{
}

S60CameraImageProcessingControl::S60CameraImageProcessingControl(S60ImageCaptureSession *session, QObject *parent) :
    QCameraImageProcessingControl(parent),
    m_session(0),
    m_advancedSettings(0)
{
    m_session = session;
    m_advancedSettings = m_session->advancedSettings();
}

S60CameraImageProcessingControl::~S60CameraImageProcessingControl()
{
    m_advancedSettings = 0;
}

void S60CameraImageProcessingControl::resetAdvancedSetting()
{
    m_advancedSettings = m_session->advancedSettings();
}

QCameraImageProcessing::WhiteBalanceMode S60CameraImageProcessingControl::whiteBalanceMode() const
{
    return m_session->whiteBalanceMode();
}

void S60CameraImageProcessingControl::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
    if (isWhiteBalanceModeSupported(mode))
        m_session->setWhiteBalanceMode(mode);
    else
        m_session->setError(KErrNotSupported, tr("Requested white balance mode is not supported."));
}

bool S60CameraImageProcessingControl::isWhiteBalanceModeSupported(
    QCameraImageProcessing::WhiteBalanceMode mode) const
{
    return m_session->isWhiteBalanceModeSupported(mode);
}

int S60CameraImageProcessingControl::manualWhiteBalance() const
{
    return 0;
}

void S60CameraImageProcessingControl::setManualWhiteBalance(int colorTemperature)
{
    m_session->setError(KErrNotSupported, tr("Setting manual white balance is not supported."));
    Q_UNUSED(colorTemperature)
}

bool S60CameraImageProcessingControl::isProcessingParameterSupported(ProcessingParameter parameter) const
{
    // First check settings requiring Adv. Settings
    if (m_advancedSettings) {
        switch (parameter) {
            case QCameraImageProcessingControl::Saturation:
                return true;
            case QCameraImageProcessingControl::Sharpening:
                return isSharpeningSupported();
            case QCameraImageProcessingControl::Denoising:
                return isDenoisingSupported();
            case QCameraImageProcessingControl::ColorTemperature:
                return false;
        }
    }

    // Then the rest
    switch (parameter) {
        case QCameraImageProcessingControl::Contrast:
        case QCameraImageProcessingControl::Brightness:
            return true;

        default:
            return false;
    }
}

QVariant S60CameraImageProcessingControl::processingParameter(
    QCameraImageProcessingControl::ProcessingParameter parameter) const
{
    switch (parameter) {
        case QCameraImageProcessingControl::Contrast:
            return QVariant(contrast());
        case QCameraImageProcessingControl::Saturation:
            return QVariant(saturation());
        case QCameraImageProcessingControl::Brightness:
            return QVariant(brightness());
        case QCameraImageProcessingControl::Sharpening:
            return QVariant(sharpeningLevel());
        case QCameraImageProcessingControl::Denoising:
            return QVariant(denoisingLevel());
        case QCameraImageProcessingControl::ColorTemperature:
            return QVariant(manualWhiteBalance());

        default:
            return QVariant();
    }
}

void S60CameraImageProcessingControl::setProcessingParameter(
    QCameraImageProcessingControl::ProcessingParameter parameter, QVariant value)
{
    switch (parameter) {
        case QCameraImageProcessingControl::Contrast:
            setContrast(value.toInt());
            break;
        case QCameraImageProcessingControl::Saturation:
            setSaturation(value.toInt());
            break;
        case QCameraImageProcessingControl::Brightness:
            setBrightness(value.toInt());
            break;
        case QCameraImageProcessingControl::Sharpening:
            if (isSharpeningSupported())
                setSharpeningLevel(value.toInt());
            break;
        case QCameraImageProcessingControl::Denoising:
            if (isDenoisingSupported())
                setDenoisingLevel(value.toInt());
            break;
        case QCameraImageProcessingControl::ColorTemperature:
            setManualWhiteBalance(value.toInt());
            break;

        default:
            break;
    }
}

void S60CameraImageProcessingControl::setContrast(int value)
{
    m_session->setContrast(value);
}

int S60CameraImageProcessingControl::contrast() const
{
    return m_session->contrast();
}

void S60CameraImageProcessingControl::setBrightness(int value)
{
    m_session->setBrightness(value);
}

int S60CameraImageProcessingControl::brightness() const
{
    return m_session->brightness();
}

void S60CameraImageProcessingControl::setSaturation(int value)
{
    if (m_advancedSettings)
        m_advancedSettings->setSaturation(value);
    else
        m_session->setError(KErrNotSupported, tr("Setting saturation is not supported."));
}

int S60CameraImageProcessingControl::saturation() const
{
    if (m_advancedSettings)
        return m_advancedSettings->saturation();
    return 0;
}

void S60CameraImageProcessingControl::setDenoisingLevel(int value)
{
    m_session->setError(KErrNotSupported, tr("Setting denoising level is not supported."));
    Q_UNUSED(value); // Not supported for Symbian
}

bool S60CameraImageProcessingControl::isDenoisingSupported() const
{
    return false; // Not supported for Symbian
}

int S60CameraImageProcessingControl::denoisingLevel() const
{
    return 0; // Not supported for Symbian
}

void S60CameraImageProcessingControl::setSharpeningLevel(int value)
{
    if (m_advancedSettings)
        m_advancedSettings->setSharpeningLevel(value);
    else
        m_session->setError(KErrNotSupported, tr("Setting sharpening level is not supported."));
}

bool S60CameraImageProcessingControl::isSharpeningSupported() const
{
    if (m_advancedSettings)
        return m_advancedSettings->isSharpeningSupported();
    return false;
}

int S60CameraImageProcessingControl::sharpeningLevel() const
{
    if (m_advancedSettings)
        return m_advancedSettings->sharpeningLevel();
    return 0;
}

// End of file
