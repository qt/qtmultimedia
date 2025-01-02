// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqnximagecapture_p.h"

#include "qqnxplatformcamera_p.h"
#include "qqnxmediacapturesession_p.h"
#include "qqnxcamera_p.h"
#include "qfile.h"

#include <private/qmediastoragelocation_p.h>

#include <QtCore/qfileinfo.h>
#include <QtCore/qfuture.h>
#include <QtCore/qpromise.h>
#include <QtCore/qthread.h>

#include <camera/camera_api.h>

using namespace Qt::Literals::StringLiterals;

static QString formatExtension(QImageCapture::FileFormat format)
{
    switch (format) {
    case QImageCapture::JPEG:
        return u"jpg"_s;
    case QImageCapture::PNG:
        return u"png"_s;
    case QImageCapture::WebP:
    case QImageCapture::Tiff:
    case QImageCapture::UnspecifiedFormat:
        break;
    }

    return {};
}

static QString resolveFileName(const QString &fileName, QImageCapture::FileFormat format)
{
    const QString extension = formatExtension(format);

    if (extension.isEmpty())
        return {};

    if (fileName.isEmpty()) {
        return QMediaStorageLocation::generateFileName(QString(),
                QStandardPaths::PicturesLocation, extension);
    }

    if (fileName.endsWith(extension))
        return QFileInfo(fileName).canonicalFilePath();

    QString path = fileName;
    path.append(u".%1"_s.arg(extension));

    return QFileInfo(path).canonicalFilePath();
}

QT_BEGIN_NAMESPACE

QQnxImageCapture::QQnxImageCapture(QImageCapture *parent)
    : QPlatformImageCapture(parent)
{
}

bool QQnxImageCapture::isReadyForCapture() const
{
    return m_camera && m_camera->isActive();
}

int QQnxImageCapture::capture(const QString &fileName)
{
    if (!isReadyForCapture()) {
        Q_EMIT error(-1, QImageCapture::NotReadyError, QPlatformImageCapture::msgCameraNotReady());
        return -1;
    }

    // default to PNG format if no format has been specified
    const QImageCapture::FileFormat format =
        m_settings.format() == QImageCapture::UnspecifiedFormat
        ? QImageCapture::PNG : m_settings.format();

    const QString resolvedFileName = resolveFileName(fileName, format);

    if (resolvedFileName.isEmpty()) {
        const QString errorMessage = (u"Invalid file format: %1"_s).arg(
                QImageCapture::fileFormatName(format));

        Q_EMIT error(-1, QImageCapture::NotSupportedFeatureError, errorMessage);
        return -1;
    }

    const int id = m_lastId++;

    auto callback = [this, id, fn=std::move(resolvedFileName)](const QVideoFrame &frame) {
        saveFrame(id, frame, fn);
    };

    m_camera->requestVideoFrame(std::move(callback));

    return id;
}

int QQnxImageCapture::captureToBuffer()
{
    if (!isReadyForCapture()) {
        Q_EMIT error(-1, QImageCapture::NotReadyError, QPlatformImageCapture::msgCameraNotReady());
        return -1;
    }

    const int id = m_lastId++;

    auto callback = [this, id](const QVideoFrame &frame) { decodeFrame(id, frame); };

    m_camera->requestVideoFrame(std::move(callback));

    return id;
}

QFuture<QImage> QQnxImageCapture::decodeFrame(int id, const QVideoFrame &frame)
{
    if (!frame.isValid()) {
        Q_EMIT error(id, QImageCapture::NotReadyError, u"Invalid frame"_s);
        return {};
    }

    QPromise<QImage> promise;
    QFuture<QImage> future = promise.future();

    // converting a QVideoFrame to QImage is an expensive operation
    // run it on a background thread to prevent it from stalling the UI
    auto runner = [frame, promise=std::move(promise)]() mutable {
        promise.start();
        promise.addResult(frame.toImage());
        promise.finish();
    };

    auto *worker = QThread::create(std::move(runner));

    auto onFinished = [this, worker, id, future]() mutable {
        worker->deleteLater();

        if (future.isValid()) {
            Q_EMIT imageCaptured(id, future.result());
        } else {
            qWarning("QQnxImageCapture: failed to capture image to buffer");
        }
    };

    connect(worker, &QThread::finished, this, std::move(onFinished));

    Q_EMIT imageExposed(id);

    worker->start();

    return future;
}

void QQnxImageCapture::saveFrame(int id, const QVideoFrame &frame, const QString &fileName)
{
    QFuture<QImage> decodeFuture = decodeFrame(id, frame);

    if (decodeFuture.isCanceled())
        return;

    QPromise<bool> promise;
    QFuture<bool> saveFuture = promise.future();

    // writing a QImage to disk is a _very_ expensive operation
    // run it on a background thread to prevent it from stalling the UI
    auto runner = [future=std::move(decodeFuture),
         promise=std::move(promise), fileName]() mutable {
        promise.start();
        promise.addResult(future.result().save(fileName));
        promise.finish();
    };

    auto *worker = QThread::create(std::move(runner));

    auto onFinished = [this, worker, id, future=std::move(saveFuture), fn=std::move(fileName)]() {
        worker->deleteLater();

        if (future.isValid() && future.result())
            Q_EMIT imageSaved(id, fn);
        else
            Q_EMIT error(id, QImageCapture::NotSupportedFeatureError, u"Failed to save image"_s);
    };

    connect(worker, &QThread::finished, this, std::move(onFinished));

    worker->start();
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

    if (m_session)
        m_session->disconnect(this);

    m_session = captureSession;

    if (m_session) {
        connect(m_session, &QQnxMediaCaptureSession::cameraChanged,
                this, &QQnxImageCapture::onCameraChanged);
    }

    onCameraChanged();
}

void QQnxImageCapture::onCameraChanged()
{
    if (m_camera)
        m_camera->disconnect(this);

    m_camera = m_session ? static_cast<QQnxPlatformCamera*>(m_session->camera()) : nullptr;

    if (m_camera) {
        connect(m_camera, &QQnxPlatformCamera::activeChanged,
                this, &QQnxImageCapture::onCameraChanged);
    }

    updateReadyForCapture();
}

void QQnxImageCapture::onCameraActiveChanged(bool active)
{
    Q_UNUSED(active);

    updateReadyForCapture();
}

void QQnxImageCapture::updateReadyForCapture()
{
    const bool readyForCapture = isReadyForCapture();

    if (m_lastReadyForCapture == readyForCapture)
        return;

    m_lastReadyForCapture = readyForCapture;

    Q_EMIT readyForCaptureChanged(m_lastReadyForCapture);
}

QT_END_NAMESPACE

#include "moc_qqnximagecapture_p.cpp"
