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

#include "qcameraimageprocessing.h"
#include "qmediaobject_p.h"

#include <qcameracontrol.h>
#include <qcameraexposurecontrol.h>
#include <qcamerafocuscontrol.h>
#include <qmediarecordercontrol.h>
#include <qcameraimageprocessingcontrol.h>
#include <qcameraimagecapturecontrol.h>
#include <qvideodevicecontrol.h>

#include <QtCore/QDebug>

namespace
{
    class QCameraImageProcessingPrivateRegisterMetaTypes
    {
    public:
        QCameraImageProcessingPrivateRegisterMetaTypes()
        {
            qRegisterMetaType<QCameraImageProcessing::WhiteBalanceMode>();
        }
    } _registerMetaTypes;
}


QT_BEGIN_NAMESPACE

/*!
    \class QCameraImageProcessing

    \brief The QCameraImageProcessing class provides an interface for
    image processing related camera settings.

    \inmodule QtMultimedia
    \ingroup camera

    After capturing the data for a camera frame, the camera hardware and
    software performs various image processing tasks to produce a final
    image.  This includes compensating for ambient light color, reducing
    noise, as well as making some other adjustments to the image.

    You can retrieve this class from an instance of a \l QCamera object.

    For example, you can set the white balance (or color temperature) used
    for processing images:

    \snippet doc/src/snippets/multimedia-snippets/camera.cpp Camera image whitebalance

    Or adjust the amount of denoising performed:

    \snippet doc/src/snippets/multimedia-snippets/camera.cpp Camera image denoising

    In some cases changing these settings may result in a longer delay
    before an image is ready.

    For more information on image processing of camera frames, see \l {Camera Image Processing}.

    \sa QCameraImageProcessingControl
*/


class QCameraImageProcessingPrivate : public QMediaObjectPrivate
{
    Q_DECLARE_NON_CONST_PUBLIC(QCameraImageProcessing)
public:
    void initControls();

    QCameraImageProcessing *q_ptr;

    QCamera *camera;
    QCameraImageProcessingControl *imageControl;
};


void QCameraImageProcessingPrivate::initControls()
{
    imageControl = 0;

    QMediaService *service = camera->service();
    if (service)
        imageControl = qobject_cast<QCameraImageProcessingControl *>(service->requestControl(QCameraImageProcessingControl_iid));
}

/*!
    Construct a QCameraImageProcessing for \a camera.
*/

QCameraImageProcessing::QCameraImageProcessing(QCamera *camera):
    QObject(camera), d_ptr(new QCameraImageProcessingPrivate)
{
    Q_D(QCameraImageProcessing);
    d->camera = camera;
    d->q_ptr = this;
    d->initControls();
}


/*!
    Destroys the camera focus object.
*/

QCameraImageProcessing::~QCameraImageProcessing()
{
}


/*!
    Returns true if image processing related settings are supported by this camera.
*/
bool QCameraImageProcessing::isAvailable() const
{
    return d_func()->imageControl != 0;
}


/*!
    Returns the white balance mode being used.
*/

QCameraImageProcessing::WhiteBalanceMode QCameraImageProcessing::whiteBalanceMode() const
{
    return d_func()->imageControl ? d_func()->imageControl->whiteBalanceMode() : QCameraImageProcessing::WhiteBalanceAuto;
}

/*!
    Sets the white balance to \a mode.
*/

void QCameraImageProcessing::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
    if (d_func()->imageControl)
        d_func()->imageControl->setWhiteBalanceMode(mode);
}

/*!
    Returns true if the white balance \a mode is supported.
*/

bool QCameraImageProcessing::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
{
    return d_func()->imageControl ? d_func()->imageControl->isWhiteBalanceModeSupported(mode) : false;
}

/*!
    Returns the current color temperature if the
    current white balance mode is \c WhiteBalanceManual.  For other modes the
    return value is undefined.
*/

int QCameraImageProcessing::manualWhiteBalance() const
{
    QVariant value;

    if (d_func()->imageControl)
        value = d_func()->imageControl->processingParameter(QCameraImageProcessingControl::ColorTemperature);

    return value.toInt();
}

/*!
    Sets manual white balance to \a colorTemperature.  This is used
    when whiteBalanceMode() is set to \c WhiteBalanceManual.  The units are Kelvin.
*/

void QCameraImageProcessing::setManualWhiteBalance(int colorTemperature)
{
    if (d_func()->imageControl) {
        d_func()->imageControl->setProcessingParameter(
                    QCameraImageProcessingControl::ColorTemperature,
                    QVariant(colorTemperature));
    }
}

/*!
    Returns the contrast adjustment setting.
*/
int QCameraImageProcessing::contrast() const
{
    QVariant value;

    if (d_func()->imageControl)
        value = d_func()->imageControl->processingParameter(QCameraImageProcessingControl::Contrast);

    return value.toInt();
}

/*!
    Set the contrast adjustment to \a value.

    Valid contrast adjustment values range between -100 and 100, with a default of 0.
*/
void QCameraImageProcessing::setContrast(int value)
{
    if (d_func()->imageControl)
        d_func()->imageControl->setProcessingParameter(QCameraImageProcessingControl::Contrast,
                                                       QVariant(value));
}

/*!
    Returns the saturation adjustment value.
*/
int QCameraImageProcessing::saturation() const
{
    QVariant value;

    if (d_func()->imageControl)
        value = d_func()->imageControl->processingParameter(QCameraImageProcessingControl::Saturation);

    return value.toInt();
}

/*!
    Sets the saturation adjustment value to \a value.

    Valid saturation values range between -100 and 100, with a default of 0.
*/

void QCameraImageProcessing::setSaturation(int value)
{
    if (d_func()->imageControl)
        d_func()->imageControl->setProcessingParameter(QCameraImageProcessingControl::Saturation,
                                                       QVariant(value));
}

/*!
    Identifies if sharpening is supported.

    Returns true if sharpening is supported; and false if it is not.
*/
bool QCameraImageProcessing::isSharpeningSupported() const
{
    if (d_func()->imageControl)
        return d_func()->imageControl->isProcessingParameterSupported(QCameraImageProcessingControl::Sharpening);
    else
        return false;
}

/*!
    Returns the sharpening level.

    This may be \c DefaultSharpening if no particular sharpening level has been applied.

*/
int QCameraImageProcessing::sharpeningLevel() const
{
    QVariant value;

    if (d_func()->imageControl)
        value = d_func()->imageControl->processingParameter(QCameraImageProcessingControl::Sharpening);

    if (value.isNull())
        return DefaultSharpening;
    else
        return value.toInt();
}

/*!
    Sets the sharpening \a level.

    If \c DefaultSharpening is supplied, the camera will decide what sharpening
    to perform.  Otherwise a level of 0 will disable sharpening, and a level of 100
    corresponds to maximum sharpening applied.

*/

void QCameraImageProcessing::setSharpeningLevel(int level)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setProcessingParameter(QCameraImageProcessingControl::Sharpening,
                                                level == DefaultSharpening ? QVariant() : QVariant(level));
}

/*!
    Returns true if denoising is supported.
*/
bool QCameraImageProcessing::isDenoisingSupported() const
{
    if (d_func()->imageControl)
        return d_func()->imageControl->isProcessingParameterSupported(QCameraImageProcessingControl::Denoising);
    else
        return false;
}

/*!
    Returns the denoising level.  This may be \c DefaultDenoising if no
    particular value has been set.

*/
int QCameraImageProcessing::denoisingLevel() const
{
    QVariant value;

    if (d_func()->imageControl)
        value = d_func()->imageControl->processingParameter(QCameraImageProcessingControl::Denoising);

    if (value.isNull())
        return DefaultDenoising;
    else
        return value.toInt();
}

/*!
    Sets the denoising \a level.

    If \c DefaultDenoising is supplied, the camera will decide what denoising
    to perform.  Otherwise a level of 0 will disable denoising, and a level of 100
    corresponds to maximum denoising applied.

*/
void QCameraImageProcessing::setDenoisingLevel(int level)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setProcessingParameter(QCameraImageProcessingControl::Denoising,
                                                level == DefaultDenoising ? QVariant() : QVariant(level));
}


/*!
    \enum QCameraImageProcessing::WhiteBalanceMode

    \value WhiteBalanceManual       Manual white balance. In this mode the white balance should be set with
                                    setManualWhiteBalance()
    \value WhiteBalanceAuto         Auto white balance mode.
    \value WhiteBalanceSunlight     Sunlight white balance mode.
    \value WhiteBalanceCloudy       Cloudy white balance mode.
    \value WhiteBalanceShade        Shade white balance mode.
    \value WhiteBalanceTungsten     Tungsten (incandescent) white balance mode.
    \value WhiteBalanceFluorescent  Fluorescent white balance mode.
    \value WhiteBalanceFlash        Flash white balance mode.
    \value WhiteBalanceSunset       Sunset white balance mode.
    \value WhiteBalanceVendor       Vendor defined white balance mode.
*/

#include "moc_qcameraimageprocessing.cpp"
QT_END_NAMESPACE
