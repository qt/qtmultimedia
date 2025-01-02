// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmimagecapture_p.h"
#include <qimagewriter.h>
#include "qwasmmediacapturesession_p.h"
#include "qwasmcamera_p.h"
#include "qwasmvideosink_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qWasmImageCapture, "qt.multimedia.wasm.imagecapture")
/* TODO
signals:
imageExposed
*/
QWasmImageCapture::QWasmImageCapture(QImageCapture *parent) : QPlatformImageCapture(parent) { }

QWasmImageCapture::~QWasmImageCapture() = default;

int QWasmImageCapture::capture(const QString &fileName)
{
    if (!isReadyForCapture()) {
        emit error(m_lastId, QImageCapture::NotReadyError, msgCameraNotReady());
        return -1;
    }

    // TODO if fileName.isEmpty() we choose filename and location

    QImage image = takePicture();
    if (image.isNull())
        return -1;

    QImageWriter writer(fileName);
    // TODO
    // writer.setQuality(quality);
    // writer.setFormat("png");

    if (writer.write(image)) {
        qCDebug(qWasmImageCapture) << Q_FUNC_INFO << "image saved";
        emit imageSaved(m_lastId, fileName);
    } else {
        QImageCapture::Error err = (writer.error() == QImageWriter::UnsupportedFormatError)
                ? QImageCapture::FormatError
                : QImageCapture::ResourceError;

        emit error(m_lastId, err, writer.errorString());
    }

    return m_lastId;
}

int QWasmImageCapture::captureToBuffer()
{
    if (!isReadyForCapture()) {
        emit error(m_lastId, QImageCapture::NotReadyError, msgCameraNotReady());
        return -1;
    }

    QImage image = takePicture();
    if (image.isNull())
        return -1;

    emit imageCaptured(m_lastId, image);
    return m_lastId;
}

QImage QWasmImageCapture::takePicture()
{
    QVideoFrame thisFrame = m_captureSession->videoSink()->videoFrame();
    if (!thisFrame.isValid())
        return QImage();

    m_lastId++;
    emit imageAvailable(m_lastId, thisFrame);

    QImage image = thisFrame.toImage();
    if (image.isNull()) {
        qCDebug(qWasmImageCapture) << Q_FUNC_INFO << "image is null";
        emit error(m_lastId, QImageCapture::ResourceError, QStringLiteral("Resource error"));
        return QImage();
    }

    emit imageCaptured(m_lastId, image);
    if (m_settings.resolution().isValid() && m_settings.resolution() != image.size())
        image = image.scaled(m_settings.resolution());

    return image;
}

bool QWasmImageCapture::isReadyForCapture() const
{
    return m_isReadyForCapture;
}

QImageEncoderSettings QWasmImageCapture::imageSettings() const
{
    return m_settings;
}

void QWasmImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    m_settings = settings;
}

void QWasmImageCapture::setReadyForCapture(bool isReady)
{
    if (m_isReadyForCapture != isReady) {
        m_isReadyForCapture = isReady;
        emit readyForCaptureChanged(m_isReadyForCapture);
    }
}

void QWasmImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWasmMediaCaptureSession *captureSession = static_cast<QWasmMediaCaptureSession *>(session);
    //  nullptr clears
    if (m_captureSession == captureSession)
        return;

    m_isReadyForCapture = captureSession;
    if (captureSession) {
        m_lastId = 0;
        m_captureSession = captureSession;
    }
    emit readyForCaptureChanged(m_isReadyForCapture);
    m_captureSession = captureSession;
}

QT_END_NAMESPACE
