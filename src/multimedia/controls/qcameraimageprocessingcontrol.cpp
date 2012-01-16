/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qcameraimageprocessingcontrol.h>
#include  "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

namespace
{
    class QCameraImageProcessingControlPrivateRegisterMetaTypes
    {
    public:
        QCameraImageProcessingControlPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QCameraImageProcessingControl::ProcessingParameter>();
        }
    } _registerMetaTypes;
}

/*!
    \class QCameraImageProcessingControl
    \inmodule QtMultimedia

    \ingroup multimedia
    \ingroup multimedia_control


    \brief The QCameraImageProcessingControl class provides an abstract class
    for controlling image processing parameters, like white balance,
    contrast, saturation, sharpening and denoising.

    The interface name of QCameraImageProcessingControl is \c com.nokia.Qt.QCameraImageProcessingControl/1.0 as
    defined in QCameraImageProcessingControl_iid.



    \sa QMediaService::requestControl(), QCamera
*/

/*!
    \macro QCameraImageProcessingControl_iid

    \c com.nokia.Qt.QCameraImageProcessingControl/1.0

    Defines the interface name of the QCameraImageProcessingControl class.

    \relates QCameraImageProcessingControl
*/

/*!
    Constructs an image processing control object with \a parent.
*/

QCameraImageProcessingControl::QCameraImageProcessingControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destruct the image processing control object.
*/

QCameraImageProcessingControl::~QCameraImageProcessingControl()
{
}


/*!
    \fn QCameraImageProcessingControl::whiteBalanceMode() const
    Return the white balance mode being used.
*/

/*!
    \fn QCameraImageProcessingControl::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
    Set the white balance mode to \a mode
*/

/*!
    \fn QCameraImageProcessingControl::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
    Returns true if the white balance \a mode is supported.
    The backend should support at least QCameraImageProcessing::WhiteBalanceAuto mode.
*/

/*!
    \fn bool QCameraImageProcessingControl::isProcessingParameterSupported(ProcessingParameter parameter) const

    Returns true if the camera supports adjusting image processing \a parameter.

    Usually the the supported settings is static,
    but some parameter may not be available depending on other
    camera settings, like presets.
    In such case the currently supported parameters should be returned.
*/

/*!
    \fn QCameraImageProcessingControl::processingParameter(ProcessingParameter parameter) const
    Returns the image processing \a parameter value.
*/

/*!
    \fn QCameraImageProcessingControl::setProcessingParameter(ProcessingParameter parameter, QVariant value)

    Sets the image processing \a parameter \a value.
    Passing the null or invalid QVariant value allows
    backend to choose the suitable parameter value.

    The valid values range depends on the parameter type,
    for contrast, saturation and brightness value should be
    between -100 and 100, the default is 0,

    For sharpening and denoising the range is 0..100,
    0 for sharpening or denoising disabled
    and 100 for maximum sharpening/denoising applied.
*/

/*!
  \enum QCameraImageProcessingControl::ProcessingParameter

  \value Contrast
    Image contrast.
  \value Saturation
    Image saturation.
  \value Brightness
    Image brightness.
  \value Sharpening
    Amount of sharpening applied.
  \value Denoising
    Amount of denoising applied.
  \value ColorTemperature
    Color temperature in K. This value is used when the manual white balance mode is selected.
  \value ExtendedParameter
    The base value for platform specific extended parameters.
 */

#include "moc_qcameraimageprocessingcontrol.cpp"
QT_END_NAMESPACE

