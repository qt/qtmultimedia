/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "qqnximagecapture_p.h"

#include "qqnxmediacapture_p.h"
#include "qqnxcamera_p.h"
#include "qfile.h"

#include <camera/camera_api.h>

QT_BEGIN_NAMESPACE

QQnxImageCapture::QQnxImageCapture(QImageCapture *parent)
    : QPlatformImageCapture(parent)
{
}

bool QQnxImageCapture::isReadyForCapture() const
{
    if (!m_session)
        return false;
    auto *camera = static_cast<QQnxCamera *>(m_session->camera());
    // ### add can take photo
    return camera && camera->isActive();
}

static void imageCaptureShutterCallback(camera_handle_t handle, void *context)
{
    Q_UNUSED(handle);

    const auto *data = static_cast<QQnxImageCapture::PendingImage *>(context);

    // We are inside a worker thread here, so emit imageExposed inside the main thread
    QMetaObject::invokeMethod(data->imageCapture, "imageExposed", Qt::QueuedConnection, Q_ARG(int, data->id));
}

#if 0

static void imageCaptureImageCallback(camera_handle_t handle, camera_buffer_t *buffer, void *context)
{
    Q_UNUSED(handle);

    auto *data = static_cast<QQnxImageCapture::PendingImage *>(context);

    if (buffer->frametype != CAMERA_FRAMETYPE_JPEG) {
        // ### Fix this, we can support other formats as well!

        // We are inside a worker thread here, so emit error signal inside the main thread
        QMetaObject::invokeMethod(data->imageCapture, "error", Qt::QueuedConnection,
                                  Q_ARG(int, data->id),
                                  Q_ARG(QImageCapture::Error, QImageCapture::FormatError),
                                  Q_ARG(QString, QCamera::tr("Camera provides image in unsupported format")));
        return;
    }

    const QByteArray rawData = QByteArray::fromRawData((const char*)buffer->framebuf, buffer->framedesc.jpeg.bufsize);

    QImage image;
    const bool ok = image.loadFromData(rawData, "JPG");
    if (!ok) {
        const QString errorMessage = QCamera::tr("Could not load JPEG data from frame");
        // We are inside a worker thread here, so emit error signal inside the main thread
        QMetaObject::invokeMethod(data->imageCapture, "error", Qt::QueuedConnection,
                                  Q_ARG(int, data->id),
                                  Q_ARG(QImageCapture::Error, QImageCapture::FormatError),
                                  Q_ARG(QString, errorMessage));
        return;
    }


    // We are inside a worker thread here, so invoke imageCaptured inside the main thread
    QMetaObject::invokeMethod(data->imageCapture, "imageCaptured", Qt::QueuedConnection,
                              Q_ARG(int, data->id),
                              Q_ARG(QImage, image));

    if (!data->filename.isEmpty()) {
        QFile file(data->filename);
        if (file.exists()) {
            const QString errorMessage = QCamera::tr("Could not overwrite existing file");
            // We are inside a worker thread here, so emit error signal inside the main thread
            QMetaObject::invokeMethod(data->imageCapture, "error", Qt::QueuedConnection,
                                      Q_ARG(int, data->id),
                                      Q_ARG(QImageCapture::Error, QImageCapture::ResourceError),
                                      Q_ARG(QString, errorMessage));
            return;
        }
        if (!file.open(QFile::WriteOnly)) {
            const QString errorMessage = QCamera::tr("Could not write image to file");
            // We are inside a worker thread here, so emit error signal inside the main thread
            QMetaObject::invokeMethod(data->imageCapture, "error", Qt::QueuedConnection,
                                      Q_ARG(int, data->id),
                                      Q_ARG(QImageCapture::Error, QImageCapture::ResourceError),
                                      Q_ARG(QString, errorMessage));
            return;
        }
        file.write(rawData);
        file.close();
        QMetaObject::invokeMethod(data->imageCapture, "imageSaved", Qt::QueuedConnection,
                                  Q_ARG(int, data->id),
                                  Q_ARG(QString, data->filename));
    }
    delete data;
}
#endif


int QQnxImageCapture::capture(const QString &fileName)
{
    auto *camera = static_cast<QQnxCamera *>(m_session->camera());
    // ### add can take photo
    if (!camera || !camera->isActive()) {
        emit error(-1, QImageCapture::NotReadyError, QPlatformImageCapture::msgCameraNotReady());
        return -1;
    }

    // prepare context object for callback
    PendingImage *pending = new PendingImage;
    pending->id = m_lastId;
    pending->filename = fileName;
    pending->imageCapture = this;
    m_lastId++;

#if 0
    // ### camera_take_photo doesn't exist anymore afaict
    const camera_error_t result = camera_take_photo(camera->handle(),
                                                    imageCaptureShutterCallback,
                                                    0,
                                                    0,
                                                    imageCaptureImageCallback,
                                                    pending, false);

    if (result != CAMERA_EOK) {
        qWarning() << "Unable to take photo:" << result;
        emit error(-1, QImageCapture::NotReadyError, QPlatformImageCapture::msgCameraNotReady());
        return -1;
    }
#endif

    return m_lastId;
}

int QQnxImageCapture::captureToBuffer()
{
    // ### implement me
    return -1;
}

QImageEncoderSettings QQnxImageCapture::imageSettings() const
{
    return m_settings;
}

void QQnxImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    m_settings = settings;
}

void QQnxImageCapture::setCaptureSession(QQnxMediaCaptureSession *captureSession)
{
    if (m_session == captureSession)
        return;

    bool oldReadyForCapture = isReadyForCapture();
    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
    }

    m_session = captureSession;

    bool readyForCapture = isReadyForCapture();
    if (readyForCapture != oldReadyForCapture)
        emit readyForCaptureChanged(readyForCapture);

    if (!m_session)
        return;

//    connect(m_session, &QPlatformMediaCaptureSession::cameraChanged, this, &QGstreamerImageCapture::onCameraChanged);
//    onCameraChanged();
}

QT_END_NAMESPACE
