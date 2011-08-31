/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <qcameraimagecapturecontrol.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraImageCaptureControl

    \brief The QCameraImageCaptureControl class provides a control interface
    for image capture services.

    \inmodule QtMultimediaKit
    \ingroup multimedia-serv
    \since 1.1



    The interface name of QCameraImageCaptureControl is \c com.nokia.Qt.QCameraImageCaptureControl/1.0 as
    defined in QCameraImageCaptureControl_iid.


    \sa QMediaService::requestControl()
*/

/*!
    \macro QCameraImageCaptureControl_iid

    \c com.nokia.Qt.QCameraImageCaptureControl/1.0

    Defines the interface name of the QCameraImageCaptureControl class.

    \relates QCameraImageCaptureControl
*/

/*!
    Constructs a new image capture control object with the given \a parent
*/
QCameraImageCaptureControl::QCameraImageCaptureControl(QObject *parent)
    :QMediaControl(parent)
{
}

/*!
    Destroys an image capture control.
*/
QCameraImageCaptureControl::~QCameraImageCaptureControl()
{
}

/*!
    \fn QCameraImageCaptureControl::isReadyForCapture() const

    Identifies if a capture control is ready to perform a capture
    immediately (all the resources necessary for image capture are allocated,
    hardware initialized, flash is charged, etc).

    Returns true if the camera is ready for capture; and false if it is not.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::readyForCaptureChanged(bool ready)

    Signals that a capture control's \a ready state has changed.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::capture(const QString &fileName)

    Initiates the capture of an image to \a fileName.
    The \a fileName can be relative or empty,
    in this case the service should use the system specific place
    and file naming scheme.

    Returns the capture request id number, which is used later
    with imageExposed(), imageCaptured() and imageSaved() signals.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::cancelCapture()

    Cancel pending capture requests.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::imageExposed(int requestId)

    Signals that an image with it \a requestId
    has just been exposed.
    This signal can be used for the shutter sound or other indicaton.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::imageCaptured(int requestId, const QImage &preview)

    Signals that an image with it \a requestId
    has been captured and a \a preview is available.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::imageMetadataAvailable(int id, QtMultimediaKit::MetaData key, const QVariant &value)

    Signals that a metadata for an image with request \a id is available.
    This signal is emitted for metadata \a value with a \a key listed in QtMultimediaKit::MetaData enum.

    This signal should be emitted between imageExposed and imageSaved signals.
    \since 1.2
*/

/*!
    \fn QCameraImageCaptureControl::imageMetadataAvailable(int id, const QString &key, const QVariant &value)

    Signals that a metadata for an image with request \a id is available.
    This signal is emitted for extended metadata \a value with a \a key not listed in QtMultimediaKit::MetaData enum.

    This signal should be emitted between imageExposed and imageSaved signals.
    \since 1.2
*/

/*!
    \fn QCameraImageCaptureControl::imageAvailable(int requestId, const QVideoFrame &buffer)

    Signals that a captured \a buffer with a \a requestId is available.
    \since 1.2
*/

/*!
    \fn QCameraImageCaptureControl::imageSaved(int requestId, const QString &fileName)

    Signals that a captured image with a \a requestId has been saved
    to \a fileName.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::driveMode() const

    Returns the current camera drive mode.
    \since 1.1
*/

/*!
    \fn QCameraImageCaptureControl::setDriveMode(QCameraImageCapture::DriveMode mode)

    Sets the current camera drive \a mode.
    \since 1.1
*/


/*!
    \fn QCameraImageCaptureControl::error(int id, int error, const QString &errorString)

    Signals the capture request \a id failed with \a error code and message \a errorString.

    \since 1.1
    \sa QCameraImageCapture::Error
*/


#include "moc_qcameraimagecapturecontrol.cpp"
QT_END_NAMESPACE

