/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qcameraexposurecontrol.h>
#include  "qmediacontrol_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QCameraExposureControl

    \brief The QCameraExposureControl class allows controlling camera exposure parameters.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_control

    You can adjust a number of parameters that will affect images and video taken with
    the corresponding QCamera object.

    There are a number of different parameters that can be adjusted, including:

    \table
    \row
    \header
    \

    \endtable

    The interface name of QCameraExposureControl is \c com.nokia.Qt.QCameraExposureControl/1.0 as
    defined in QCameraExposureControl_iid.

    \sa QCamera
*/

/*!
    \macro QCameraExposureControl_iid

    \c com.nokia.Qt.QCameraExposureControl/1.0

    Defines the interface name of the QCameraExposureControl class.

    \relates QCameraExposureControl
*/

/*!
    Constructs a camera exposure control object with \a parent.
*/
QCameraExposureControl::QCameraExposureControl(QObject *parent):
    QMediaControl(*new QMediaControlPrivate, parent)
{
}

/*!
    Destroys the camera exposure control object.
*/
QCameraExposureControl::~QCameraExposureControl()
{
}

/*!
  \enum QCameraExposureControl::ExposureParameter
  \value ISO
         Camera ISO sensitivity, specified as integer value.
  \value Aperture
         Lens aperture is specified as an qreal F number.
         The supported apertures list can change depending on the focal length,
         in such a case the exposureParameterRangeChanged() signal is emitted.
  \value ShutterSpeed
         Shutter speed in seconds, specified as qreal.
  \value ExposureCompensation
         Exposure compensation, specified as qreal EV value.
  \value FlashPower
         Manual flash power, specified as qreal value.
         Accepted power range is [0..1.0],
         with 0 value means no flash and 1.0 corresponds to full flash power.

         This value is only used in the \l{QCameraExposure::FlashManual}{manual flash mode}.
  \value TorchPower
         Manual torch power, specified as qreal value.
         Accepted power range is [0..1.0],
         with 0 value means no light and 1.0 corresponds to full torch power.

         This value is only used in the \l{QCameraExposure::FlashTorch}{torch flash mode}.
  \value FlashCompensation
         Flash compensation, specified as qreal EV value.
  \value SpotMeteringPoint
         The relative frame coordinate of the point to use for exposure metering
         in spot metering mode, specified as a QPointF.
  \value ExposureMode
         Camera exposure mode.
  \value MeteringMode
         Camera metering mode.
  \value ExtendedExposureParameter
         The base value for platform specific extended parameters.
         For such parameters the sequential values starting from ExtendedExposureParameter shuld be used.
*/

/*!
  \fn QCameraExposureControl::isParameterSupported(ExposureParameter parameter) const

  Returns true is exposure \a parameter is supported by backend.
  \since 5.0
*/

/*!
  \fn QCameraExposureControl::requestedValue(ExposureParameter parameter) const

  Returns the requested exposure \a parameter value.

  \since 5.0
*/

/*!
  \fn QCameraExposureControl::actualValue(ExposureParameter parameter) const

  Returns the actual exposure \a parameter value, or invalid QVariant() if the value is unknown or not supported.

  The actual parameter value may differ for the requested one if automatic mode is selected or
  camera supports only limited set of values within the supported range.
  \since 5.0
*/


/*!
  \fn QCameraExposureControl::supportedParameterRange(ExposureParameter parameter, bool *continuous = 0) const

  Returns the list of supported \a parameter values;

  If the camera supports arbitrary exposure parameter value within the supported range,
  *\a continuous is set to true, otherwise *\a continuous is set to false.

  \since 5.0
*/

/*!
  \fn bool QCameraExposureControl::setValue(ExposureParameter parameter, const QVariant& value)

  Set the exposure \a parameter to \a value.
  If a null or invalid QVariant is passed, backend should choose the value automatically,
  and if possible report the actual value to user with QCameraExposureControl::actualValue().

  Returns true if parameter is supported and value is correct.
  \since 5.0
*/

/*!
    \fn void QCameraExposureControl::requestedValueChanged(int parameter)

    Signal emitted when the requested exposure \a parameter value has changed,
    usually in result of setValue() call.
    \since 5.0
*/

/*!
    \fn void QCameraExposureControl::actualValueChanged(int parameter)

    Signal emitted when the actual exposure \a parameter value has changed,
    usually in result of auto exposure algorithms or manual exposure parameter applied.

    \since 5.0
*/

/*!
    \fn void QCameraExposureControl::parameterRangeChanged(int parameter)

    Signal emitted when the supported range of exposure \a parameter values has changed.

    \since 5.0
*/


#include "moc_qcameraexposurecontrol.cpp"
QT_END_NAMESPACE

