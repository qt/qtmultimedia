/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativecamera_p.h"
#include "qdeclarativecameraimageprocessing_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass CameraImageProcessing QDeclarativeCameraImageProcessing
    \brief The CameraCapture element provides an interface for camera capture related settings
    \ingroup multimedia_qml

    The CameraImageProcessing element provides control over post-processing
    done by the camera middleware, including white balance adjustments,
    contrast, saturation, sharpening, and denoising

    It is not constructed separately but is provided by the Camera element's
    \l {Camera::imageProcessing}{imageProcessing} property.

    \qml
    import QtQuick 2.0
    import QtMultimedia 5.0

    Camera {
        id: camera

        imageProcessing {
            whiteBalanceMode: Camera.WhiteBalanceTungsten
            contrast: 0.66
            saturation: -0.5
        }
    }

    \endqml


*/
/*!
    \class QDeclarativeCameraImageProcessing
    \internal
    \brief The CameraCapture element provides an interface for camera capture related settings
*/


QDeclarativeCameraImageProcessing::QDeclarativeCameraImageProcessing(QCamera *camera, QObject *parent) :
    QObject(parent)
{
    m_imageProcessing = camera->imageProcessing();
}

QDeclarativeCameraImageProcessing::~QDeclarativeCameraImageProcessing()
{
}

/*!
    \qmlproperty enumeration CameraImageProcessing::whiteBalanceMode

    \table
    \header \o Value \o Description
    \row \o WhiteBalanceManual       \o Manual white balance. In this mode the manual white balance property value is used.
    \row \o WhiteBalanceAuto         \o Auto white balance mode.
    \row \o WhiteBalanceSunlight     \o Sunlight white balance mode.
    \row \o WhiteBalanceCloudy       \o Cloudy white balance mode.
    \row \o WhiteBalanceShade        \o Shade white balance mode.
    \row \o WhiteBalanceTungsten     \o Tungsten white balance mode.
    \row \o WhiteBalanceFluorescent  \o Fluorescent white balance mode.
    \row \o WhiteBalanceFlash        \o Flash white balance mode.
    \row \o WhiteBalanceSunset       \o Sunset white balance mode.
    \row \o WhiteBalanceVendor       \o Vendor defined white balance mode.
    \endtable

    \sa manualWhiteBalance
*/
/*!
    \property QDeclarativeCameraImageProcessing::whiteBalanceMode

    \sa WhiteBalanceMode
*/
QDeclarativeCameraImageProcessing::WhiteBalanceMode QDeclarativeCameraImageProcessing::whiteBalanceMode() const
{
    return WhiteBalanceMode(m_imageProcessing->whiteBalanceMode());
}

void QDeclarativeCameraImageProcessing::setWhiteBalanceMode(QDeclarativeCameraImageProcessing::WhiteBalanceMode mode) const
{
    if (whiteBalanceMode() != mode) {
        m_imageProcessing->setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode(mode));
        emit whiteBalanceModeChanged(whiteBalanceMode());
    }
}

/*!
    \qmlproperty qreal CameraImageProcessing::manualWhiteBalance

    The color temperature used when in manual white balance mode (WhiteBalanceManual).
    The units are Kelvin.

    \sa whiteBalanceMode
*/
qreal QDeclarativeCameraImageProcessing::manualWhiteBalance() const
{
    return m_imageProcessing->manualWhiteBalance();
}

void QDeclarativeCameraImageProcessing::setManualWhiteBalance(qreal colorTemp) const
{
    if (manualWhiteBalance() != colorTemp) {
        m_imageProcessing->setManualWhiteBalance(colorTemp);
        emit manualWhiteBalanceChanged(manualWhiteBalance());
    }
}

/*!
    \qmlproperty int CameraImageProcessing::contrast

    Image contrast adjustment.
    Valid contrast adjustment values range between -1.0 and 1.0, with a default of 0.
*/
qreal QDeclarativeCameraImageProcessing::contrast() const
{
    return m_imageProcessing->contrast();
}

void QDeclarativeCameraImageProcessing::setContrast(qreal value)
{
    if (value != contrast()) {
        m_imageProcessing->setContrast(value);
        emit contrastChanged(contrast());
    }
}

/*!
    \qmlproperty int CameraImageProcessing::saturation

    Image saturation adjustment.
    Valid saturation adjustment values range between -1.0 and 1.0, the default is 0.
*/
qreal QDeclarativeCameraImageProcessing::saturation() const
{
    return m_imageProcessing->saturation();
}

void QDeclarativeCameraImageProcessing::setSaturation(qreal value)
{
    if (value != saturation()) {
        m_imageProcessing->setSaturation(value);
        emit saturationChanged(saturation());
    }
}

/*!
    \qmlproperty int CameraImageProcessing::sharpeningLevel

    Adjustment of sharpening level applied to image.

    Valid sharpening level values range between -1.0 for for sharpening disabled,
    0 for default sharpening level and 1.0 for maximum sharpening applied.
*/
qreal QDeclarativeCameraImageProcessing::sharpeningLevel() const
{
    return m_imageProcessing->sharpeningLevel();
}

void QDeclarativeCameraImageProcessing::setSharpeningLevel(qreal value)
{
    if (value != sharpeningLevel()) {
        m_imageProcessing->setSharpeningLevel(value);
        emit sharpeningLevelChanged(sharpeningLevel());
    }
}

/*!
    \qmlproperty int CameraImageProcessing::denoisingLevel

    Adjustment of denoising applied to image.

    Valid denoising level values range between -1.0 for for denoising disabled,
    0 for default denoising level and 1.0 for maximum denoising applied.
*/
qreal QDeclarativeCameraImageProcessing::denoisingLevel() const
{
    return m_imageProcessing->denoisingLevel();
}

void QDeclarativeCameraImageProcessing::setDenoisingLevel(qreal value)
{
    if (value != denoisingLevel()) {
        m_imageProcessing->setDenoisingLevel(value);
        emit denoisingLevelChanged(denoisingLevel());
    }
}

/*!
    \qmlsignal Camera::whiteBalanceModeChanged(Camera::WhiteBalanceMode)
*/

/*!
    \qmlsignal Camera::manualWhiteBalanceChanged(qreal)
*/

QT_END_NAMESPACE

#include "moc_qdeclarativecameraimageprocessing_p.cpp"
