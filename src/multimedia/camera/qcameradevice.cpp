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

QCameraFormat::QCameraFormat() noexcept = default;

QCameraFormat::QCameraFormat(const QCameraFormat &other) noexcept = default;

QCameraFormat &QCameraFormat::operator=(const QCameraFormat &other) noexcept = default;

QCameraFormat::~QCameraFormat() = default;

QVideoFrameFormat::PixelFormat QCameraFormat::pixelFormat() const noexcept
{
    return d ? d->pixelFormat : QVideoFrameFormat::Format_Invalid;
}

QSize QCameraFormat::resolution() const noexcept
{
    return d ? d->resolution : QSize();
}

float QCameraFormat::minFrameRate() const noexcept
{
    return d ? d->minFrameRate : 0;
}

float QCameraFormat::maxFrameRate() const noexcept
{
    return d ? d->maxFrameRate : 0;
}


QCameraFormat::QCameraFormat(QCameraFormatPrivate *p)
    : d(p)
{
}

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
    \class QCameraDevice
    \brief The QCameraDevice class provides general information about camera devices.
    \since 5.3
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    QCameraDevice lets you query for camera devices that are currently available on the system.

    The static functions defaultCamera() and availableCameras() provide you a list of all
    available cameras.

    This example prints the name of all available cameras:

    \snippet multimedia-snippets/camera.cpp Camera listing

    A QCameraDevice can be used to construct a QCamera. The following example instantiates a QCamera
    whose camera device is named 'mycamera':

    \snippet multimedia-snippets/camera.cpp Camera selection

    You can also use QCameraDevice to get general information about a camera device such as
    description, physical position on the system, or camera sensor orientation.

    \snippet multimedia-snippets/camera.cpp Camera info

    \sa QCamera
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
    Returns the device id of the camera

    This is a unique ID to identify the camera and may not be human-readable.
*/
QByteArray QCameraDevice::id() const
{
    return d ? d->id : QByteArray();
}

bool QCameraDevice::isDefault() const
{
    return d ? d->isDefault : false;
}

/*!
    Returns the human-readable description of the camera.
*/
QString QCameraDevice::description() const
{
    return d ? d->description : QString();
}

/*!
    Returns the physical position of the camera on the hardware system.
*/
QCameraDevice::Position QCameraDevice::position() const
{
    return d ? d->position : QCameraDevice::UnspecifiedPosition;
}

QList<QSize> QCameraDevice::photoResolutions() const
{
    return d->photoResolutions;
}

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
