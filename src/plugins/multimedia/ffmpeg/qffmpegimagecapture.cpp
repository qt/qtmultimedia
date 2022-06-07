// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegimagecapture_p.h"
#include <private/qplatformmediaformatinfo_p.h>
#include <private/qplatformcamera_p.h>
#include <private/qplatformimagecapture_p.h>
#include <qvideoframeformat.h>
#include <private/qmediastoragelocation_p.h>
#include <qimagewriter.h>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <qstandardpaths.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcImageCapture, "qt.multimedia.imageCapture")

QFFmpegImageCapture::QFFmpegImageCapture(QImageCapture *parent)
  : QPlatformImageCapture(parent)
{
}

QFFmpegImageCapture::~QFFmpegImageCapture()
{
}

bool QFFmpegImageCapture::isReadyForCapture() const
{
    return m_isReadyForCapture;
}

static const char *extensionForFormat(QImageCapture::FileFormat format)
{
    const char *fmt = "jpg";
    switch (format) {
    case QImageCapture::UnspecifiedFormat:
    case QImageCapture::JPEG:
        fmt = "jpg";
        break;
    case QImageCapture::PNG:
        fmt = "png";
        break;
    case QImageCapture::WebP:
        fmt = "webp";
        break;
    case QImageCapture::Tiff:
        fmt = "tiff";
        break;
    }
    return fmt;
}

int QFFmpegImageCapture::capture(const QString &fileName)
{
    QString path = QMediaStorageLocation::generateFileName(fileName, QStandardPaths::PicturesLocation, QLatin1String(extensionForFormat(m_settings.format())));
    return doCapture(path);
}

int QFFmpegImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QFFmpegImageCapture::doCapture(const QString &fileName)
{
    qCDebug(qLcImageCapture) << "do capture";
    if (!m_session) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString, QPlatformImageCapture::msgImageCaptureNotSet()));

        qCDebug(qLcImageCapture) << "error 1";
        return -1;
    }
    if (!m_camera) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::ResourceError),
                                  Q_ARG(QString,tr("No camera available.")));

        qCDebug(qLcImageCapture) << "error 2";
        return -1;
    }
    if (passImage) {
        //emit error in the next event loop,
        //so application can associate it with returned request id.
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, -1),
                                  Q_ARG(int, QImageCapture::NotReadyError),
                                  Q_ARG(QString, QPlatformImageCapture::msgCameraNotReady()));

        qCDebug(qLcImageCapture) << "error 3";
        return -1;
    }
    m_lastId++;

    pendingImages.enqueue({m_lastId, fileName, QMediaMetaData{}});
    // let one image pass the pipeline
    passImage = true;

    updateReadyForCapture();
    return m_lastId;
}

void QFFmpegImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    auto *captureSession = static_cast<QFFmpegMediaCaptureSession *>(session);
    if (m_session == captureSession)
        return;

    if (m_session) {
        disconnect(m_session, nullptr, this, nullptr);
        m_lastId = 0;
        pendingImages.clear();
        passImage = false;
        cameraActive = false;
    }

    m_session = captureSession;
    if (m_session)
        connect(m_session, &QPlatformMediaCaptureSession::cameraChanged, this, &QFFmpegImageCapture::onCameraChanged);

    onCameraChanged();
    updateReadyForCapture();
}

void QFFmpegImageCapture::updateReadyForCapture()
{
    bool ready = m_session && !passImage && cameraActive;
    if (ready == m_isReadyForCapture)
        return;
    m_isReadyForCapture = ready;
    emit readyForCaptureChanged(m_isReadyForCapture);
}

void QFFmpegImageCapture::cameraActiveChanged(bool active)
{
    qCDebug(qLcImageCapture) << "cameraActiveChanged" << cameraActive << active;
    if (cameraActive == active)
        return;
    cameraActive = active;
    qCDebug(qLcImageCapture) << "isReady" << isReadyForCapture();
    updateReadyForCapture();
}

void QFFmpegImageCapture::newVideoFrame(const QVideoFrame &frame)
{
    if (!passImage)
        return;

    passImage = false;
    Q_ASSERT(!pendingImages.isEmpty());
    auto pending = pendingImages.dequeue();

    emit imageExposed(pending.id);
    // ### Add metadata from the AVFrame
    emit imageMetadataAvailable(pending.id, pending.metaData);
    emit imageAvailable(pending.id, frame);
    QImage image = frame.toImage();
    if (m_settings.resolution().isValid() && m_settings.resolution() != image.size())
        image = image.scaled(m_settings.resolution());

    emit imageCaptured(pending.id, image);
    if (!pending.filename.isEmpty()) {
        const char *fmt = nullptr;
        switch (m_settings.format()) {
        case QImageCapture::UnspecifiedFormat:
        case QImageCapture::JPEG:
            fmt = "jpeg";
            break;
        case QImageCapture::PNG:
            fmt = "png";
            break;
        case QImageCapture::WebP:
            fmt = "webp";
            break;
        case QImageCapture::Tiff:
            fmt = "tiff";
            break;
        }
        int quality = -1;
        switch (m_settings.quality()) {
        case QImageCapture::VeryLowQuality:
            quality = 25;
            break;
        case QImageCapture::LowQuality:
            quality = 50;
            break;
        case QImageCapture::NormalQuality:
            break;
        case QImageCapture::HighQuality:
            quality = 75;
            break;
        case QImageCapture::VeryHighQuality:
            quality = 99;
            break;
        }

        QImageWriter writer(pending.filename, fmt);
        writer.setQuality(quality);

        if (writer.write(image)) {
            emit imageSaved(pending.id, pending.filename);
        } else {
            QImageCapture::Error err = QImageCapture::ResourceError;
            if (writer.error() == QImageWriter::UnsupportedFormatError)
                err = QImageCapture::FormatError;
            emit error(pending.id, err, writer.errorString());
        }
    }
    updateReadyForCapture();
}

void QFFmpegImageCapture::onCameraChanged()
{
    auto *camera = m_session ? m_session->camera() : nullptr;
    if (m_camera == camera)
        return;

    if (m_camera)
        disconnect(m_camera);

    m_camera = camera;

    if (camera) {
        cameraActiveChanged(camera->isActive());
        connect(camera, &QPlatformCamera::activeChanged, this, &QFFmpegImageCapture::cameraActiveChanged);
        connect(camera, &QPlatformCamera::newVideoFrame, this, &QFFmpegImageCapture::newVideoFrame);
    } else {
        cameraActiveChanged(false);
    }
}

QImageEncoderSettings QFFmpegImageCapture::imageSettings() const
{
    return m_settings;
}

void QFFmpegImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    auto s = settings;
    const auto supportedFormats = QPlatformMediaIntegration::instance()->formatInfo()->imageFormats;
    if (supportedFormats.isEmpty()) {
        emit error(-1, QImageCapture::FormatError, "No image formats supported, can't capture.");
        return;
    }
    if (s.format() == QImageCapture::UnspecifiedFormat) {
        auto f = QImageCapture::JPEG;
        if (!supportedFormats.contains(f))
            f = supportedFormats.first();
        s.setFormat(f);
    } else if (!supportedFormats.contains(settings.format())) {
        emit error(-1, QImageCapture::FormatError, "Image format not supported.");
        return;
    }

    m_settings = settings;
}

QT_END_NAMESPACE
