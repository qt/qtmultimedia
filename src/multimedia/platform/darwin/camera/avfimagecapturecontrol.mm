/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd and/or its subsidiary(-ies).
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

#include "avfcameradebug_p.h"
#include "avfimagecapturecontrol_p.h"
#include "avfcameraservice_p.h"
#include "avfcamerautility_p.h"
#include "avfcameracontrol_p.h"
#include <private/qmemoryvideobuffer_p.h>

#include <QtCore/qurl.h>
#include <QtCore/qfile.h>
#include <QtCore/qbuffer.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <QtGui/qimagereader.h>

QT_USE_NAMESPACE

AVFImageCaptureControl::AVFImageCaptureControl(AVFCameraService *service, QObject *parent)
   : QCameraImageCaptureControl(parent)
   , m_service(service)
   , m_session(service->session())
   , m_cameraControl(service->avfCameraControl())
   , m_ready(false)
   , m_lastCaptureId(0)
   , m_videoConnection(nil)
{
    Q_UNUSED(service);
    m_stillImageOutput = [[AVCaptureStillImageOutput alloc] init];

    NSDictionary *outputSettings = [[NSDictionary alloc] initWithObjectsAndKeys:
                                        AVVideoCodecTypeJPEG, AVVideoCodecKey, nil];

    [m_stillImageOutput setOutputSettings:outputSettings];
    [outputSettings release];
    connect(m_cameraControl, SIGNAL(statusChanged(QCamera::Status)), SLOT(updateReadyStatus()));

    connect(m_session, SIGNAL(readyToConfigureConnections()), SLOT(updateCaptureConnection()));

    connect(m_session, &AVFCameraSession::newViewfinderFrame,
            this, &AVFImageCaptureControl::onNewViewfinderFrame,
            Qt::DirectConnection);
}

AVFImageCaptureControl::~AVFImageCaptureControl()
{
}

bool AVFImageCaptureControl::isReadyForCapture() const
{
    return m_videoConnection && m_cameraControl->status() == QCamera::ActiveStatus;
}

void AVFImageCaptureControl::updateReadyStatus()
{
    if (m_ready != isReadyForCapture()) {
        m_ready = !m_ready;
        qDebugCamera() << "ReadyToCapture status changed:" << m_ready;
        Q_EMIT readyForCaptureChanged(m_ready);
    }
}

int AVFImageCaptureControl::capture(const QString &fileName)
{
    m_lastCaptureId++;

    if (!isReadyForCapture()) {
        QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                  Q_ARG(int, m_lastCaptureId),
                                  Q_ARG(int, QCameraImageCapture::NotReadyError),
                                  Q_ARG(QString, tr("Camera not ready")));
        return m_lastCaptureId;
    }

    auto destination = m_service->imageCaptureControl()->captureDestination();
    QString actualFileName;
    if (destination & QCameraImageCapture::CaptureToFile) {
        actualFileName = m_storageLocation.generateFileName(fileName,
                                                            AVFStorageLocation::Image,
                                                            QLatin1String("img_"),
                                                            QLatin1String("jpg"));

        qDebugCamera() << "Capture image to" << actualFileName;
    }

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
            qDebugCamera() << "Image capture failed:" << errorMessage;

            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                      Q_ARG(int, request.captureId),
                                      Q_ARG(int, QCameraImageCapture::ResourceError),
                                      Q_ARG(QString, errorMessage));
            return;
        }

        // Wait for the preview to be generated before saving the JPEG (but only
        // if we have AVFCameraRendererControl attached).
        // It is possible to stop camera immediately after trying to capture an
        // image; this can result in a blocked callback's thread, waiting for a
        // new viewfinder's frame to arrive/semaphore to be released. It is also
        // unspecified on which thread this callback gets executed, (probably it's
        // not the same thread that initiated a capture and stopped the camera),
        // so we cannot reliably check the camera's status. Instead, we wait
        // with a timeout and treat a failure to acquire a semaphore as an error.
        if (!m_session->videoOutput() || request.previewReady->tryAcquire(1, 1000)) {
            qDebugCamera() << "Image capture completed";

            NSData *nsJpgData = [AVCaptureStillImageOutput jpegStillImageNSDataRepresentation:imageSampleBuffer];
            QByteArray jpgData = QByteArray::fromRawData((const char *)[nsJpgData bytes], [nsJpgData length]);

            if (destination & QCameraImageCapture::CaptureToBuffer) {
                QBuffer data(&jpgData);
                QImageReader reader(&data, "JPEG");
                QSize size = reader.size();
                QVideoFrame frame(new QMemoryVideoBuffer(QByteArray(jpgData.constData(), jpgData.size()), -1), size, QVideoFrame::Format_Jpeg);
                QMetaObject::invokeMethod(this, "imageAvailable", Qt::QueuedConnection,
                                          Q_ARG(int, request.captureId),
                                          Q_ARG(QVideoFrame, frame));
            }

            if (!(destination & QCameraImageCapture::CaptureToFile))
                return;

            QFile f(actualFileName);
            if (f.open(QFile::WriteOnly)) {
                if (f.write(jpgData) != -1) {
                    QMetaObject::invokeMethod(this, "imageSaved", Qt::QueuedConnection,
                                              Q_ARG(int, request.captureId),
                                              Q_ARG(QString, actualFileName));
                } else {
                    QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                              Q_ARG(int, request.captureId),
                                              Q_ARG(int, QCameraImageCapture::OutOfSpaceError),
                                              Q_ARG(QString, f.errorString()));
                }
            } else {
                QString errorMessage = tr("Could not open destination file:\n%1").arg(actualFileName);
                QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                          Q_ARG(int, request.captureId),
                                          Q_ARG(int, QCameraImageCapture::ResourceError),
                                          Q_ARG(QString, errorMessage));
            }
        } else {
            const QLatin1String errorMessage("Image capture failed: timed out waiting"
                                             " for a preview frame.");
            qDebugCamera() << errorMessage;
            QMetaObject::invokeMethod(this, "error", Qt::QueuedConnection,
                                      Q_ARG(int, request.captureId),
                                      Q_ARG(int, QCameraImageCapture::ResourceError),
                                      Q_ARG(QString, errorMessage));
        }
    }];

    return request.captureId;
}

void AVFImageCaptureControl::onNewViewfinderFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&m_requestsMutex);

    if (m_captureRequests.isEmpty())
        return;

    CaptureRequest request = m_captureRequests.dequeue();
    Q_EMIT imageExposed(request.captureId);

    (void) QtConcurrent::run(&AVFImageCaptureControl::makeCapturePreview, this,
                      request,
                      frame,
                      0 /* rotation */);
}

void AVFImageCaptureControl::makeCapturePreview(CaptureRequest request,
                                                const QVideoFrame &frame,
                                                int rotation)
{
    QTransform transform;
    transform.rotate(rotation);

    Q_EMIT imageCaptured(request.captureId, frame.image().transformed(transform));

    request.previewReady->release();
}

void AVFImageCaptureControl::cancelCapture()
{
    //not supported
}

QCameraImageCapture::CaptureDestinations AVFImageCaptureControl::captureDestination() const
{
    return m_destination;
}

void AVFImageCaptureControl::setCaptureDestination(QCameraImageCapture::CaptureDestinations destination)
{
    if (m_destination != destination) {
        m_destination = destination;
        updateCaptureConnection();
    }
}

void AVFImageCaptureControl::updateCaptureConnection()
{
    if (m_session->videoCaptureDevice()) {
        qDebugCamera() << Q_FUNC_INFO;
        AVCaptureSession *captureSession = m_session->captureSession();

        if (![captureSession.outputs containsObject:m_stillImageOutput]) {
            if ([captureSession canAddOutput:m_stillImageOutput]) {
                // Lock the video capture device to make sure the active format is not reset
                const AVFConfigurationLock lock(m_session->videoCaptureDevice());
                [captureSession addOutput:m_stillImageOutput];
                m_videoConnection = [m_stillImageOutput connectionWithMediaType:AVMediaTypeVideo];
                updateReadyStatus();
            }
        } else {
            m_videoConnection = [m_stillImageOutput connectionWithMediaType:AVMediaTypeVideo];
        }
    }
}


QImageEncoderSettings AVFImageCaptureControl::imageSettings() const
{
    QImageEncoderSettings settings;

    if (!videoCaptureDeviceIsValid())
        return settings;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice.activeFormat) {
        qDebugCamera() << Q_FUNC_INFO << "no active format";
        return settings;
    }

    QSize res(qt_device_format_resolution(captureDevice.activeFormat));
#ifdef Q_OS_IOS
    if (!m_service->imageCaptureControl() || !m_service->imageCaptureControl()->stillImageOutput()) {
        qDebugCamera() << Q_FUNC_INFO << "no still image output";
        return settings;
    }

    AVCaptureStillImageOutput *stillImageOutput = m_service->imageCaptureControl()->stillImageOutput();
    if (stillImageOutput.highResolutionStillImageOutputEnabled)
        res = qt_device_format_high_resolution(captureDevice.activeFormat);
#endif
    if (res.isNull() || !res.isValid()) {
        qDebugCamera() << Q_FUNC_INFO << "failed to exctract the image resolution";
        return settings;
    }

    settings.setResolution(res);
    settings.setFormat(QImageEncoderSettings::JPEG);

    return settings;
}

void AVFImageCaptureControl::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_settings == settings)
        return;

    m_settings = settings;
    applySettings();
}

bool AVFImageCaptureControl::applySettings()
{
    if (!videoCaptureDeviceIsValid())
        return false;

    AVFCameraSession *session = m_service->session();
    if (!session || (session->state() != QCamera::ActiveState
        && session->state() != QCamera::LoadedState))
        return false;

    if (!m_service->imageCaptureControl()
        || !m_service->avfImageCaptureControl()->stillImageOutput()) {
        qDebugCamera() << Q_FUNC_INFO << "no still image output";
        return false;
    }

    if (m_settings.format() != QImageEncoderSettings::UnspecifiedFormat && m_settings.format() != QImageEncoderSettings::JPEG) {
        qDebugCamera() << Q_FUNC_INFO << "unsupported format:" << m_settings.format();
        return false;
    }

    QSize res(m_settings.resolution());
    if (res.isNull()) {
        qDebugCamera() << Q_FUNC_INFO << "invalid resolution:" << res;
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
        qDebugCamera() << Q_FUNC_INFO << "unsupported resolution:" << res;
        return false;
    }

    activeFormatChanged = qt_set_active_format(captureDevice, match, true);

#ifdef Q_OS_IOS
    AVCaptureStillImageOutput *imageOutput = m_service->imageCaptureControl()->stillImageOutput();
    if (res == qt_device_format_high_resolution(captureDevice.activeFormat))
        imageOutput.highResolutionStillImageOutputEnabled = YES;
    else
        imageOutput.highResolutionStillImageOutputEnabled = NO;
#endif

    return activeFormatChanged;
}

bool AVFImageCaptureControl::videoCaptureDeviceIsValid() const
{
    if (!m_service->session() || !m_service->session()->videoCaptureDevice())
        return false;

    AVCaptureDevice *captureDevice = m_service->session()->videoCaptureDevice();
    if (!captureDevice.formats || !captureDevice.formats.count)
        return false;

    return true;
}

#include "moc_avfimagecapturecontrol_p.cpp"
