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

#include "qcamerainfo_p.h"

#include "qcamera_p.h"

QT_BEGIN_NAMESPACE

QCameraFormat::QCameraFormat(const QCameraFormat &other) = default;

QCameraFormat &QCameraFormat::operator=(const QCameraFormat &other) = default;

QCameraFormat::~QCameraFormat() = default;

QVideoSurfaceFormat::PixelFormat QCameraFormat::pixelFormat() const
{
    return d->pixelFormat;
}

QSize QCameraFormat::resolution() const
{
    return d->resolution;
}

float QCameraFormat::minFrameRate() const
{
    return d->minFrameRate;
}

float QCameraFormat::maxFrameRate() const
{
    return d->maxFrameRate;
}

QCameraFormat::QCameraFormat(QCameraFormatPrivate *p)
    : d(p)
{
}

/*!
    \class QCameraInfo
    \brief The QCameraInfo class provides general information about camera devices.
    \since 5.3
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera

    QCameraInfo lets you query for camera devices that are currently available on the system.

    The static functions defaultCamera() and availableCameras() provide you a list of all
    available cameras.

    This example prints the name of all available cameras:

    \snippet multimedia-snippets/camera.cpp Camera listing

    A QCameraInfo can be used to construct a QCamera. The following example instantiates a QCamera
    whose camera device is named 'mycamera':

    \snippet multimedia-snippets/camera.cpp Camera selection

    You can also use QCameraInfo to get general information about a camera device such as
    description, physical position on the system, or camera sensor orientation.

    \snippet multimedia-snippets/camera.cpp Camera info

    \sa QCamera
*/

QCameraInfo::QCameraInfo() = default;

/*!
    Constructs a copy of \a other.
*/
QCameraInfo::QCameraInfo(const QCameraInfo &other) = default;

/*!
    Destroys the QCameraInfo.
*/
QCameraInfo::~QCameraInfo() = default;

/*!
    Returns true if this QCameraInfo is equal to \a other.
*/
bool QCameraInfo::operator==(const QCameraInfo &other) const
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
    Returns true if this QCameraInfo is null or invalid.
*/
bool QCameraInfo::isNull() const
{
    return !d;
}

/*!
    Returns the device id of the camera

    This is a unique ID to identify the camera and may not be human-readable.
*/
QByteArray QCameraInfo::id() const
{
    return d ? d->id : QByteArray();
}

bool QCameraInfo::isDefault() const
{
    return d ? d->isDefault : false;
}

/*!
    Returns the human-readable description of the camera.
*/
QString QCameraInfo::description() const
{
    return d ? d->description : QString();
}

/*!
    Returns the physical position of the camera on the hardware system.
*/
QCameraInfo::Position QCameraInfo::position() const
{
    return d ? d->position : QCameraInfo::UnspecifiedPosition;
}

QList<QSize> QCameraInfo::photoResolutions() const
{
    return d->photoResolutions;
}

/*!
    Returns the video formats supported by the camera.
*/
QList<QCameraFormat> QCameraInfo::videoFormats() const
{
    return d ? d->videoFormats : QList<QCameraFormat>{};
}

QCameraInfo::QCameraInfo(QCameraInfoPrivate *p)
    : d(p)
{}

/*!
    Sets the QCameraInfo object to be equal to \a other.
*/
QCameraInfo& QCameraInfo::operator=(const QCameraInfo& other) = default;

/*!
    \fn QCameraInfo::operator!=(const QCameraInfo &other) const

    Returns true if this QCameraInfo is different from \a other.
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QCameraInfo &camera)
{
    d.maybeSpace() << QStringLiteral("QCameraInfo(name=%1, position=%2, orientation=%3)")
                          .arg(camera.description())
                          .arg(QString::fromLatin1(QCamera::staticMetaObject.enumerator(QCamera::staticMetaObject.indexOfEnumerator("Position"))
                               .valueToKey(camera.position())));
    return d.space();
}
#endif

QT_END_NAMESPACE
