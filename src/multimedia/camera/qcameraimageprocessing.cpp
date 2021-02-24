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

#include "qcameraimageprocessing.h"
#include "private/qobject_p.h"

#include <private/qplatformcamera_p.h>
#include <private/qplatformcameraimageprocessing_p.h>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraImageProcessing

    \brief The QCameraImageProcessing class provides an interface for
    image processing related camera settings.

    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    After capturing the data for a camera frame, the camera hardware and
    software performs various image processing tasks to produce a final
    image.  This includes compensating for ambient light color, reducing
    noise, as well as making some other adjustments to the image.

    You can retrieve this class from an instance of a \l QCamera object.

    For example, you can set the white balance (or color temperature) used
    for processing images:

    \snippet multimedia-snippets/camera.cpp Camera image whitebalance

    In some cases changing these settings may result in a longer delay
    before an image is ready.

    For more information on image processing of camera frames, see \l {camera_image_processing}{Camera Image Processing}.

    \sa QPlatformCameraImageProcessing
*/

class QCameraImageProcessingPrivate : public QObjectPrivate
{
public:
    void init(QPlatformCamera *cameraControl);

    QCamera *camera;
    QPlatformCameraImageProcessing *imageControl;
};


void QCameraImageProcessingPrivate::init(QPlatformCamera *cameraControl)
{
    imageControl = cameraControl->imageProcessingControl();
}

/*!
    Construct a QCameraImageProcessing for \a camera.
*/

QCameraImageProcessing::QCameraImageProcessing(QCamera *camera, QPlatformCamera *cameraControl)
    : QObject(*new QCameraImageProcessingPrivate, camera)
{
    Q_D(QCameraImageProcessing);
    d->camera = camera;
    d->init(cameraControl);
}


/*!
    Destroys the camera focus object.
*/

QCameraImageProcessing::~QCameraImageProcessing() = default;


/*!
    Returns true if image processing related settings are supported by this camera.
*/
bool QCameraImageProcessing::isAvailable() const
{
    return d_func()->imageControl;
}


/*!
    Returns the white balance mode being used.
*/

QCameraImageProcessing::WhiteBalanceMode QCameraImageProcessing::whiteBalanceMode() const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return WhiteBalanceAuto;
    return d->imageControl->parameter(QPlatformCameraImageProcessing::WhiteBalancePreset)
            .value<QCameraImageProcessing::WhiteBalanceMode>();
}

/*!
    Sets the white balance to \a mode.
*/

void QCameraImageProcessing::setWhiteBalanceMode(QCameraImageProcessing::WhiteBalanceMode mode)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setParameter(
                QPlatformCameraImageProcessing::WhiteBalancePreset,
                QVariant::fromValue<QCameraImageProcessing::WhiteBalanceMode>(mode));
}

/*!
    Returns true if the white balance \a mode is supported.
*/

bool QCameraImageProcessing::isWhiteBalanceModeSupported(QCameraImageProcessing::WhiteBalanceMode mode) const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return false;
    return d->imageControl->isParameterValueSupported(
                QPlatformCameraImageProcessing::WhiteBalancePreset,
                QVariant::fromValue<QCameraImageProcessing::WhiteBalanceMode>(mode));

}

/*!
    Returns the current color temperature if the
    current white balance mode is \c WhiteBalanceManual.  For other modes the
    return value is undefined.
*/

qreal QCameraImageProcessing::manualWhiteBalance() const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return 0.;

    return d->imageControl->parameter(QPlatformCameraImageProcessing::ColorTemperature).toReal();
}

/*!
    Sets manual white balance to \a colorTemperature.  This is used
    when whiteBalanceMode() is set to \c WhiteBalanceManual.  The units are Kelvin.
*/

void QCameraImageProcessing::setManualWhiteBalance(qreal colorTemperature)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setParameter(
                QPlatformCameraImageProcessing::ColorTemperature,
                QVariant(colorTemperature));
}

/*!
    Returns the brightness adjustment setting.
 */
qreal QCameraImageProcessing::brightness() const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return 0.;
    return d->imageControl->parameter(QPlatformCameraImageProcessing::BrightnessAdjustment).toReal();
}

/*!
    Set the brightness adjustment to \a value.

    Valid brightness adjustment values range between -1.0 and 1.0, with a default of 0.
 */
void QCameraImageProcessing::setBrightness(qreal value)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setParameter(QPlatformCameraImageProcessing::BrightnessAdjustment,
                                         QVariant(value));
}

/*!
    Returns the contrast adjustment setting.
*/
qreal QCameraImageProcessing::contrast() const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return 0.;
    return d->imageControl->parameter(QPlatformCameraImageProcessing::ContrastAdjustment).toReal();
}

/*!
    Set the contrast adjustment to \a value.

    Valid contrast adjustment values range between -1.0 and 1.0, with a default of 0.
*/
void QCameraImageProcessing::setContrast(qreal value)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setParameter(QPlatformCameraImageProcessing::ContrastAdjustment,
                                         QVariant(value));
}

/*!
    Returns the saturation adjustment value.
*/
qreal QCameraImageProcessing::saturation() const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return 0.;
    return d->imageControl->parameter(QPlatformCameraImageProcessing::SaturationAdjustment).toReal();
}

/*!
    Sets the saturation adjustment value to \a value.

    Valid saturation values range between -1.0 and 1.0, with a default of 0.
*/

void QCameraImageProcessing::setSaturation(qreal value)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setParameter(QPlatformCameraImageProcessing::SaturationAdjustment,
                                         QVariant(value));
}

/*!
    \enum QCameraImageProcessing::WhiteBalanceMode

    \value WhiteBalanceAuto         Auto white balance mode.
    \value WhiteBalanceManual       Manual white balance. In this mode the white balance should be set with
                                    setManualWhiteBalance()
    \value WhiteBalanceSunlight     Sunlight white balance mode.
    \value WhiteBalanceCloudy       Cloudy white balance mode.
    \value WhiteBalanceShade        Shade white balance mode.
    \value WhiteBalanceTungsten     Tungsten (incandescent) white balance mode.
    \value WhiteBalanceFluorescent  Fluorescent white balance mode.
    \value WhiteBalanceFlash        Flash white balance mode.
    \value WhiteBalanceSunset       Sunset white balance mode.
    \value WhiteBalanceVendor       Base value for vendor defined white balance modes.
*/

/*!
    \enum QCameraImageProcessing::ColorFilter

    \value ColorFilterNone               No filter is applied to images.
    \value ColorFilterGrayscale          A grayscale filter.
    \value ColorFilterNegative           A negative filter.
    \value ColorFilterSolarize           A solarize filter.
    \value ColorFilterSepia              A sepia filter.
    \value ColorFilterPosterize          A posterize filter.
    \value ColorFilterWhiteboard         A whiteboard filter.
    \value ColorFilterBlackboard         A blackboard filter.
    \value ColorFilterAqua               An aqua filter.
    \value ColorFilterVendor             The base value for vendor defined filters.

    \since 5.5
*/

/*!
    Returns the color filter which will be applied to image data captured by the camera.

    \since 5.5
*/

QCameraImageProcessing::ColorFilter QCameraImageProcessing::colorFilter() const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return ColorFilterNone;
    return d->imageControl->parameter(QPlatformCameraImageProcessing::ColorFilter)
            .value<QCameraImageProcessing::ColorFilter>();
}


/*!
    Sets the color \a filter which will be applied to image data captured by the camera.

    \since 5.5
*/

void QCameraImageProcessing::setColorFilter(QCameraImageProcessing::ColorFilter filter)
{
    Q_D(QCameraImageProcessing);
    if (d->imageControl)
        d->imageControl->setParameter(
                    QPlatformCameraImageProcessing::ColorFilter,
                    QVariant::fromValue<QCameraImageProcessing::ColorFilter>(filter));
}

/*!
    Returns true if a color \a filter is supported.

    \since 5.5
*/

bool QCameraImageProcessing::isColorFilterSupported(QCameraImageProcessing::ColorFilter filter) const
{
    Q_D(const QCameraImageProcessing);
    if (!d->imageControl)
        return false;
    return d->imageControl->isParameterValueSupported(
                QPlatformCameraImageProcessing::ColorFilter,
                QVariant::fromValue<QCameraImageProcessing::ColorFilter>(filter));

}

QT_END_NAMESPACE

#include "moc_qcameraimageprocessing.cpp"
