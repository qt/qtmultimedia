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
#include <qcameraimagecapture.h>
#include <private/qplatformcameraimagecapture_p.h>
#include <qmediaencodersettings.h>
#include <qmediametadata.h>
#include <private/qplatformmediacapture_p.h>
#include <private/qplatformmediaintegration_p.h>
#include <private/qplatformmediaformatinfo_p.h>
#include <qmediacapturesession.h>

#include "private/qobject_p.h"
#include <qcamera.h>
#include <private/qplatformcamera_p.h>
#include <QtCore/qdebug.h>
#include <QtCore/qurl.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

/*!
    \class QCameraImageCapture
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_camera


    \brief The QCameraImageCapture class is used for the recording of media content.

    The QCameraImageCapture class is a high level images recording class.
    It's not intended to be used alone but for accessing the media
    recording functions of other media objects, like QCamera.

    \snippet multimedia-snippets/camera.cpp Camera

    \snippet multimedia-snippets/camera.cpp Camera keys

    \sa QCamera
*/

class QCameraImageCapturePrivate
{
    Q_DECLARE_PUBLIC(QCameraImageCapture)
public:
    QCamera *camera = nullptr;

    QMediaCaptureSession *captureSession = nullptr;
    QPlatformCameraImageCapture *control = nullptr;

    QCameraImageCapture::Error error = QCameraImageCapture::NoError;
    QString errorString;
    QMediaMetaData metaData;

    void _q_error(int id, int error, const QString &errorString);

    void unsetError() { error = QCameraImageCapture::NoError; errorString.clear(); }

    QCameraImageCapture *q_ptr;
};

void QCameraImageCapturePrivate::_q_error(int id, int error, const QString &errorString)
{
    Q_Q(QCameraImageCapture);

    this->error = QCameraImageCapture::Error(error);
    this->errorString = errorString;

    emit q->errorChanged();
    emit q->errorOccurred(id, this->error, errorString);
}

/*!
    Constructs a image capture object that can capture individual still images produced by a camera.

    Connect both an image capture object and a QCamera to a capture session to capture images.
*/

QCameraImageCapture::QCameraImageCapture(QObject *parent)
    : QObject(parent), d_ptr(new QCameraImageCapturePrivate)
{
    Q_D(QCameraImageCapture);
    d->q_ptr = this;
    d->control = QPlatformMediaIntegration::instance()->createImageCapture(this);
}

void QCameraImageCapture::setCaptureSession(QMediaCaptureSession *session)
{
    Q_D(QCameraImageCapture);
    d->captureSession = session;

    QPlatformMediaCaptureSession *platformSession = session ? session->platformSession() : nullptr;

    if (platformSession && d->control) {
        platformSession->setImageCapture(d->control);
    } else {
        return;
    }

    connect(d->control, SIGNAL(imageExposed(int)),
            this, SIGNAL(imageExposed(int)));
    connect(d->control, SIGNAL(imageCaptured(int,QImage)),
            this, SIGNAL(imageCaptured(int,QImage)));
    connect(d->control, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)),
            this, SIGNAL(imageMetadataAvailable(int,const QMediaMetaData&)));
    connect(d->control, SIGNAL(imageAvailable(int,QVideoFrame)),
            this, SIGNAL(imageAvailable(int,QVideoFrame)));
    connect(d->control, SIGNAL(imageSaved(int,QString)),
            this, SIGNAL(imageSaved(int,QString)));
    connect(d->control, SIGNAL(readyForCaptureChanged(bool)),
            this, SIGNAL(readyForCaptureChanged(bool)));
    connect(d->control, SIGNAL(error(int,int,QString)),
            this, SLOT(_q_error(int,int,QString)));
}

/*!
    Destroys images capture object.
*/

QCameraImageCapture::~QCameraImageCapture()
{
    if (d_ptr->captureSession) {
        d_ptr->captureSession->platformSession()->setImageCapture(nullptr);
        d_ptr->captureSession->setImageCapture(nullptr);
    }
    delete d_ptr;
}

/*!
    Returns true if the images capture service ready to use.
*/
bool QCameraImageCapture::isAvailable() const
{
    return d_func()->control != nullptr && d_func()->captureSession->camera();
}

/*!
    Returns the capture session this camera is connected to, or
    a nullptr if the camera is not connected to a capture session.

    Use QMediaCaptureSession::setImageCapture() to connect the image capture to
    a session.
*/
QMediaCaptureSession *QCameraImageCapture::captureSession() const
{
    return d_ptr->captureSession;
}

/*!
    Returns the current error state.

    \sa errorString()
*/

QCameraImageCapture::Error QCameraImageCapture::error() const
{
    return d_func()->error;
}

/*!
    Returns a string describing the current error state.

    \sa error()
*/

QString QCameraImageCapture::errorString() const
{
    return d_func()->errorString;
}

/*!
    Returns the meta data that will get embedded into the image. A couple of additional fields
    such as a time stamp or location might get added by the camera backend.
*/
QMediaMetaData QCameraImageCapture::metaData() const
{
    Q_D(const QCameraImageCapture);
    return d->metaData;
}

/*!
    Defines a set of meta data that will get embedded into the captured image.
*/
void QCameraImageCapture::setMetaData(const QMediaMetaData &metaData)
{
    Q_D(QCameraImageCapture);
    d->metaData = metaData;
    d->control->setMetaData(d->metaData);
    emit metaDataChanged();
}

/*!
    Adds additional meta data to be embedded into the captured image.
*/
void QCameraImageCapture::addMetaData(const QMediaMetaData &metaData)
{
    Q_D(QCameraImageCapture);
    auto data = d->metaData;
    for (auto k : metaData.keys())
        data.insert(k, metaData.value(k));
    setMetaData(data);
}

/*!
  \property QCameraImageCapture::readyForCapture
  \brief whether the service is ready to capture a an image immediately.

  Calling capture() while \e readyForCapture is \c false is not permitted and
  results in an error.
*/

bool QCameraImageCapture::isReadyForCapture() const
{
    Q_D(const QCameraImageCapture);
    if (!d->control || !d->captureSession || !d->control->isReadyForCapture())
        return false;
    auto *camera = d->captureSession->camera();
    if (!camera || !camera->isActive())
        return false;
    return true;
}

/*!
    \fn QCameraImageCapture::readyForCaptureChanged(bool ready)

    Signals that a camera's \a ready for capture state has changed.
*/


/*!
    Capture the image and save it to \a file.
    This operation is asynchronous in majority of cases,
    followed by signals QCameraImageCapture::imageExposed(),
    QCameraImageCapture::imageCaptured(), QCameraImageCapture::imageSaved()
    or QCameraImageCapture::error().

    If an empty \a file is passed, the camera backend choses
    the default location and naming scheme for photos on the system,
    if only file name without full path is specified, the image will be saved to
    the default directory, with a full path reported with imageCaptured() and imageSaved() signals.

    QCamera saves all the capture parameters like exposure settings or
    image processing parameters, so changes to camera parameters after
    capture() is called do not affect previous capture requests.

    QCameraImageCapture::capture returns the capture Id parameter, used with
    imageExposed(), imageCaptured() and imageSaved() signals.

    \sa isReadyForCapture()
*/
int QCameraImageCapture::captureToFile(const QString &file)
{
    Q_D(QCameraImageCapture);

    d->unsetError();

    if (!d->control) {
        d->_q_error(-1, NotSupportedFeatureError, tr("Camera is not ready."));
        return -1;
    }

    if (!isReadyForCapture()) {
        d->_q_error(-1, NotReadyError, tr("Could not capture in stopped state"));
        return -1;
    }

    return d->control->capture(file);
}

/*!
    Capture the image and make it available as a QImage.
    This operation is asynchronous in majority of cases,
    followed by signals QCameraImageCapture::imageExposed(),
    QCameraImageCapture::imageCaptured()
    or QCameraImageCapture::error().

    QCameraImageCapture::capture returns the capture Id parameter, used with
    imageExposed(), imageCaptured() and imageSaved() signals.

    \sa isReadyForCapture()
*/
int QCameraImageCapture::capture()
{
    Q_D(QCameraImageCapture);

    d->unsetError();

    if (d->control)
        return d->control->captureToBuffer();

    d->error = NotSupportedFeatureError;
    d->errorString = tr("Device does not support images capture.");

    d->_q_error(-1, d->error, d->errorString);

    return -1;
}

/*!
    \enum QCameraImageCapture::Error

    \value NoError         No Errors.
    \value NotReadyError   The service is not ready for capture yet.
    \value ResourceError   Device is not ready or not available.
    \value OutOfSpaceError No space left on device.
    \value NotSupportedFeatureError Device does not support stillimages capture.
    \value FormatError     Current format is not supported.
*/

/*!
    \fn QCameraImageCapture::error(int id, QCameraImageCapture::Error error, const QString &errorString)

    Signals that the capture request \a id has failed with an \a error
    and \a errorString description.
*/

/*!
    \fn QCameraImageCapture::bufferFormatChanged(QVideoFrameFormat::PixelFormat format)

    Signal emitted when the buffer \a format for the buffer image capture has changed.
*/

/*!
    \fn QCameraImageCapture::imageExposed(int id)

    Signal emitted when the frame with request \a id was exposed.
*/

/*!
    \fn QCameraImageCapture::imageCaptured(int id, const QImage &preview);

    Signal emitted when the frame with request \a id was captured, but not
    processed and saved yet. Frame \a preview can be displayed to user.
*/

/*!
    \fn QCameraImageCapture::imageMetadataAvailable(int id, const QString &key, const QVariant &value)

    Signals that a metadata for an image with request \a id is available. Also
    includes the \a key and \a value of the metadata.

    This signal is emitted between imageExposed and imageSaved signals.
*/

/*!
    \fn QCameraImageCapture::imageAvailable(int id, const QVideoFrame &frame)

    Signal emitted when QCameraImageCapture::CaptureToBuffer is set and
    the \a frame with request \a id is available.
*/

/*!
    \fn QCameraImageCapture::imageSaved(int id, const QString &fileName)

    Signal emitted when QCameraImageCapture::CaptureToFile is set and
    the frame with request \a id was saved to \a fileName.
*/

/*!
    Returns the image format.
*/

QCameraImageCapture::FileFormat QCameraImageCapture::fileFormat() const
{
    Q_D(const QCameraImageCapture);
    if (!d->control)
        return UnspecifiedFormat;
    return d->control->imageSettings().format();
}

/*!
    Sets the image \a format.
*/
void QCameraImageCapture::setFileFormat(QCameraImageCapture::FileFormat format)
{
    Q_D(QCameraImageCapture);
    if (!d->control)
        return;
    auto fmt = d->control->imageSettings();
    if (fmt.format() == format)
        return;
    fmt.setFormat(format);
    d->control->setImageSettings(fmt);
    emit fileFormatChanged();
}

QList<QCameraImageCapture::FileFormat> QCameraImageCapture::supportedFormats()
{
    return QPlatformMediaIntegration::instance()->formatInfo()->imageFormats;
}

QString QCameraImageCapture::fileFormatName(QCameraImageCapture::FileFormat f)
{
    const char *name = nullptr;
    switch (f) {
    case UnspecifiedFormat:
        name = "Unspecified image format";
        break;
    case JPEG:
        name = "JPEG";
        break;
    case PNG:
        name = "PNG";
        break;
    case WebP:
        name = "WebP";
        break;
    case Tiff:
        name = "Tiff";
        break;
    }
    return QString::fromUtf8(name);
}

QString QCameraImageCapture::fileFormatDescription(QCameraImageCapture::FileFormat f)
{
    const char *name = nullptr;
    switch (f) {
    case UnspecifiedFormat:
        name = "Unspecified image format";
        break;
    case JPEG:
        name = "JPEG";
        break;
    case PNG:
        name = "PNG";
        break;
    case WebP:
        name = "WebP";
        break;
    case Tiff:
        name = "Tiff";
        break;
    }
    return QString::fromUtf8(name);
}

/*!
    Returns the resolution of the encoded image.
*/

QSize QCameraImageCapture::resolution() const
{
    Q_D(const QCameraImageCapture);
    if (!d->control)
        return QSize();
    return d->control->imageSettings().resolution();
}

/*!
    Sets the \a resolution of the encoded image.

    An empty QSize indicates the encoder should make an optimal choice based on
    what is available from the image source and the limitations of the codec.
*/
void QCameraImageCapture::setResolution(const QSize &resolution)
{
    Q_D(QCameraImageCapture);
    if (!d->control)
        return;
    auto fmt = d->control->imageSettings();
    if (fmt.resolution() == resolution)
        return;
    fmt.setResolution(resolution);
    d->control->setImageSettings(fmt);
    emit resolutionChanged();
}

/*!
    Sets the \a width and \a height of the resolution of the encoded image.

    \overload
*/
void QCameraImageCapture::setResolution(int width, int height)
{
    setResolution(QSize(width, height));
}

/*!
    \enum QCameraImageCapture::EncodingQuality

    Enumerates quality encoding levels.

    \value VeryLowQuality
    \value LowQuality
    \value NormalQuality
    \value HighQuality
    \value VeryHighQuality
*/

/*!
    Returns the image encoding quality.
*/
QCameraImageCapture::Quality QCameraImageCapture::quality() const
{
    Q_D(const QCameraImageCapture);
    if (!d->control)
        return NormalQuality;
    return d->control->imageSettings().quality();
}

/*!
    Sets the image encoding \a quality.
*/
void QCameraImageCapture::setQuality(Quality quality)
{
    Q_D(QCameraImageCapture);
    if (!d->control)
        return;
    auto fmt = d->control->imageSettings();
    if (fmt.quality() == quality)
        return;
    fmt.setQuality(quality);
    d->control->setImageSettings(fmt);
    emit resolutionChanged();
}

QT_END_NAMESPACE

#include "moc_qcameraimagecapture.cpp"
