/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativecamera_p.h"
#include "qdeclarativecameracapture_p.h"
#include "qdeclarativecamerapreviewprovider_p.h"

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

QDeclarativeCameraCapture::QDeclarativeCameraCapture(QCamera *camera, QObject *parent) :
    QObject(parent),
    m_camera(camera)
{
    m_capture = new QCameraImageCapture(camera, this);

    connect(m_capture, SIGNAL(readyForCaptureChanged(bool)), this, SIGNAL(readyForCaptureChanged(bool)));
    connect(m_capture, SIGNAL(imageExposed(int)), this, SIGNAL(imageExposed()));
    connect(m_capture, SIGNAL(imageCaptured(int,QImage)), this, SLOT(_q_imageCaptured(int, QImage)));
    connect(m_capture, SIGNAL(imageMetadataAvailable(int,QString,QVariant)), this,
            SLOT(_q_imageMetadataAvailable(int,QString,QVariant)));
    connect(m_capture, SIGNAL(imageSaved(int,QString)), this, SLOT(_q_imageSaved(int, QString)));
    connect(m_capture, SIGNAL(error(int,QCameraImageCapture::Error,QString)),
            this, SLOT(_q_captureFailed(int,QCameraImageCapture::Error,QString)));
}

QDeclarativeCameraCapture::~QDeclarativeCameraCapture()
{
}

/*!
    \qmlproperty string CameraCapture::ready
    \property QDeclarativeCameraCapture::ready

    Indicates camera is ready to capture photo.
*/
bool QDeclarativeCameraCapture::isReadyForCapture() const
{
    return m_capture->isReadyForCapture();
}

/*!
    \qmlmethod CameraCapture::capture()
    \fn QDeclarativeCameraCapture::capture()

    Start image capture.  The \l onImageCaptured() and \l onImageSaved() signals will
    be emitted when the capture is complete.
*/
void QDeclarativeCameraCapture::capture()
{
    m_capture->capture();
}

/*!
    \qmlmethod CameraCapture::captureToLocation()
    \fn QDeclarativeCameraCapture::captureToLocation()

    Start image capture to specified \a location.  The \l onImageCaptured() and \l onImageSaved() signals will
    be emitted when the capture is complete.
*/
void QDeclarativeCameraCapture::captureToLocation(const QString &location)
{
    m_capture->capture(location);
}

/*!
    \qmlmethod CameraCapture::cancelCapture()
    \fn QDeclarativeCameraCapture::cancelCapture()

    Cancel pendig image capture requests.
*/

void QDeclarativeCameraCapture::cancelCapture()
{
    m_capture->cancelCapture();
}

/*!
    \qmlproperty string CameraCapture::capturedImagePath
    \property QDeclarativeCameraCapture::capturedImagePath

    The path to the captured image.
*/
QString QDeclarativeCameraCapture::capturedImagePath() const
{
    return m_capturedImagePath;
}

void QDeclarativeCameraCapture::_q_imageCaptured(int id, const QImage &preview)
{
    QString previewId = QString("preview_%1").arg(id);
    QDeclarativeCameraPreviewProvider::registerPreview(previewId, preview);

    emit imageCaptured(QLatin1String("image://camera/")+previewId);
}

void QDeclarativeCameraCapture::_q_imageSaved(int id, const QString &fileName)
{
    Q_UNUSED(id);
    m_capturedImagePath = fileName;
    emit imageSaved(fileName);
}

void QDeclarativeCameraCapture::_q_imageMetadataAvailable(int id, const QString &key, const QVariant &value)
{
    Q_UNUSED(id);
    emit imageMetadataAvailable(key, value);
}


void QDeclarativeCameraCapture::_q_captureFailed(int id, QCameraImageCapture::Error error, const QString &message)
{
    Q_UNUSED(id);
    Q_UNUSED(error);
    qWarning() << "QCameraImageCapture error:" << message;
    emit captureFailed(message);
}

/*!
    \qmlproperty size CameraCapture::resolution
    \property QDeclarativeCameraCapture::resolution

    The resolution to capture the image at.  If empty, the system will pick
    a good size.
*/

QSize QDeclarativeCameraCapture::resolution()
{
    return m_imageSettings.resolution();
}

void QDeclarativeCameraCapture::setResolution(const QSize &captureResolution)
{
    if (captureResolution != resolution()) {
        m_imageSettings.setResolution(captureResolution);
        m_capture->setEncodingSettings(m_imageSettings);
        emit resolutionChanged(captureResolution);
    }
}

QCameraImageCapture::Error QDeclarativeCameraCapture::error() const
{
    return m_capture->error();
}


/*!
    \qmlproperty size CameraCapture::errorString
    \property QDeclarativeCameraCapture::errorString

    The last capture related error message.
*/
QString QDeclarativeCameraCapture::errorString() const
{
    return m_capture->errorString();
}

void QDeclarativeCameraCapture::setMetadata(const QString &key, const QVariant &value)
{
    Q_UNUSED(key);
    Q_UNUSED(value);
    //m_capture->setExtendedMetaData(key, value);
}

/*!
    \qmlsignal CameraCapture::onCaptureFailed(message)
    \fn QDeclarativeCameraCapture::captureFailed(const QString &message)

    This handler is called when an error occurs during capture.  A descriptive message is available in \a message.
*/

/*!
    \qmlsignal CameraCapture::onImageCaptured(preview)
    \fn QDeclarativeCameraCapture::imageCaptured(const QString &preview)

    This handler is called when an image has been captured but not yet saved to the filesystem.  The \a preview
    parameter can be used as the URL supplied to an Image element.

    \sa onImageSaved
*/

/*!
    \qmlsignal CameraCapture::onImageSaved(path)
    \fn QDeclarativeCameraCapture::imageSaved(const QString &path)

    This handler is called after the image has been written to the filesystem.  The \a path is a local file path, not a URL.

    \sa onImageCaptured
*/


QT_END_NAMESPACE

#include "moc_qdeclarativecameracapture_p.cpp"
