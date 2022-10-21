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

#include "qplatformimagecapture_p.h"
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPlatformImageCapture
    \obsolete

    \brief The QPlatformImageCapture class provides a control interface
    for image capture services.

    \inmodule QtMultimedia
    \ingroup multimedia_control

*/

QString QPlatformImageCapture::msgCameraNotReady()
{
    return QImageCapture::tr("Camera is not ready.");
}

QString QPlatformImageCapture::msgImageCaptureNotSet()
{
    return QImageCapture::tr("No instance of QImageCapture set on QMediaCaptureSession.");
}

/*!
    Constructs a new image capture control object with the given \a parent
*/
QPlatformImageCapture::QPlatformImageCapture(QImageCapture *parent)
    : QObject(parent),
    m_imageCapture(parent)
{
}

/*!
    \fn QPlatformImageCapture::isReadyForCapture() const

    Identifies if a capture control is ready to perform a capture
    immediately (all the resources necessary for image capture are allocated,
    hardware initialized, flash is charged, etc).

    Returns true if the camera is ready for capture; and false if it is not.

    It's permissible to call capture() while the camera status is QCamera::ActiveStatus
    regardless of isReadyForCapture property value.
    If camera is not ready to capture image immediately,
    the capture request is queued with all the related camera settings
    to be executed as soon as possible.
*/

/*!
    \fn QPlatformImageCapture::readyForCaptureChanged(bool ready)

    Signals that a capture control's \a ready state has changed.
*/

/*!
    \fn QPlatformImageCapture::capture(const QString &fileName)

    Initiates the capture of an image to \a fileName.
    The \a fileName can be relative or empty,
    in this case the service should use the system specific place
    and file naming scheme.

    The Camera service should save all the capture parameters
    like exposure settings or image processing parameters,
    so changes to camera parameters after capture() is called
    do not affect previous capture requests.

    Returns the capture request id number, which is used later
    with imageExposed(), imageCaptured() and imageSaved() signals.
*/

/*!
    \fn QPlatformImageCapture::imageExposed(int requestId)

    Signals that an image with it \a requestId
    has just been exposed.
    This signal can be used for the shutter sound or other indicaton.
*/

/*!
    \fn QPlatformImageCapture::imageCaptured(int requestId, const QImage &preview)

    Signals that an image with it \a requestId
    has been captured and a \a preview is available.
*/

/*!
    \fn QPlatformImageCapture::imageMetadataAvailable(int id, const QMediaMetaData &metaData)

    Signals that a metadata for an image with request \a id is available.

    This signal should be emitted between imageExposed and imageSaved signals.
*/

/*!
    \fn QPlatformImageCapture::imageAvailable(int requestId, const QVideoFrame &buffer)

    Signals that a captured \a buffer with a \a requestId is available.
*/

/*!
    \fn QPlatformImageCapture::imageSaved(int requestId, const QString &fileName)

    Signals that a captured image with a \a requestId has been saved
    to \a fileName.
*/

/*!
    \fn QPlatformImageCapture::imageSettings() const

    Returns the currently used image encoder settings.

    The returned value may be different than passed to setImageSettings()
    if the settings contains defaulted or undefined parameters.
*/

/*!
    \fn QPlatformImageCapture::setImageSettings(const QImageEncoderSettings &settings)

    Sets the selected image encoder \a settings.
*/

/*!
    \fn QPlatformImageCapture::error(int id, int error, const QString &errorString)

    Signals the capture request \a id failed with \a error code and message \a errorString.

    \sa QImageCapture::Error
*/


QT_END_NAMESPACE

#include "moc_qplatformimagecapture_p.cpp"
