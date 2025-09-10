// Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/private/qavfcameradebug_p.h>
#include "avfimagecapture_p.h"
#include "avfcameraservice_p.h"
#include <QtMultimedia/private/qavfcamerautility_p.h>
#include "avfcamera_p.h"
#include "avfcamerasession_p.h"
#include "avfcamerarenderer_p.h"
#include "private/qmediastoragelocation_p.h"
#include <private/qplatformimagecapture_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qvideoframe_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qbuffer.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QtGui/qimagereader.h>

#import <AVFoundation/AVFoundation.h>

QT_USE_NAMESPACE

AVFImageCapture::AVFImageCapture(QImageCapture *parent)
   : QPlatformImageCapture(parent)
{
    m_stillImageOutput = [[AVCaptureStillImageOutput alloc] init];

    NSDictionary *outputSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
                                        AVVideoCodecTypeJPEG, AVVideoCodecKey, nil];

    [m_stillImageOutput setOutputSettings:outputSettings];
    [outputSettings release];
}

AVFImageCapture::~AVFImageCapture()
{
    [m_stillImageOutput release];
}

bool AVFImageCapture::isReadyForCapture() const
{
    return m_cameraControl && m_videoConnection && m_cameraControl->isActive();
}

void AVFImageCapture::updateReadyStatus()
{
    if (m_ready != isReadyForCapture()) {
        m_ready = !m_ready;
        qCDebug(qLcCamera) << "ReadyToCapture status changed:" << m_ready;
        Q_EMIT readyForCaptureChanged(m_ready);
    }
}

int AVFImageCapture::doCapture(const QString &fileName)
{
    if (!m_session) {
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                Q_ARG(int, m_lastCaptureId),
                                Q_ARG(int, QImageCapture::ResourceError),
                                Q_ARG(QString, QPlatformImageCapture::msgImageCaptureNotSet()));
        return -1;
    }
    if (!isReadyForCapture()) {
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, m_lastCaptureId),
                                  Q_ARG(int, QImageCapture::NotReadyError),
                                  Q_ARG(QString, QPlatformImageCapture::msgCameraNotReady()));
        return -1;
    }
    m_lastCaptureId++;

    bool captureToBuffer = fileName.isEmpty();

    CaptureRequest request = { m_lastCaptureId, QSharedPointer<QSemaphore>::create()};
    m_requestsMutex.lock();
    m_captureRequests.enqueue(request);
    m_requestsMutex.unlock();

    [m_stillImageOutput captureStillImageAsynchronouslyFromConnection:m_videoConnection
                        completionHandler: ^(CMSampleBufferRef imageSampleBuffer, NSError *error) {

        if (error) {
            QStringList messageParts;
            messageParts << QString::fromUtf8([[error localizedDescription] UTF8String]);
            messageParts << QString::fromUtf8([[error localizedFailureReason] UTF8String]);
            messageParts << QString::fromUtf8([[error localizedRecoverySuggestion] UTF8String]);

            QString errorMessage = messageParts.join(QChar(u' '));
            qCDebug(qLcCamera) << "Image capture failed:" << errorMessage;

            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                      Q_ARG(int, request.captureId),
                                      Q_ARG(int, QImageCapture::ResourceError),
                                      Q_ARG(QString, errorMessage));
            return;
        }

        // Wait for the preview to be generated before saving the JPEG (but only
        // if we have AVFCameraRenderer attached).
        // It is possible to stop camera immediately after trying to capture an
        // image; this can result in a blocked callback's thread, waiting for a
        // new viewfinder's frame to arrive/semaphore to be released. It is also
        // unspecified on which thread this callback gets executed, (probably it's
        // not the same thread that initiated a capture and stopped the camera),
        // so we cannot reliably check the camera's status. Instead, we wait
        // with a timeout and treat a failure to acquire a semaphore as an error.
        if (!m_session->videoOutput() || request.previewReady->tryAcquire(1, 1000)) {
            qCDebug(qLcCamera) << "Image capture completed";

            NSData *nsJpgData = [AVCaptureStillImageOutput jpegStillImageNSDataRepresentation:imageSampleBuffer];
            QByteArray jpgData = QByteArray::fromRawData((const char *)[nsJpgData bytes], [nsJpgData length]);

            if (captureToBuffer) {
                QBuffer data(&jpgData);
                QImageReader reader(&data, "JPEG");
                QSize size = reader.size();
                auto buffer = std::make_unique<QMemoryVideoBuffer>(
                        QByteArray(jpgData.constData(), jpgData.size()), -1);
                QVideoFrame frame = QVideoFramePrivate::createFrame(
                        std::move(buffer), QVideoFrameFormat(size, QVideoFrameFormat::Format_Jpeg));
                QMetaObject::invokeMethod(this, "imageAvailable", Qt::QueuedConnection,
                                          Q_ARG(int, request.captureId),
                                          Q_ARG(QVideoFrame, frame));
            } else {
                QFile f(fileName);
                if (f.open(QFile::WriteOnly)) {
                    if (f.write(jpgData) != -1) {
                        QMetaObject::invokeMethod(this, "imageSaved", Qt::QueuedConnection,
                                                  Q_ARG(int, request.captureId),
                                                  Q_ARG(QString, fileName));
                    } else {
                        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                                  Q_ARG(int, request.captureId),
                                                  Q_ARG(int, QImageCapture::OutOfSpaceError),
                                                  Q_ARG(QString, f.errorString()));
                    }
                } else {
                    QString errorMessage = tr("Could not open destination file:\n%1").arg(fileName);
                    QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                              Q_ARG(int, request.captureId),
                                              Q_ARG(int, QImageCapture::ResourceError),
                                              Q_ARG(QString, errorMessage));
                }
            }
        } else {
            const QLatin1String errorMessage("Image capture failed: timed out waiting"
                                             " for a preview frame.");
            qCDebug(qLcCamera) << errorMessage;
            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                      Q_ARG(int, request.captureId),
                                      Q_ARG(int, QImageCapture::ResourceError),
                                      Q_ARG(QString, errorMessage));
        }
    }];

    return request.captureId;
}

int AVFImageCapture::capture(const QString &fileName)
{
    auto actualFileName = QMediaStorageLocation::generateFileName(fileName, QStandardPaths::PicturesLocation, QLatin1String("jpg"));

    qCDebug(qLcCamera) << "Capture image to" << actualFileName;
    return doCapture(actualFileName);
}

int AVFImageCapture::captureToBuffer()
{
    return doCapture(QString());
}

void AVFImageCapture::onNewViewfinderFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&m_requestsMutex);

    if (m_captureRequests.isEmpty())
        return;

    CaptureRequest request = m_captureRequests.dequeue();
    Q_EMIT imageExposed(request.captureId);

    (void) QtConcurrent::run(&AVFImageCapture::makeCapturePreview, this,
                      request,
                      frame,
                      0 /* rotation */);
}

void AVFImageCapture::onCameraChanged()
{
    auto camera = m_service ? static_cast<AVFCamera *>(m_service->camera()) : nullptr;

    if (camera == m_cameraControl)
        return;

    m_cameraControl = camera;

    if (m_cameraControl)
        connect(m_cameraControl, SIGNAL(activeChanged(bool)), this, SLOT(updateReadyStatus()));
    updateReadyStatus();
}

void AVFImageCapture::makeCapturePreview(CaptureRequest request,
                                                const QVideoFrame &frame,
                                                int rotation)
{
    QTransform transform;
    transform.rotate(rotation);

    Q_EMIT imageCaptured(request.captureId, frame.toImage().transformed(transform));

    request.previewReady->release();
}

void AVFImageCapture::updateCaptureConnection()
{
    if (m_session && m_session->videoCaptureDevice()) {
        qCDebug(qLcCamera) << Q_FUNC_INFO;
        AVCaptureSession *captureSession = m_session->captureSession();

        if (![captureSession.outputs containsObject:m_stillImageOutput]) {
            if ([captureSession canAddOutput:m_stillImageOutput]) {
                [captureSession beginConfiguration];
                // Lock the video capture device to make sure the active format is not reset
                const AVFConfigurationLock lock(m_session->videoCaptureDevice());
                [captureSession addOutput:m_stillImageOutput];
                m_videoConnection = [m_stillImageOutput connectionWithMediaType:AVMediaTypeVideo];
                [captureSession commitConfiguration];
                updateReadyStatus();
            }
        } else {
            m_videoConnection = [m_stillImageOutput connectionWithMediaType:AVMediaTypeVideo];
        }
    }
}


QImageEncoderSettings AVFImageCapture::imageSettings() const
{
    QImageEncoderSettings settings;

    if (!videoCaptureDeviceIsValid())
        return settings;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice.activeFormat) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "no active format";
        return settings;
    }

    QSize res(qt_device_format_resolution(captureDevice.activeFormat));
#ifdef Q_OS_IOS
    if (!m_service->avfImageCaptureControl() || !m_service->avfImageCaptureControl()->stillImageOutput()) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "no still image output";
        return settings;
    }

    AVCaptureStillImageOutput *stillImageOutput = m_service->avfImageCaptureControl()->stillImageOutput();
    if (stillImageOutput.highResolutionStillImageOutputEnabled)
        res = qt_device_format_high_resolution(captureDevice.activeFormat);
#endif
    if (res.isNull() || !res.isValid()) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "failed to exctract the image resolution";
        return settings;
    }

    settings.setResolution(res);
    settings.setFormat(QImageCapture::JPEG);

    return settings;
}

void AVFImageCapture::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings == settings)
        return;

    m_settings = settings;
    applySettings();
}

bool AVFImageCapture::applySettings()
{
    if (!videoCaptureDeviceIsValid())
        return false;

    AVFCameraSession *session = m_service->session();
    if (!session)
        return false;

    if (!m_service->imageCapture()
        || !m_service->avfImageCaptureControl()->stillImageOutput()) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "no still image output";
        return false;
    }

    if (m_settings.format() != QImageCapture::UnspecifiedFormat && m_settings.format() != QImageCapture::JPEG) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "unsupported format:" << m_settings.format();
        return false;
    }

    QSize res(m_settings.resolution());
    if (res.isNull()) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "invalid resolution:" << res;
        return false;
    }

    if (!res.isValid()) {
        // Invalid == default value.
        // Here we could choose the best format available, but
        // activeFormat is already equal to 'preset high' by default,
        // which is good enough, otherwise we can end in some format with low framerates.
        return false;
    }

    bool activeFormatChanged = false;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    AVCaptureDeviceFormat *match = qt_find_best_resolution_match(captureDevice, res,
                                                                 m_service->session()->defaultCodec());

    if (!match) {
        qCDebug(qLcCamera) << Q_FUNC_INFO << "unsupported resolution:" << res;
        return false;
    }

    activeFormatChanged = qt_set_active_format(captureDevice, match, true);

#ifdef Q_OS_IOS
    AVCaptureStillImageOutput *imageOutput = m_service->avfImageCaptureControl()->stillImageOutput();
    if (res == qt_device_format_high_resolution(captureDevice.activeFormat))
        imageOutput.highResolutionStillImageOutputEnabled = YES;
    else
        imageOutput.highResolutionStillImageOutputEnabled = NO;
#endif

    return activeFormatChanged;
}

void AVFImageCapture::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    AVFCameraService *captureSession = static_cast<AVFCameraService *>(session);
    if (m_service == captureSession)
        return;

    m_service = captureSession;
    if (!m_service) {
        m_session->disconnect(this);
        if (m_cameraControl)
            m_cameraControl->disconnect(this);
        m_session = nullptr;
        m_cameraControl = nullptr;
        m_videoConnection = nil;
    } else {
        m_session = m_service->session();
        Q_ASSERT(m_session);

        connect(m_service, &AVFCameraService::cameraChanged, this, &AVFImageCapture::onCameraChanged);
        connect(m_session, SIGNAL(readyToConfigureConnections()), SLOT(updateCaptureConnection()));
        connect(m_session, &AVFCameraSession::newViewfinderFrame,
                     this, &AVFImageCapture::onNewViewfinderFrame);
    }

    updateCaptureConnection();
    onCameraChanged();
    updateReadyStatus();
}

bool AVFImageCapture::videoCaptureDeviceIsValid() const
{
    if (!m_service || !m_service->session() || !m_service->session()->videoCaptureDevice())
        return false;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice.formats || !captureDevice.formats.count)
        return false;

    return true;
}

#include "moc_avfimagecapture_p.cpp"
