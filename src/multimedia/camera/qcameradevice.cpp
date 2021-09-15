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

#include "qcameradevice_p.h"

#include "qcamera_p.h"

QT_BEGIN_NAMESPACE


/*!
    \class QCameraFormat
    \since 6.2
    \brief The QCameraFormat class describes a video format supported by a camera device.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    QCameraFormat represents a certain video format supported by a camera device.

    The format is a combination of a
    \l{QVideoFrameFormat::PixelFormat}{pixel format}, resolution and a range of frame
    rates.

    QCameraFormat objects can be queried from QCameraDevice to inspect the set of
    supported video formats.

    \sa QCameraDevice, QCamera
*/

/*!
    \qmlbasictype cameraFormat
    \inqmlmodule QtMultimedia
    \since 6.2
    //! \instantiates QCameraFormat
    \brief Describes a video format supported by a camera device.
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml

    cameraFormat represents a certain video format supported by a camera device.

    The format is a combination of a
    \l{pixel format}{QVideoFrameFormat::PixelFormat}, resolution and a range of frame
    rates.

    cameraFormat objects can be queried from \l cameraDevice to inspect the set of
    supported video formats.

    \sa cameraDevice, Camera
*/

/*!
    Constructs a null camera format.

    \sa isNull()
*/
QCameraFormat::QCameraFormat() noexcept = default;

/*!
    Copy constructs a camera format from the \a other format.
*/
QCameraFormat::QCameraFormat(const QCameraFormat &other) noexcept = default;

/*!
    Assign \a other to this.
*/
QCameraFormat &QCameraFormat::operator=(const QCameraFormat &other) noexcept = default;

/*!
    Destructs the camera format object.
*/
QCameraFormat::~QCameraFormat() = default;

/*! \fn bool QCameraFormat::isNull() const noexcept

    Returns true if this is a default constructed QCameraFormat.
*/

/*!
    \qmlproperty enumeration QtMultimedia::CameraFormat::pixelFormat

    Returns the pixel format.

    Most commonly this is either QVideoFrameFormat::Format_Jpeg or QVideoFrameFormat::Format_YUVY
    but other formats could also be supported by the camera.

    \sa QVideoFrameFormat::PixelFormat
*/

/*!
    Returns the pixel format.

    Most commonly this is either QVideoFrameFormat::Format_Jpeg or QVideoFrameFormat::Format_YUVY
    but other formats could also be supported by the camera.

    \sa QVideoFrameFormat::PixelFormat
*/
QVideoFrameFormat::PixelFormat QCameraFormat::pixelFormat() const noexcept
{
    return d ? d->pixelFormat : QVideoFrameFormat::Format_Invalid;
}

/*!
    \qmlproperty size QtMultimedia::CameraFormat::resolution

    Returns the resolution.
*/

/*!
    Returns the resolution.
*/
QSize QCameraFormat::resolution() const noexcept
{
    return d ? d->resolution : QSize();
}

/*!
    \qmlproperty real QtMultimedia::CameraFormat::minFrameRate

    Returns the lowest frame rate defined by this format.
*/

/*!
    Returns the lowest frame rate defined by this format.
*/
float QCameraFormat::minFrameRate() const noexcept
{
    return d ? d->minFrameRate : 0;
}

/*!
    \qmlproperty real QtMultimedia::CameraFormat::maxFrameRate

    Returns the highest frame rate defined by this format.

    In 6.2, the camera will always try to use the maximum frame rate supported by a
    certain video format.
*/

/*!
    Returns the highest frame rate defined by this format.

    In 6.2, the camera will always try to use the highest frame rate supported by a
    certain video format.
*/
float QCameraFormat::maxFrameRate() const noexcept
{
    return d ? d->maxFrameRate : 0;
}

/*!
    \internal
*/
QCameraFormat::QCameraFormat(QCameraFormatPrivate *p)
    : d(p)
{
}

/*!
    Returns \c true if the \a other format is equal to this camera format, otherwise \c false.
*/
bool QCameraFormat::operator==(const QCameraFormat &other) const
{
    if (d == other.d)
        return true;
    if (!d || !other.d)
        return false;
    return d->pixelFormat == other.d->pixelFormat &&
           d->minFrameRate == other.d->minFrameRate &&
           d->maxFrameRate == other.d->maxFrameRate &&
           d->resolution == other.d->resolution;
}

/*!
    \fn bool QCameraFormat::operator!=(const QCameraFormat &other) const

    Returns \c false if the \a other format is equal to this camera format, otherwise \c true.
*/

/*!
    \class QCameraDevice
    \brief The QCameraDevice class provides general information about camera devices.
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    QCameraDevice represents a physical camera device and its properties.

    You can discover what cameras are available on a system using the
    availableCameras() and defaultCamera() functions. These are contained within
    QtMultimedia::MediaDevices.

    This example prints the name of all available cameras:

    \snippet multimedia-snippets/camera.cpp Camera listing

    A QCameraDevice can be used to construct a QCamera. The following example
    instantiates a QCamera whose camera device is named \c {mycamera}:

    \snippet multimedia-snippets/camera.cpp Camera selection

    You can also use QCameraDevice to get general information about a camera
    device such as description, physical position on the system, or camera sensor
    orientation.

    \snippet multimedia-snippets/camera.cpp Camera info

    \sa QCamera
*/

/*!
    \qmlbasictype cameraDevice
    \inqmlmodule QtMultimedia
    \since 6.2
    //! \instantiates QCameraDevice
    \brief Describes a camera device.
    \ingroup multimedia_qml
    \ingroup multimedia_video_qml

    The cameraDevice value type describes the properties of a camera device that
    is connected to the system.

    The list of camera devices can be queried from the \l{MediaDevices}
    type. To select a certain camera device set it as the device
    on \l{Camera}.

    \qml
    CaptureSession {
        camera: Camera {
            cameraDevice: mediaDevices.defaultVideoInput
        }
    }
    MediaDevices {
        id: mediaDevices
    }
    \endqml
*/

/*!
  Constructs a null camera device
*/
QCameraDevice::QCameraDevice() = default;

/*!
    Constructs a copy of \a other.
*/
QCameraDevice::QCameraDevice(const QCameraDevice &other) = default;

/*!
    Destroys the QCameraDevice.
*/
QCameraDevice::~QCameraDevice() = default;

/*!
    Returns true if this QCameraDevice is equal to \a other.
*/
bool QCameraDevice::operator==(const QCameraDevice &other) const
{
    if (d == other.d)
        return true;

    if (!d || ! other.d)
        return false;

    return (d->id == other.d->id
            && d->description == other.d->description
            && d->position == other.d->position);
}

/*!
    Returns true if this QCameraDevice is null or invalid.
*/
bool QCameraDevice::isNull() const
{
    return !d;
}

/*!
    \qmlmethod string QtMultimedia::cameraDevice::id

    Returns the device id of the camera

    This is a unique ID to identify the camera and may not be human-readable.
*/

/*!
    Returns the device id of the camera

    This is a unique ID to identify the camera and may not be human-readable.
*/
QByteArray QCameraDevice::id() const
{
    return d ? d->id : QByteArray();
}

/*!
    \qmlmethod bool QtMultimedia::cameraDevice::isDefault

    Returns true if this is the default camera device.
*/

/*!
    Returns true if this is the default camera device.
*/
bool QCameraDevice::isDefault() const
{
    return d ? d->isDefault : false;
}

/*!
    \qmlmethod string QtMultimedia::cameraDevice::description

    Returns a human readable name of the camera.

    Use this string to present the device to the user.
*/

/*!
    Returns the human-readable description of the camera.

    Use this string to present the device to the user.
*/
QString QCameraDevice::description() const
{
    return d ? d->description : QString();
}

/*!
    \enum QCameraDevice::Position

    This enum specifies the physical position of the camera on the system hardware.

    \value UnspecifiedPosition  The camera position is unspecified or unknown.
    \value BackFace  The camera is on the back face of the system hardware. For example on a
           mobile device, it means it is on the opposite side to that of the screen.
    \value FrontFace  The camera is on the front face of the system hardware. For example on a
           mobile device, it means it is on the same side as that of the screen.

    \sa position()
*/

/*!
    \qmlmethod enumeration QtMultimedia::cameraDevice::position

    Returns the physical position of the camera on the hardware system.

    The returned value can be one of the following:

    \table
    \header
        \li Property value
        \li Description
    \row \li CameraDevice.UnspecifiedPosition
        \li The physical positition of the camera is unspecified.
    \row \li CameraDevice.FrontFace
        \li The camera is on the front of the hardware, facing towards the user.
    \row \li CameraDevice.BackFace
        \li The camera is on the back of the hardware, facing away from the user.
    \endtable
*/

/*!
    Returns the physical position of the camera on the hardware system.
*/
QCameraDevice::Position QCameraDevice::position() const
{
    return d ? d->position : QCameraDevice::UnspecifiedPosition;
}

/*!
    Returns a list of resolutions that the camera can use to
    capture still images.

    \sa QImageCapture
 */
QList<QSize> QCameraDevice::photoResolutions() const
{
    return d ? d->photoResolutions : QList<QSize>{};
}

/*!
    \qmlproperty CameraFormat QtMultiMedia::CameraDevice::videoFormats

    Holds the video formats supported by the camera.
*/

/*!
    Returns the video formats supported by the camera.
*/
QList<QCameraFormat> QCameraDevice::videoFormats() const
{
    return d ? d->videoFormats : QList<QCameraFormat>{};
}

QCameraDevice::QCameraDevice(QCameraDevicePrivate *p)
    : d(p)
{}

/*!
    Sets the QCameraDevice object to be equal to \a other.
*/
QCameraDevice& QCameraDevice::operator=(const QCameraDevice& other) = default;

/*!
    \fn QCameraDevice::operator!=(const QCameraDevice &other) const

    Returns true if this QCameraDevice is different from \a other.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QCameraDevice &camera)
{
    d.maybeSpace() << QStringLiteral("QCameraDevice(name=%1, position=%2, orientation=%3)")
                          .arg(camera.description())
                          .arg(QString::fromLatin1(QCamera::staticMetaObject.enumerator(QCamera::staticMetaObject.indexOfEnumerator("Position"))
                               .valueToKey(camera.position())));
    return d.space();
}
#endif

QT_END_NAMESPACE
