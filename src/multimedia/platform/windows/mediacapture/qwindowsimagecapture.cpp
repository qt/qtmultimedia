/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qwindowsimagecapture_p.h"

#include "qwindowsmediadevicesession_p.h"
#include "qwindowsmediacapture_p.h"
#include "qmediastoragelocation_p.h"

#include <QtConcurrent/qtconcurrentrun.h>
#include <QtGui/qimagewriter.h>

QT_BEGIN_NAMESPACE

QWindowsImageCapture::QWindowsImageCapture(QImageCapture *parent)
    : QPlatformImageCapture(parent)
{
}

QWindowsImageCapture::~QWindowsImageCapture() = default;

bool QWindowsImageCapture::isReadyForCapture() const
{
    if (!m_mediaDeviceSession)
        return false;
    return !m_capturing && m_mediaDeviceSession->isActive() && !m_mediaDeviceSession->activeCamera().isNull();
}

int QWindowsImageCapture::capture(const QString &fileName)
{
    auto ext = writerFormat(m_settings.format());
    auto path = QMediaStorageLocation::generateFileName(fileName, QStandardPaths::PicturesLocation, ext);
    return doCapture(path);
}

int QWindowsImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

int QWindowsImageCapture::doCapture(const QString &fileName)
{
    if (!isReadyForCapture())
        return -1;
    m_fileName = fileName;
    m_capturing = true;
    return m_captureId;
}

QImageEncoderSettings QWindowsImageCapture::imageSettings() const
{
    return m_settings;
}

void QWindowsImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    m_settings = settings;
}

void QWindowsImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWindowsMediaCaptureService *captureService = static_cast<QWindowsMediaCaptureService *>(session);
    if (m_captureService == captureService)
        return;

    auto readyForCapture = isReadyForCapture();
    if (m_mediaDeviceSession)
        disconnect(m_mediaDeviceSession, nullptr, this, nullptr);

    m_captureService = captureService;
    if (!m_captureService) {
        if (readyForCapture)
            emit readyForCaptureChanged(false);
        m_mediaDeviceSession = nullptr;
        return;
    }

    m_mediaDeviceSession = m_captureService->session();
    Q_ASSERT(m_mediaDeviceSession);

    if (isReadyForCapture() != readyForCapture)
        emit readyForCaptureChanged(isReadyForCapture());

    connect(m_mediaDeviceSession, SIGNAL(readyForCaptureChanged(bool)),
            this, SIGNAL(readyForCaptureChanged(bool)));

    connect(m_mediaDeviceSession, SIGNAL(videoFrameChanged(QVideoFrame)),
            this, SLOT(handleVideoFrameChanged(QVideoFrame)));
}

void QWindowsImageCapture::handleVideoFrameChanged(const QVideoFrame &frame)
{
    if (m_capturing) {

        QImage image = frame.toImage();

        emit imageExposed(m_captureId);
        emit imageAvailable(m_captureId, frame);
        emit imageCaptured(m_captureId, image);

        QMediaMetaData metaData = this->metaData();
        metaData.insert(QMediaMetaData::Date, QDateTime::currentDateTime());
        metaData.insert(QMediaMetaData::Resolution, frame.size());

        emit imageMetadataAvailable(m_captureId, metaData);

        if (!m_fileName.isEmpty()) {

            (void)QtConcurrent::run(&QWindowsImageCapture::saveImage, this,
                                    m_captureId, m_fileName, image, metaData, m_settings);
        }

        ++m_captureId;
        m_capturing = false;
    }
}

void QWindowsImageCapture::saveImage(int captureId, const QString &fileName,
                                           const QImage &image, const QMediaMetaData &metaData,
                                           const QImageEncoderSettings &settings)
{
    QImageWriter imageWriter;
    imageWriter.setFileName(fileName);

    QString format = writerFormat(settings.format());
    imageWriter.setFormat(format.toUtf8());

    int quality = writerQuality(format, settings.quality());
    if (quality > -1)
        imageWriter.setQuality(quality);

    for (auto key : metaData.keys())
        imageWriter.setText(QMediaMetaData::metaDataKeyToString(key),
                            metaData.stringValue(key));

    imageWriter.write(image);

    QMetaObject::invokeMethod(this, "imageSaved", Qt::QueuedConnection,
                              Q_ARG(int, captureId), Q_ARG(QString, fileName));
}

QString QWindowsImageCapture::writerFormat(QImageCapture::FileFormat reqFormat)
{
    QString format;

    switch (reqFormat) {
    case QImageCapture::FileFormat::JPEG:
        format = QLatin1String("jpg");
        break;
    case QImageCapture::FileFormat::PNG:
        format = QLatin1String("png");
        break;
    case QImageCapture::FileFormat::WebP:
        format = QLatin1String("webp");
        break;
    case QImageCapture::FileFormat::Tiff:
        format = QLatin1String("tiff");
        break;
    default:
        format = QLatin1String("jpg");
    }

    auto supported = QImageWriter::supportedImageFormats();
    for (const auto &f : supported)
        if (format.compare(QString::fromUtf8(f), Qt::CaseInsensitive) == 0)
            return format;

    return QLatin1String("jpg");
}

int QWindowsImageCapture::writerQuality(const QString &writerFormat,
                                              QImageCapture::Quality quality)
{
    if (writerFormat.compare(QLatin1String("jpg"), Qt::CaseInsensitive) == 0 ||
            writerFormat.compare(QLatin1String("jpeg"), Qt::CaseInsensitive) == 0) {

        switch (quality) {
        case QImageCapture::Quality::VeryLowQuality:
            return 10;
        case QImageCapture::Quality::LowQuality:
            return 30;
        case QImageCapture::Quality::NormalQuality:
            return 75;
        case QImageCapture::Quality::HighQuality:
            return 90;
        case QImageCapture::Quality::VeryHighQuality:
            return 98;
        default:
            return 75;
        }
    }
    return -1;
}

QT_END_NAMESPACE
