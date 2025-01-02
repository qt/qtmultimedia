// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2016 Ruslan Baratov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidcamerasession_p.h"

#include "androidcamera_p.h"
#include "androidmultimediautils_p.h"
#include "qandroidvideooutput_p.h"
#include "qandroidmultimediautils_p.h"
#include "androidmediarecorder_p.h"
#include <qvideosink.h>
#include <QtConcurrent/qtconcurrentrun.h>
#include <qfile.h>
#include <qguiapplication.h>
#include <qscreen.h>
#include <qdebug.h>
#include <qvideoframe.h>
#include <private/qplatformimagecapture_p.h>
#include <private/qplatformvideosink_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qcameradevice_p.h>
#include <private/qmediastoragelocation_p.h>
#include <QImageWriter>

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QList<QCameraDevice>, g_availableCameras)

QAndroidCameraSession::QAndroidCameraSession(QObject *parent)
    : QObject(parent)
    , m_selectedCamera(0)
    , m_camera(0)
    , m_videoOutput(0)
    , m_savedState(-1)
    , m_previewStarted(false)
    , m_readyForCapture(false)
    , m_currentImageCaptureId(-1)
    , m_previewCallback(0)
    , m_keepActive(false)
{
    if (qApp) {
        connect(qApp, &QGuiApplication::applicationStateChanged,
                this, &QAndroidCameraSession::onApplicationStateChanged);

        auto screen = qApp->primaryScreen();
        if (screen) {
            connect(screen, &QScreen::orientationChanged, this,
                    &QAndroidCameraSession::updateOrientation);
            enableRotation();
        }
    }
}

QAndroidCameraSession::~QAndroidCameraSession()
{
    if (m_sink)
        disconnect(m_retryPreviewConnection);
    close();
}

//void QAndroidCameraSession::setCaptureMode(QCamera::CaptureModes mode)
//{
//    if (m_captureMode == mode || !isCaptureModeSupported(mode))
//        return;

//    m_captureMode = mode;
//    emit captureModeChanged(m_captureMode);

//    if (m_previewStarted && m_captureMode.testFlag(QCamera::CaptureStillImage))
//        applyResolution(m_actualImageSettings.resolution());
//}

void QAndroidCameraSession::setActive(bool active)
{
    if (m_active == active)
        return;

    // If the application is inactive, the camera shouldn't be started. Save the desired state
    // instead and it will be set when the application becomes active.
    if (active && qApp->applicationState() == Qt::ApplicationInactive) {
        m_isStateSaved = true;
        m_savedState = active;
        return;
    }

    m_isStateSaved = false;
    m_active = active;
    setActiveHelper(m_active);
    emit activeChanged(m_active);
}

void QAndroidCameraSession::setActiveHelper(bool active)
{
    if (!active) {
        stopPreview();
        close();
    } else {
        if (!m_camera && !open()) {
            emit error(QCamera::CameraError, QStringLiteral("Failed to open camera"));
            return;
        }
        startPreview();
    }
}

void QAndroidCameraSession::updateAvailableCameras()
{
    g_availableCameras->clear();

    const int numCameras = AndroidCamera::getNumberOfCameras();
    for (int i = 0; i < numCameras; ++i) {
        QCameraDevicePrivate *info = new QCameraDevicePrivate;
        AndroidCamera::getCameraInfo(i, info);

        if (!info->id.isEmpty()) {
            // Add supported picture and video sizes to the camera info
            AndroidCamera *camera = AndroidCamera::open(i);

            if (camera) {
                info->videoFormats = camera->getSupportedFormats();
                info->photoResolutions = camera->getSupportedPictureSizes();
            }

            delete camera;
            g_availableCameras->append(info->create());
        }
    }
}

const QList<QCameraDevice> &QAndroidCameraSession::availableCameras()
{
    if (g_availableCameras->isEmpty())
        updateAvailableCameras();

    return *g_availableCameras;
}

bool QAndroidCameraSession::open()
{
    close();

    m_camera = AndroidCamera::open(m_selectedCamera);

    if (m_camera) {
        connect(m_camera, &AndroidCamera::pictureExposed,
                this, &QAndroidCameraSession::onCameraPictureExposed);
        connect(m_camera, &AndroidCamera::lastPreviewFrameFetched,
                this, &QAndroidCameraSession::onLastPreviewFrameFetched,
                Qt::DirectConnection);
        connect(m_camera, &AndroidCamera::newPreviewFrame,
                this, &QAndroidCameraSession::onNewPreviewFrame,
                Qt::DirectConnection);
        connect(m_camera, &AndroidCamera::pictureCaptured,
                this, &QAndroidCameraSession::onCameraPictureCaptured);
        connect(m_camera, &AndroidCamera::previewStarted,
                this, &QAndroidCameraSession::onCameraPreviewStarted);
        connect(m_camera, &AndroidCamera::previewStopped,
                this, &QAndroidCameraSession::onCameraPreviewStopped);
        connect(m_camera, &AndroidCamera::previewFailedToStart,
                this, &QAndroidCameraSession::onCameraPreviewFailedToStart);
        connect(m_camera, &AndroidCamera::takePictureFailed,
                this, &QAndroidCameraSession::onCameraTakePictureFailed);

        if (m_camera->getPreviewFormat() != AndroidCamera::NV21)
            m_camera->setPreviewFormat(AndroidCamera::NV21);

        m_camera->notifyNewFrames(m_previewCallback);

        emit opened();
        setActive(true);
    }

    return m_camera != 0;
}

void QAndroidCameraSession::close()
{
    if (!m_camera)
        return;

    stopPreview();

    m_readyForCapture = false;
    m_currentImageCaptureId = -1;
    m_currentImageCaptureFileName.clear();
    m_actualImageSettings = m_requestedImageSettings;

    m_camera->release();
    delete m_camera;
    m_camera = 0;

    setActive(false);
}

void QAndroidCameraSession::setVideoOutput(QAndroidVideoOutput *output)
{
    if (m_videoOutput) {
        m_videoOutput->stop();
        m_videoOutput->reset();
    }

    if (output) {
        m_videoOutput = output;
        if (m_videoOutput->isReady()) {
            onVideoOutputReady(true);
        } else {
            connect(m_videoOutput, &QAndroidVideoOutput::readyChanged,
                    this, &QAndroidCameraSession::onVideoOutputReady);
        }
    } else {
        m_videoOutput = 0;
    }
}

void QAndroidCameraSession::setCameraFormat(const QCameraFormat &format)
{
    m_requestedFpsRange.min = format.minFrameRate();
    m_requestedFpsRange.max = format.maxFrameRate();
    m_requestedPixelFromat = AndroidCamera::AndroidImageFormatFromQtPixelFormat(format.pixelFormat());

    m_requestedImageSettings.setResolution(format.resolution());
    m_actualImageSettings.setResolution(format.resolution());
    if (m_readyForCapture)
        applyResolution(m_actualImageSettings.resolution());
}

void QAndroidCameraSession::applyResolution(const QSize &captureSize, bool restartPreview)
{
    if (!m_camera)
        return;

    const QSize currentViewfinderResolution = m_camera->previewSize();
    const AndroidCamera::ImageFormat currentPreviewFormat = m_camera->getPreviewFormat();
    const AndroidCamera::FpsRange currentFpsRange = m_camera->getPreviewFpsRange();

    // -- adjust resolution
    QSize adjustedViewfinderResolution;
    const QList<QSize> previewSizes = m_camera->getSupportedPreviewSizes();

    const bool validCaptureSize = captureSize.width() > 0 && captureSize.height() > 0;
    if (validCaptureSize
            && m_camera->getPreferredPreviewSizeForVideo().isEmpty()) {
        // According to the Android doc, if getPreferredPreviewSizeForVideo() returns null, it means
        // the preview size cannot be different from the capture size
        adjustedViewfinderResolution = captureSize;
    } else {
        qreal captureAspectRatio = 0;
        if (validCaptureSize)
            captureAspectRatio = qreal(captureSize.width()) / qreal(captureSize.height());

        if (validCaptureSize) {
            // search for viewfinder resolution with the same aspect ratio
            qreal minAspectDiff = 1;
            QSize closestResolution;
            for (int i = previewSizes.count() - 1; i >= 0; --i) {
                const QSize &size = previewSizes.at(i);
                const qreal sizeAspect = qreal(size.width()) / size.height();
                if (qFuzzyCompare(captureAspectRatio, sizeAspect)) {
                    adjustedViewfinderResolution = size;
                    break;
                } else if (minAspectDiff > qAbs(sizeAspect - captureAspectRatio)) {
                    closestResolution = size;
                    minAspectDiff = qAbs(sizeAspect - captureAspectRatio);
                }
            }
            if (!adjustedViewfinderResolution.isValid()) {
                qWarning("Cannot find a viewfinder resolution matching the capture aspect ratio.");
                if (closestResolution.isValid()) {
                    adjustedViewfinderResolution = closestResolution;
                    qWarning("Using closest viewfinder resolution.");
                } else {
                    return;
                }
            }
        } else {
            adjustedViewfinderResolution = previewSizes.last();
        }
    }

    // -- adjust pixel format

    AndroidCamera::ImageFormat adjustedPreviewFormat = m_requestedPixelFromat;
    if (adjustedPreviewFormat == AndroidCamera::UnknownImageFormat)
        adjustedPreviewFormat = AndroidCamera::NV21;

    // -- adjust FPS

    AndroidCamera::FpsRange adjustedFps = m_requestedFpsRange;;
    if (adjustedFps.min == 0 || adjustedFps.max == 0)
        adjustedFps = currentFpsRange;

    // -- Set values on camera

    // fix the resolution of output based on the orientation
    QSize cameraOutputResolution = adjustedViewfinderResolution;
    QSize videoOutputResolution = adjustedViewfinderResolution;
    QSize currentVideoOutputResolution = m_videoOutput ? m_videoOutput->getVideoSize() : QSize(0, 0);
    const int rotation = currentCameraRotation();
    // only transpose if it's valid for the preview
    if (rotation == 90 || rotation == 270) {
        videoOutputResolution.transpose();
        if (previewSizes.contains(cameraOutputResolution.transposed()))
            cameraOutputResolution.transpose();
    }

    if (currentViewfinderResolution != cameraOutputResolution
        || (m_videoOutput && currentVideoOutputResolution != videoOutputResolution)
        || currentPreviewFormat != adjustedPreviewFormat || currentFpsRange.min != adjustedFps.min
        || currentFpsRange.max != adjustedFps.max) {
        if (m_videoOutput) {
            m_videoOutput->setVideoSize(videoOutputResolution);
        }

        // if preview is started, we have to stop it first before changing its size
        if (m_previewStarted && restartPreview)
            m_camera->stopPreview();

        m_camera->setPreviewSize(cameraOutputResolution);
        m_camera->setPreviewFormat(adjustedPreviewFormat);
        m_camera->setPreviewFpsRange(adjustedFps);

        // restart preview
        if (m_previewStarted && restartPreview)
            m_camera->startPreview();
    }
}

QList<QSize> QAndroidCameraSession::getSupportedPreviewSizes() const
{
    return m_camera ? m_camera->getSupportedPreviewSizes() : QList<QSize>();
}

QList<QVideoFrameFormat::PixelFormat> QAndroidCameraSession::getSupportedPixelFormats() const
{
    QList<QVideoFrameFormat::PixelFormat> formats;

    if (!m_camera)
        return formats;

    const QList<AndroidCamera::ImageFormat> nativeFormats = m_camera->getSupportedPreviewFormats();

    formats.reserve(nativeFormats.size());

    for (AndroidCamera::ImageFormat nativeFormat : nativeFormats) {
        QVideoFrameFormat::PixelFormat format = AndroidCamera::QtPixelFormatFromAndroidImageFormat(nativeFormat);
        if (format != QVideoFrameFormat::Format_Invalid)
            formats.append(format);
    }

    return formats;
}

QList<AndroidCamera::FpsRange> QAndroidCameraSession::getSupportedPreviewFpsRange() const
{
    return m_camera ? m_camera->getSupportedPreviewFpsRange() : QList<AndroidCamera::FpsRange>();
}


bool QAndroidCameraSession::startPreview()
{
    if (!m_camera || !m_videoOutput)
        return false;

    if (m_previewStarted)
        return true;

    if (!m_videoOutput->isReady())
        return true; // delay starting until the video output is ready

    Q_ASSERT(m_videoOutput->surfaceTexture() || m_videoOutput->surfaceHolder());

    if ((m_videoOutput->surfaceTexture() && !m_camera->setPreviewTexture(m_videoOutput->surfaceTexture()))
            || (m_videoOutput->surfaceHolder() && !m_camera->setPreviewDisplay(m_videoOutput->surfaceHolder())))
        return false;

    applyResolution(m_actualImageSettings.resolution());

    AndroidMultimediaUtils::enableOrientationListener(true);

    updateOrientation();
    m_camera->startPreview();
    m_previewStarted = true;
    m_videoOutput->start();

    return true;
}

QSize QAndroidCameraSession::getDefaultResolution() const
{
    const bool hasHighQualityProfile = AndroidCamcorderProfile::hasProfile(
                m_camera->cameraId(),
                AndroidCamcorderProfile::Quality(AndroidCamcorderProfile::QUALITY_HIGH));

    if (hasHighQualityProfile) {
        const AndroidCamcorderProfile camProfile = AndroidCamcorderProfile::get(
                    m_camera->cameraId(),
                    AndroidCamcorderProfile::Quality(AndroidCamcorderProfile::QUALITY_HIGH));

        return QSize(camProfile.getValue(AndroidCamcorderProfile::videoFrameWidth),
                     camProfile.getValue(AndroidCamcorderProfile::videoFrameHeight));
    }
    return QSize();
}

void QAndroidCameraSession::stopPreview()
{
    if (!m_camera || !m_previewStarted)
        return;

    AndroidMultimediaUtils::enableOrientationListener(false);

    m_camera->stopPreview();
    m_camera->setPreviewSize(QSize());
    m_camera->setPreviewTexture(0);
    m_camera->setPreviewDisplay(0);

    if (m_videoOutput) {
        m_videoOutput->stop();
        m_videoOutput->reset();
    }
    m_previewStarted = false;
}

void QAndroidCameraSession::setImageSettings(const QImageEncoderSettings &settings)
{
    if (m_requestedImageSettings == settings)
        return;

    m_requestedImageSettings = m_actualImageSettings = settings;

    applyImageSettings();

    if (m_readyForCapture)
        applyResolution(m_actualImageSettings.resolution());
}

void QAndroidCameraSession::enableRotation()
{
    m_rotationEnabled = true;
}

void QAndroidCameraSession::disableRotation()
{
    m_rotationEnabled = false;
}

void QAndroidCameraSession::updateOrientation()
{
    if (!m_camera || !m_rotationEnabled)
        return;

    m_camera->setDisplayOrientation(currentCameraRotation());
    applyResolution(m_actualImageSettings.resolution());
}


int QAndroidCameraSession::currentCameraRotation() const
{
    if (!m_camera)
        return 0;

    auto screen = QGuiApplication::primaryScreen();
    auto screenOrientation = screen->orientation();
    if (screenOrientation == Qt::PrimaryOrientation)
        screenOrientation = screen->primaryOrientation();

    int deviceOrientation = 0;
    switch (screenOrientation) {
    case Qt::PrimaryOrientation:
    case Qt::PortraitOrientation:
        break;
    case Qt::LandscapeOrientation:
        deviceOrientation = 90;
        break;
    case Qt::InvertedPortraitOrientation:
        deviceOrientation = 180;
        break;
    case Qt::InvertedLandscapeOrientation:
        deviceOrientation = 270;
        break;
    }

    int nativeCameraOrientation = m_camera->getNativeOrientation();

    int rotation;
    // subtract natural camera orientation and physical device orientation
    if (m_camera->getFacing() == AndroidCamera::CameraFacingFront) {
        rotation = (nativeCameraOrientation + deviceOrientation) % 360;
        rotation = (360 - rotation) % 360;  // compensate the mirror
    } else { // back-facing camera
        rotation = (nativeCameraOrientation - deviceOrientation + 360) % 360;
    }
    return rotation;
}

void QAndroidCameraSession::setPreviewFormat(AndroidCamera::ImageFormat format)
{
    if (format == AndroidCamera::UnknownImageFormat)
        return;

    m_camera->setPreviewFormat(format);
}

void QAndroidCameraSession::setPreviewCallback(PreviewCallback *callback)
{
    m_videoFrameCallbackMutex.lock();
    m_previewCallback = callback;
    if (m_camera)
        m_camera->notifyNewFrames(m_previewCallback);
    m_videoFrameCallbackMutex.unlock();
}

void QAndroidCameraSession::applyImageSettings()
{
    if (!m_camera)
        return;

    // only supported format right now.
    m_actualImageSettings.setFormat(QImageCapture::JPEG);

    const QSize requestedResolution = m_requestedImageSettings.resolution();
    const QList<QSize> supportedResolutions = m_camera->getSupportedPictureSizes();
    if (!requestedResolution.isValid()) {
        m_actualImageSettings.setResolution(getDefaultResolution());
    } else if (!supportedResolutions.contains(requestedResolution)) {
        // if the requested resolution is not supported, find the closest one
        int reqPixelCount = requestedResolution.width() * requestedResolution.height();
        QList<int> supportedPixelCounts;
        for (int i = 0; i < supportedResolutions.size(); ++i) {
            const QSize &s = supportedResolutions.at(i);
            supportedPixelCounts.append(s.width() * s.height());
        }
        int closestIndex = qt_findClosestValue(supportedPixelCounts, reqPixelCount);
        m_actualImageSettings.setResolution(supportedResolutions.at(closestIndex));
    }
    m_camera->setPictureSize(m_actualImageSettings.resolution());

    int jpegQuality = 100;
    switch (m_requestedImageSettings.quality()) {
    case QImageCapture::VeryLowQuality:
        jpegQuality = 20;
        break;
    case QImageCapture::LowQuality:
        jpegQuality = 40;
        break;
    case QImageCapture::NormalQuality:
        jpegQuality = 60;
        break;
    case QImageCapture::HighQuality:
        jpegQuality = 80;
        break;
    case QImageCapture::VeryHighQuality:
        jpegQuality = 100;
        break;
    }
    m_camera->setJpegQuality(jpegQuality);
}

bool QAndroidCameraSession::isReadyForCapture() const
{
    return isActive() && m_readyForCapture;
}

void QAndroidCameraSession::setReadyForCapture(bool ready)
{
    if (m_readyForCapture == ready)
        return;

    m_readyForCapture = ready;
    emit readyForCaptureChanged(ready);
}

int QAndroidCameraSession::captureImage()
{
    const int newImageCaptureId = m_currentImageCaptureId + 1;

    if (!isReadyForCapture()) {
        emit imageCaptureError(newImageCaptureId, QImageCapture::NotReadyError,
                               QPlatformImageCapture::msgCameraNotReady());
        return newImageCaptureId;
    }

    setReadyForCapture(false);

    m_currentImageCaptureId = newImageCaptureId;

    applyResolution(m_actualImageSettings.resolution());
    m_camera->takePicture();

    return m_currentImageCaptureId;
}

int QAndroidCameraSession::capture(const QString &fileName)
{
    m_currentImageCaptureFileName = fileName;
    m_imageCaptureToBuffer = false;
    return captureImage();
}

int QAndroidCameraSession::captureToBuffer()
{
    m_currentImageCaptureFileName.clear();
    m_imageCaptureToBuffer = true;
    return captureImage();
}

void QAndroidCameraSession::onCameraTakePictureFailed()
{
    emit imageCaptureError(m_currentImageCaptureId, QImageCapture::ResourceError,
                           tr("Failed to capture image"));

    // Preview needs to be restarted and the preview call back must be setup again
    m_camera->startPreview();
}

void QAndroidCameraSession::onCameraPictureExposed()
{
    if (!m_camera)
        return;

    emit imageExposed(m_currentImageCaptureId);
    m_camera->fetchLastPreviewFrame();
}

void QAndroidCameraSession::onLastPreviewFrameFetched(const QVideoFrame &frame)
{
    if (!m_camera)
        return;

    updateOrientation();

    (void)QtConcurrent::run(&QAndroidCameraSession::processPreviewImage, this,
                            m_currentImageCaptureId, frame, currentCameraRotation());
}

void QAndroidCameraSession::processPreviewImage(int id, const QVideoFrame &frame, int rotation)
{
    // Preview display of front-facing cameras is flipped horizontally, but the frame data
    // we get here is not. Flip it ourselves if the camera is front-facing to match what the user
    // sees on the viewfinder.
    QTransform transform;
    transform.rotate(rotation);

    if (m_camera->getFacing() == AndroidCamera::CameraFacingFront)
        transform.scale(-1, 1);

    emit imageCaptured(id, frame.toImage().transformed(transform));
}

void QAndroidCameraSession::onNewPreviewFrame(const QVideoFrame &frame)
{
    if (!m_camera)
        return;

    m_videoFrameCallbackMutex.lock();

    if (m_previewCallback)
        m_previewCallback->onFrameAvailable(frame);

    m_videoFrameCallbackMutex.unlock();
}

void QAndroidCameraSession::onCameraPictureCaptured(const QByteArray &bytes,
                                QVideoFrameFormat::PixelFormat format, QSize size,int bytesPerLine)
{
    if (m_imageCaptureToBuffer) {
        processCapturedImageToBuffer(m_currentImageCaptureId, bytes, format, size, bytesPerLine);
    } else {
        // Loading and saving the captured image can be slow, do it in a separate thread
        (void)QtConcurrent::run(&QAndroidCameraSession::processCapturedImage, this,
                         m_currentImageCaptureId, bytes, m_currentImageCaptureFileName);
    }

    // Preview needs to be restarted after taking a picture
    if (m_camera)
        m_camera->startPreview();
}

void QAndroidCameraSession::onCameraPreviewStarted()
{
    setReadyForCapture(true);
}

void QAndroidCameraSession::onCameraPreviewFailedToStart()
{
    if (isActive()) {
        Q_EMIT error(QCamera::CameraError, tr("Camera preview failed to start."));

        AndroidMultimediaUtils::enableOrientationListener(false);
        m_camera->setPreviewSize(QSize());
        m_camera->setPreviewTexture(0);
        if (m_videoOutput) {
            m_videoOutput->stop();
            m_videoOutput->reset();
        }
        m_previewStarted = false;

        setActive(false);
        setReadyForCapture(false);
    }
}

void QAndroidCameraSession::onCameraPreviewStopped()
{
    if (!m_previewStarted)
        setActive(false);
    setReadyForCapture(false);
}

void QAndroidCameraSession::processCapturedImage(int id, const QByteArray &bytes, const QString &fileName)
{
    const QString actualFileName = QMediaStorageLocation::generateFileName(
            fileName, QStandardPaths::PicturesLocation, QLatin1String("jpg"));
    QFile writer(actualFileName);
    if (!writer.open(QIODeviceBase::WriteOnly)) {
        const QString errorMessage = tr("File is not available: %1").arg(writer.errorString());
        emit imageCaptureError(id, QImageCapture::Error::ResourceError, errorMessage);
        return;
    }

    if (writer.write(bytes) < 0) {
        const QString errorMessage = tr("Could not save to file: %1").arg(writer.errorString());
        emit imageCaptureError(id, QImageCapture::Error::ResourceError, errorMessage);
        return;
    }

    writer.close();
    if (fileName.isEmpty() || QFileInfo(fileName).isRelative())
        AndroidMultimediaUtils::registerMediaFile(actualFileName);

    emit imageSaved(id, actualFileName);
}

void QAndroidCameraSession::processCapturedImageToBuffer(int id, const QByteArray &bytes,
                              QVideoFrameFormat::PixelFormat format, QSize size, int bytesPerLine)
{
    QVideoFrame frame(new QMemoryVideoBuffer(bytes, bytesPerLine), QVideoFrameFormat(size, format));
    emit imageAvailable(id, frame);
}

void QAndroidCameraSession::onVideoOutputReady(bool ready)
{
    if (ready && m_active)
        startPreview();
}

void QAndroidCameraSession::onApplicationStateChanged()
{

    switch (QGuiApplication::applicationState()) {
    case Qt::ApplicationInactive:
        if (!m_keepActive && m_active) {
            m_savedState = m_active;
            setActive(false);
            m_isStateSaved = true;
        }
        break;
    case Qt::ApplicationActive:
        if (m_isStateSaved) {
            setActive(m_savedState);
            m_isStateSaved = false;
        }
        break;
    default:
        break;
    }
}

void QAndroidCameraSession::setKeepAlive(bool keepAlive)
{
    m_keepActive = keepAlive;
}

void QAndroidCameraSession::setVideoSink(QVideoSink *sink)
{
    if (m_sink == sink)
        return;

    if (m_sink)
        disconnect(m_retryPreviewConnection);

    m_sink = sink;

    if (m_sink)
        m_retryPreviewConnection =
                connect(m_sink->platformVideoSink(), &QPlatformVideoSink::rhiChanged, this, [&]()
                        {
                            if (m_active) {
                                setActive(false);
                                setActive(true);
                            }
                        }, Qt::DirectConnection);
    if (m_sink) {
        delete m_textureOutput;
        m_textureOutput = nullptr;

        m_textureOutput = new QAndroidTextureVideoOutput(m_sink, this);
    }

    setVideoOutput(m_textureOutput);
}

QT_END_NAMESPACE

#include "moc_qandroidcamerasession_p.cpp"
