// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohoscamerasession_p.h"

#include "common/qohosvideooutput_p.h"
#include "qohosglobal_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>
#include <QtCore/qset.h>
#include <QtCore/qthread.h>
#include <QtCore/qthreadpool.h>
#include <QtGui/qimage.h>
#include <QtMultimedia/qvideosink.h>

#include <fcntl.h>
#include <unistd.h>

#include <private/qcameradevice_p.h>
#include <private/qmediastoragelocation_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qvideoframe_p.h>

#include <multimedia/image_framework/image/image_native.h>
#include <multimedia/image_framework/image/image_packer_native.h>
#include <multimedia/image_framework/image/image_source_native.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>

QT_BEGIN_NAMESPACE

namespace {

constexpr int32_t kImageReceiverCapacity = 4;
constexpr const char *kJpegMimeType = "image/jpeg";

// OHOS phones only allow one Camera_Input open at a time. Other platforms
// auto-preempt the previously-open camera when a new one is opened, but on
// OHOS OH_CameraInput_Open fails with CAMERA_OPERATION_NOT_ALLOWED if any
// other input is still alive in this process. Track live sessions so a
// starting session can stop the others first.
//
// The mutex is recursive because stopSession() -> releaseSession() removes
// the session from liveSessions() under the same lock, and preemption iterates
// while holding it (so a pointer can't be destroyed mid-iteration).
QRecursiveMutex &liveSessionMutex()
{
    static QRecursiveMutex m;
    return m;
}
QSet<QOhosCameraSession *> &liveSessions()
{
    static QSet<QOhosCameraSession *> s;
    return s;
}

QVideoFrameFormat::PixelFormat pixelFormatFor(Camera_Format format)
{
    switch (format) {
    case CAMERA_FORMAT_RGBA_8888:
        return QVideoFrameFormat::Format_RGBA8888;
    case CAMERA_FORMAT_YUV_420_SP:
        return QVideoFrameFormat::Format_NV12;
    case CAMERA_FORMAT_JPEG:
        return QVideoFrameFormat::Format_Jpeg;
    default:
        break;
    }
    return QVideoFrameFormat::Format_Invalid;
}

int qualityToInt(QImageCapture::Quality q)
{
    switch (q) {
    case QImageCapture::VeryLowQuality:
        return 25;
    case QImageCapture::LowQuality:
        return 50;
    case QImageCapture::HighQuality:
        return 90;
    case QImageCapture::VeryHighQuality:
        return 100;
    case QImageCapture::NormalQuality:
    default:
        return 75;
    }
}

QByteArray encodeNativeImageToJpeg(OH_ImageNative *image, int quality)
{
    uint32_t *components = nullptr;
    size_t componentCount = 0;
    if (OH_ImageNative_GetComponentTypes(image, nullptr, &componentCount) != IMAGE_SUCCESS
        || componentCount == 0) {
        return {};
    }
    std::vector<uint32_t> types(componentCount);
    components = types.data();
    if (OH_ImageNative_GetComponentTypes(image, &components, &componentCount) != IMAGE_SUCCESS)
        return {};

    OH_NativeBuffer *nativeBuffer = nullptr;
    if (OH_ImageNative_GetByteBuffer(image, types[0], &nativeBuffer) != IMAGE_SUCCESS
        || !nativeBuffer) {
        return {};
    }

    OH_NativeBuffer_Config bufferConfig{};
    OH_NativeBuffer_GetConfig(nativeBuffer, &bufferConfig);

    void *mapped = nullptr;
    if (OH_NativeBuffer_Map(nativeBuffer, &mapped) != 0 || !mapped)
        return {};

    size_t bufferSize = 0;
    OH_ImageNative_GetBufferSize(image, types[0], &bufferSize);

    QByteArray result;
    OH_ImageSourceNative *source = nullptr;
    if (OH_ImageSourceNative_CreateFromData(static_cast<uint8_t *>(mapped), bufferSize, &source)
                == IMAGE_SUCCESS
        && source) {
        OH_ImagePackerNative *packer = nullptr;
        if (OH_ImagePackerNative_Create(&packer) == IMAGE_SUCCESS && packer) {
            OH_PackingOptions *options = nullptr;
            if (OH_PackingOptions_Create(&options) == IMAGE_SUCCESS && options) {
                Image_MimeType mime{ const_cast<char *>(kJpegMimeType),
                                     std::strlen(kJpegMimeType) };
                OH_PackingOptions_SetMimeType(options, &mime);
                OH_PackingOptions_SetQuality(options, uint32_t(quality));

                size_t outSize = bufferSize * 2 + 1024;
                result.resize(int(outSize));
                if (OH_ImagePackerNative_PackToDataFromImageSource(
                            packer, options, source,
                            reinterpret_cast<uint8_t *>(result.data()), &outSize)
                    == IMAGE_SUCCESS) {
                    result.resize(int(outSize));
                } else {
                    result.clear();
                }
                OH_PackingOptions_Release(options);
            }
            OH_ImagePackerNative_Release(packer);
        }
        OH_ImageSourceNative_Release(source);
    }

    OH_NativeBuffer_Unmap(nativeBuffer);
    return result;
}

void imageArriveCallbackTrampoline(OH_ImageReceiverNative * /*receiver*/, void *userData)
{
    auto *session = static_cast<QOhosCameraSession *>(userData);
    if (!session)
        return;
    QMetaObject::invokeMethod(session, &QOhosCameraSession::onCapturedImageAvailable,
                              Qt::QueuedConnection);
}

} // namespace

QOhosCameraSession::QOhosCameraSession(QObject *parent) : QObject(parent) { }

QOhosCameraSession::~QOhosCameraSession()
{
    {
        QMutexLocker lock{ &liveSessionMutex() };
        liveSessions().remove(this);
    }
    releaseSession();
    if (m_supportedDevices) {
        OH_CameraManager_DeleteSupportedCameras(m_manager, m_supportedDevices,
                                                m_supportedDeviceCount);
        m_supportedDevices = nullptr;
    }
    if (m_manager) {
        OH_Camera_DeleteCameraManager(m_manager);
        m_manager = nullptr;
    }
}

void QOhosCameraSession::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    const bool wasActive = m_active;
    if (wasActive)
        setActive(false);
    m_cameraDevice = camera;
    if (wasActive)
        setActive(true);
}

void QOhosCameraSession::setCameraFormat(const QCameraFormat &format)
{
    m_cameraFormat = format;
}

void QOhosCameraSession::setVideoSink(QVideoSink *sink)
{
    if (m_videoSink == sink)
        return;
    m_videoSink = sink;
    if (m_videoOutput)
        m_videoOutput.reset();
    m_videoOutput = std::make_unique<QOhosVideoOutput>(
            sink, QOhosVideoOutput::ContentSource::Camera, this);
    connect(m_videoOutput.get(), &QOhosVideoOutput::surfaceReady, this,
            &QOhosCameraSession::onSurfaceReady);
}

void QOhosCameraSession::setActive(bool active)
{
    if (m_active == active)
        return;
    if (active) {
        if (!startSession()) {
            m_pendingStart = true;
            return;
        }
    } else {
        m_pendingStart = false;
        stopSession();
    }
    m_active = active;
    emit activeChanged(active);
    emitReadyForCaptureChanged();
}

void QOhosCameraSession::setImageSettings(const QImageEncoderSettings &settings)
{
    m_imageSettings = settings;
}

int QOhosCameraSession::capture(const QString &fileName, bool toBuffer)
{
    const int id = ++m_lastCaptureId;
    if (!m_active || !m_photoOutput) {
        emit imageCaptureError(id, QImageCapture::NotReadyError,
                               tr("Camera not ready for capture"));
        return id;
    }
    if (m_captureInProgress) {
        emit imageCaptureError(id, QImageCapture::NotReadyError,
                               tr("Capture already in progress"));
        return id;
    }

    m_pendingCaptureId = id;
    m_pendingCaptureFileName = fileName;
    m_pendingCaptureToBuffer = toBuffer;
    m_captureInProgress = true;
    emitReadyForCaptureChanged();

    if (OH_PhotoOutput_Capture(m_photoOutput) != CAMERA_OK) {
        m_captureInProgress = false;
        emit imageCaptureError(id, QImageCapture::ResourceError,
                               tr("OH_PhotoOutput_Capture failed"));
        emitReadyForCaptureChanged();
    }
    return id;
}

void QOhosCameraSession::onSurfaceReady()
{
    if (m_pendingStart && !m_active) {
        m_pendingStart = false;
        if (startSession()) {
            m_active = true;
            emit activeChanged(true);
            emitReadyForCaptureChanged();
        }
        return;
    }

    // Session is already running headless because the sink wasn't ready at
    // start time. Now that we have a surface, restart with preview attached.
    if (m_active && !m_previewOutput && m_videoOutput
        && !m_videoOutput->surfaceId().isEmpty()) {
        m_active = false;
        stopSession();
        if (startSession()) {
            m_active = true;
            emit activeChanged(true);
            emitReadyForCaptureChanged();
        }
    }
}

void QOhosCameraSession::onCapturedImageAvailable()
{
    if (!m_imageReceiver)
        return;

    OH_ImageNative *image = nullptr;
    if (OH_ImageReceiverNative_ReadLatestImage(m_imageReceiver, &image) != IMAGE_SUCCESS
        || !image) {
        m_captureInProgress = false;
        emit imageCaptureError(m_pendingCaptureId, QImageCapture::ResourceError,
                               tr("Failed to read captured image"));
        emitReadyForCaptureChanged();
        return;
    }

    const int quality = qualityToInt(m_imageSettings.quality());
    QByteArray jpegBytes = encodeNativeImageToJpeg(image, quality);
    OH_ImageNative_Release(image);

    const int id = m_pendingCaptureId;
    const QString fileName = m_pendingCaptureFileName;
    const bool toBuffer = m_pendingCaptureToBuffer;
    m_pendingCaptureFileName.clear();
    m_pendingCaptureToBuffer = false;
    m_pendingCaptureId = 0;
    m_captureInProgress = false;

    if (jpegBytes.isEmpty()) {
        emit imageCaptureError(id, QImageCapture::FormatError,
                               tr("Failed to encode captured image"));
        emitReadyForCaptureChanged();
        return;
    }

    QImage preview = QImage::fromData(jpegBytes, "JPEG");
    emit imageExposed(id);
    emit imageCaptured(id, preview);

    if (toBuffer) {
        QVideoFrame buffer = QVideoFramePrivate::createFrame(
                std::make_unique<QMemoryVideoBuffer>(QByteArray(jpegBytes),
                                                     preview.bytesPerLine()),
                QVideoFrameFormat(preview.size(), QVideoFrameFormat::Format_Jpeg));
        emit imageAvailable(id, buffer);
        emitReadyForCaptureChanged();
        return;
    }

    // captureToFile: persist to disk and emit imageSaved. Empty filename means
    // "let the platform pick" — Qt apps default to PicturesLocation/IMG_<n>.jpg.
    // Mirrors the Android backend.
    const QImageCapture::FileFormat targetFormat = m_imageSettings.format();
    const QString defaultExt = [&]() -> QString {
        switch (targetFormat) {
        case QImageCapture::PNG:  return QStringLiteral("png");
        case QImageCapture::WebP: return QStringLiteral("webp");
        case QImageCapture::Tiff: return QStringLiteral("tiff");
        case QImageCapture::JPEG:
        case QImageCapture::UnspecifiedFormat:
        default:                  return QStringLiteral("jpg");
        }
    }();
    const char *qImageFormat = [&]() -> const char * {
        switch (targetFormat) {
        case QImageCapture::PNG:  return "PNG";
        case QImageCapture::WebP: return "WEBP";
        case QImageCapture::Tiff: return "TIFF";
        default:                  return nullptr; // JPEG: stream raw bytes
        }
    }();
    const QString resolved = QMediaStorageLocation::generateFileName(
            fileName, QStandardPaths::PicturesLocation, defaultExt);
    bool saved = false;
    if (qImageFormat) {
        saved = preview.save(resolved, qImageFormat, quality);
    } else {
        QFile out(resolved);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(jpegBytes);
            out.close();
            saved = true;
        }
    }
    if (saved) {
        emit imageSaved(id, resolved);
    } else {
        emit imageCaptureError(id, QImageCapture::ResourceError,
                               tr("Could not save captured image to: %1").arg(resolved));
    }

    emitReadyForCaptureChanged();
}

void QOhosCameraSession::emitReadyForCaptureChanged()
{
    const bool ready = isReadyForCapture();
    if (m_lastReadyForCapture && *m_lastReadyForCapture == ready)
        return;
    m_lastReadyForCapture = ready;
    emit readyForCaptureChanged(ready);
}

bool QOhosCameraSession::ensureManager()
{
    if (m_manager)
        return true;
    if (OH_Camera_GetCameraManager(&m_manager) != CAMERA_OK || !m_manager) {
        qCWarning(qLcOhosMediaPlugin) << "OH_Camera_GetCameraManager failed";
        return false;
    }
    if (OH_CameraManager_GetSupportedCameras(m_manager, &m_supportedDevices,
                                             &m_supportedDeviceCount) != CAMERA_OK) {
        qCWarning(qLcOhosMediaPlugin) << "GetSupportedCameras failed";
        m_supportedDevices = nullptr;
        m_supportedDeviceCount = 0;
    }
    return true;
}

Camera_Device *QOhosCameraSession::findDevice(const QByteArray &id)
{
    if (!m_supportedDevices)
        return nullptr;
    for (uint32_t i = 0; i < m_supportedDeviceCount; ++i) {
        if (m_supportedDevices[i].cameraId && id == m_supportedDevices[i].cameraId)
            return &m_supportedDevices[i];
    }
    return nullptr;
}

bool QOhosCameraSession::createPhotoPath(Camera_OutputCapability *caps,
                                         Camera_Profile *previewProfile)
{
    if (!caps || caps->photoProfilesSize == 0)
        return false;

    Camera_Profile *photoProfile = caps->photoProfiles[0];
    if (m_imageSettings.resolution().isValid()) {
        const QSize wanted = m_imageSettings.resolution();
        for (uint32_t i = 0; i < caps->photoProfilesSize; ++i) {
            Camera_Profile *p = caps->photoProfiles[i];
            if (!p)
                continue;
            if (int(p->size.width) == wanted.width()
                && int(p->size.height) == wanted.height()) {
                photoProfile = p;
                break;
            }
        }
    } else if (previewProfile) {
        for (uint32_t i = 0; i < caps->photoProfilesSize; ++i) {
            Camera_Profile *p = caps->photoProfiles[i];
            if (!p)
                continue;
            if (p->size.width == previewProfile->size.width
                && p->size.height == previewProfile->size.height) {
                photoProfile = p;
                break;
            }
        }
    }
    if (!photoProfile)
        return false;

    if (OH_ImageReceiverOptions_Create(&m_imageReceiverOptions) != IMAGE_SUCCESS
        || !m_imageReceiverOptions) {
        return false;
    }
    Image_Size size{ uint32_t(photoProfile->size.width), uint32_t(photoProfile->size.height) };
    OH_ImageReceiverOptions_SetSize(m_imageReceiverOptions, size);
    OH_ImageReceiverOptions_SetCapacity(m_imageReceiverOptions, kImageReceiverCapacity);

    if (OH_ImageReceiverNative_Create(m_imageReceiverOptions, &m_imageReceiver) != IMAGE_SUCCESS
        || !m_imageReceiver) {
        return false;
    }

    OH_ImageReceiverNative_OnImageArrive(m_imageReceiver, imageArriveCallbackTrampoline, this);

    uint64_t surfaceIdNum = 0;
    if (OH_ImageReceiverNative_GetReceivingSurfaceId(m_imageReceiver, &surfaceIdNum) != IMAGE_SUCCESS
        || surfaceIdNum == 0) {
        return false;
    }
    const QByteArray surfaceId = QByteArray::number(qulonglong(surfaceIdNum));

    if (OH_CameraManager_CreatePhotoOutput(m_manager, photoProfile, surfaceId.constData(),
                                           &m_photoOutput) != CAMERA_OK
        || !m_photoOutput) {
        return false;
    }
    return true;
}

void QOhosCameraSession::destroyPhotoPath()
{
    if (m_photoOutput) {
        OH_PhotoOutput_Release(m_photoOutput);
        m_photoOutput = nullptr;
    }
    if (m_imageReceiver) {
        OH_ImageReceiverNative_OffImageArrive(m_imageReceiver, imageArriveCallbackTrampoline);
        OH_ImageReceiverNative_Release(m_imageReceiver);
        m_imageReceiver = nullptr;
    }
    if (m_imageReceiverOptions) {
        OH_ImageReceiverOptions_Release(m_imageReceiverOptions);
        m_imageReceiverOptions = nullptr;
    }
}

bool QOhosCameraSession::startSession()
{
    // Phones only let one Camera_Input be open at a time. Preempt any other
    // live session in this process before attempting to open ours; otherwise
    // OH_CameraInput_Open returns CAMERA_OPERATION_NOT_ALLOWED.
    {
        QMutexLocker lock{ &liveSessionMutex() };
        const auto sessions = liveSessions();
        for (auto *other : sessions) {
            if (other != this)
                other->stopSession();
        }
    }

    // OHOS capture sessions require at least one preview output to commit, so
    // we always produce a surface — backed by a real QVideoSink RHI when one is
    // attached, or an internal offscreen GLES2 RHI otherwise. surfaceReady will
    // re-attach later if a sink with a live RHI shows up.
    if (!m_videoOutput) {
        m_videoOutput = std::make_unique<QOhosVideoOutput>(
                nullptr, QOhosVideoOutput::ContentSource::Camera, this);
        connect(m_videoOutput.get(), &QOhosVideoOutput::surfaceReady, this,
                &QOhosCameraSession::onSurfaceReady);
    }
    QByteArray previewSurfaceId = m_videoOutput->surfaceId();

    if (!ensureManager())
        return false;

    Camera_Device *device = findDevice(m_cameraDevice.id());
    if (!device && m_supportedDeviceCount > 0)
        device = &m_supportedDevices[0];
    if (!device) {
        qCWarning(qLcOhosMediaPlugin) << "No camera device available";
        return false;
    }

    Camera_Profile *previewProfile = nullptr;
    Camera_OutputCapability *caps = nullptr;
    if (OH_CameraManager_GetSupportedCameraOutputCapability(m_manager, device, &caps) == CAMERA_OK
        && caps && caps->previewProfilesSize > 0) {
        previewProfile = caps->previewProfiles[0];
        const bool haveRequest = !m_cameraFormat.isNull();
        const QSize requestedSize = haveRequest ? m_cameraFormat.resolution() : QSize{};
        const QVideoFrameFormat::PixelFormat requestedPixel =
                haveRequest ? m_cameraFormat.pixelFormat() : QVideoFrameFormat::Format_Invalid;
        for (uint32_t i = 0; i < caps->previewProfilesSize; ++i) {
            Camera_Profile *p = caps->previewProfiles[i];
            if (!p)
                continue;
            const bool sizeMatches = !haveRequest
                    || (int(p->size.width) == requestedSize.width()
                        && int(p->size.height) == requestedSize.height());
            const bool formatMatches = !haveRequest
                    || requestedPixel == QVideoFrameFormat::Format_Invalid
                    || requestedPixel == pixelFormatFor(p->format);
            if (sizeMatches && formatMatches) {
                previewProfile = p;
                if (haveRequest)
                    break;
                if (p->format == CAMERA_FORMAT_YUV_420_SP && p->size.width == 1280
                    && p->size.height == 720)
                    break;
            }
        }
    }

    if (!previewProfile) {
        qCWarning(qLcOhosMediaPlugin) << "No preview profile available";
        if (caps)
            OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);
        return false;
    }

    if (OH_CameraManager_CreateCameraInput(m_manager, device, &m_cameraInput) != CAMERA_OK
        || !m_cameraInput) {
        qCWarning(qLcOhosMediaPlugin) << "CreateCameraInput failed";
        OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);
        return false;
    }

    // OH_CameraInput_Open can briefly fail with CAMERA_CONFLICT_CAMERA right
    // after a sibling Camera_Input was released — the camera service finishes
    // tearing down its session asynchronously. Poll for up to ~1 s.
    {
        Camera_ErrorCode err = CAMERA_OK;
        constexpr int kMaxAttempts = 20;
        constexpr int kBackoffMs = 50;
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
            err = OH_CameraInput_Open(m_cameraInput);
            if (err == CAMERA_OK)
                break;
            // CAMERA_OPERATION_NOT_ALLOWED is returned when another
            // Camera_Input is still alive in this process. Force-stop any
            // other live QOhosCameraSession and retry.
            if (err == CAMERA_OPERATION_NOT_ALLOWED) {
                QSet<QOhosCameraSession *> snapshot;
                {
                    QMutexLocker lock{ &liveSessionMutex() };
                    snapshot = liveSessions();
                }
                for (auto *other : snapshot) {
                    if (other != this)
                        other->stopSession();
                }
            } else if (err != CAMERA_CONFLICT_CAMERA && err != CAMERA_DEVICE_PREEMPTED) {
                break;
            }
            QThread::msleep(kBackoffMs);
        }
        if (err != CAMERA_OK) {
            qCWarning(qLcOhosMediaPlugin) << "CameraInput_Open failed:" << err;
            OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);
            releaseSession();
            return false;
        }
        QMutexLocker lock{ &liveSessionMutex() };
        liveSessions().insert(this);
    }

    if (m_videoOutput && !previewSurfaceId.isEmpty()) {
        m_videoOutput->setVideoSize(
                QSize{ int(previewProfile->size.width), int(previewProfile->size.height) });
        if (OH_CameraManager_CreatePreviewOutput(m_manager, previewProfile,
                                                 previewSurfaceId.constData(), &m_previewOutput)
                    != CAMERA_OK
            || !m_previewOutput) {
            qCWarning(qLcOhosMediaPlugin) << "CreatePreviewOutput failed";
            OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);
            releaseSession();
            return false;
        }
    }

    const bool hasPhoto = createPhotoPath(caps, previewProfile);
    if (!hasPhoto)
        qCWarning(qLcOhosMediaPlugin) << "Photo output unavailable; image capture disabled";

    OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);

    if (!m_previewOutput && !m_photoOutput) {
        qCWarning(qLcOhosMediaPlugin) << "No capture outputs available";
        releaseSession();
        return false;
    }

    if (OH_CameraManager_CreateCaptureSession(m_manager, &m_captureSession) != CAMERA_OK
        || !m_captureSession) {
        qCWarning(qLcOhosMediaPlugin) << "CreateCaptureSession failed";
        releaseSession();
        return false;
    }

    OH_CaptureSession_BeginConfig(m_captureSession);
    OH_CaptureSession_AddInput(m_captureSession, m_cameraInput);
    if (m_previewOutput)
        OH_CaptureSession_AddPreviewOutput(m_captureSession, m_previewOutput);
    if (m_photoOutput)
        OH_CaptureSession_AddPhotoOutput(m_captureSession, m_photoOutput);
    if (OH_CaptureSession_CommitConfig(m_captureSession) != CAMERA_OK) {
        qCWarning(qLcOhosMediaPlugin) << "CommitConfig failed";
        releaseSession();
        return false;
    }

    if (OH_CaptureSession_Start(m_captureSession) != CAMERA_OK) {
        qCWarning(qLcOhosMediaPlugin) << "CaptureSession_Start failed";
        releaseSession();
        return false;
    }

    return true;
}

void QOhosCameraSession::stopSession()
{
    if (m_captureSession) {
        OH_CaptureSession_Stop(m_captureSession);
    }
    releaseSession();
}

void QOhosCameraSession::releaseSession()
{
    destroyRecorder();
    detachVideoOutput();
    destroyPhotoPath();
    if (m_captureSession) {
        OH_CaptureSession_Release(m_captureSession);
        m_captureSession = nullptr;
    }
    if (m_previewOutput) {
        OH_PreviewOutput_Release(m_previewOutput);
        m_previewOutput = nullptr;
    }
    if (m_cameraInput) {
        OH_CameraInput_Close(m_cameraInput);
        OH_CameraInput_Release(m_cameraInput);
        m_cameraInput = nullptr;
    }
    QMutexLocker lock{ &liveSessionMutex() };
    liveSessions().remove(this);
}

namespace {

OH_AVRecorder_CodecMimeType videoCodecToOhos(QMediaFormat::VideoCodec codec)
{
    switch (codec) {
    case QMediaFormat::VideoCodec::H265:
        return AVRECORDER_VIDEO_HEVC;
    case QMediaFormat::VideoCodec::MPEG4:
        return AVRECORDER_VIDEO_MPEG4;
    case QMediaFormat::VideoCodec::H264:
    case QMediaFormat::VideoCodec::Unspecified:
    default:
        return AVRECORDER_VIDEO_AVC;
    }
}

OH_AVRecorder_CodecMimeType audioCodecToOhos(QMediaFormat::AudioCodec codec)
{
    switch (codec) {
    case QMediaFormat::AudioCodec::MP3:
        return AVRECORDER_AUDIO_MP3;
    case QMediaFormat::AudioCodec::AAC:
    case QMediaFormat::AudioCodec::Unspecified:
    default:
        return AVRECORDER_AUDIO_AAC;
    }
}

OH_AVRecorder_ContainerFormatType containerToOhos(QMediaFormat::FileFormat fmt)
{
    switch (fmt) {
    case QMediaFormat::AAC:
        return AVRECORDER_CFT_AAC;
    case QMediaFormat::MP3:
        return AVRECORDER_CFT_MP3;
    case QMediaFormat::Wave:
        return AVRECORDER_CFT_WAV;
    case QMediaFormat::Mpeg4Audio:
        return AVRECORDER_CFT_MPEG_4A;
    case QMediaFormat::MPEG4:
    case QMediaFormat::UnspecifiedFormat:
    default:
        return AVRECORDER_CFT_MPEG_4;
    }
}

int qualityToVideoBitrate(QMediaRecorder::Quality q, const QSize &resolution)
{
    const int pixels = qMax(1, resolution.width() * resolution.height());
    const double bpp = [&]() {
        switch (q) {
        case QMediaRecorder::VeryLowQuality: return 0.05;
        case QMediaRecorder::LowQuality:     return 0.1;
        case QMediaRecorder::HighQuality:    return 0.25;
        case QMediaRecorder::VeryHighQuality:return 0.4;
        case QMediaRecorder::NormalQuality:
        default:                             return 0.15;
        }
    }();
    return int(pixels * 30 * bpp);
}

int qualityToAudioBitrate(QMediaRecorder::Quality q)
{
    switch (q) {
    case QMediaRecorder::VeryLowQuality: return 32000;
    case QMediaRecorder::LowQuality:     return 64000;
    case QMediaRecorder::HighQuality:    return 192000;
    case QMediaRecorder::VeryHighQuality:return 256000;
    case QMediaRecorder::NormalQuality:
    default:                             return 128000;
    }
}

} // namespace

bool QOhosCameraSession::findVideoProfile(const QMediaEncoderSettings &settings,
                                          Camera_VideoProfile *out)
{
    if (!m_manager || !out)
        return false;
    Camera_Device *device = findDevice(m_cameraDevice.id());
    if (!device && m_supportedDeviceCount > 0)
        device = &m_supportedDevices[0];
    if (!device)
        return false;

    Camera_OutputCapability *caps = nullptr;
    if (OH_CameraManager_GetSupportedCameraOutputCapability(m_manager, device, &caps) != CAMERA_OK
        || !caps || caps->videoProfilesSize == 0) {
        if (caps)
            OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);
        return false;
    }

    Camera_VideoProfile *chosen = nullptr;
    const QSize wanted = settings.videoResolution();
    if (wanted.isValid()) {
        for (uint32_t i = 0; i < caps->videoProfilesSize; ++i) {
            Camera_VideoProfile *p = caps->videoProfiles[i];
            if (!p)
                continue;
            if (int(p->size.width) == wanted.width()
                && int(p->size.height) == wanted.height()) {
                chosen = p;
                break;
            }
        }
    }
    if (!chosen) {
        for (uint32_t i = 0; i < caps->videoProfilesSize; ++i) {
            Camera_VideoProfile *p = caps->videoProfiles[i];
            if (!p)
                continue;
            if (p->size.width == 1280 && p->size.height == 720
                && p->format == CAMERA_FORMAT_YUV_420_SP) {
                chosen = p;
                break;
            }
        }
    }
    if (!chosen)
        chosen = caps->videoProfiles[0];
    *out = *chosen;
    OH_CameraManager_DeleteSupportedCameraOutputCapability(m_manager, caps);
    return true;
}

bool QOhosCameraSession::attachVideoOutput(const Camera_VideoProfile &profile,
                                           const QByteArray &surfaceId)
{
    if (!m_captureSession)
        return false;
    if (OH_CameraManager_CreateVideoOutput(m_manager, &profile, surfaceId.constData(),
                                           &m_videoOutputCamera) != CAMERA_OK
        || !m_videoOutputCamera) {
        return false;
    }

    OH_CaptureSession_Stop(m_captureSession);
    OH_CaptureSession_BeginConfig(m_captureSession);
    OH_CaptureSession_AddVideoOutput(m_captureSession, m_videoOutputCamera);
    if (OH_CaptureSession_CommitConfig(m_captureSession) != CAMERA_OK) {
        OH_VideoOutput_Release(m_videoOutputCamera);
        m_videoOutputCamera = nullptr;
        OH_CaptureSession_Start(m_captureSession);
        return false;
    }
    if (OH_CaptureSession_Start(m_captureSession) != CAMERA_OK)
        return false;
    if (OH_VideoOutput_Start(m_videoOutputCamera) != CAMERA_OK)
        return false;
    return true;
}

void QOhosCameraSession::detachVideoOutput()
{
    if (!m_videoOutputCamera)
        return;
    OH_VideoOutput_Stop(m_videoOutputCamera);
    if (m_captureSession) {
        OH_CaptureSession_Stop(m_captureSession);
        OH_CaptureSession_BeginConfig(m_captureSession);
        OH_CaptureSession_RemoveVideoOutput(m_captureSession, m_videoOutputCamera);
        OH_CaptureSession_CommitConfig(m_captureSession);
        OH_CaptureSession_Start(m_captureSession);
    }
    OH_VideoOutput_Release(m_videoOutputCamera);
    m_videoOutputCamera = nullptr;
}

void QOhosCameraSession::recorderStateCallback(OH_AVRecorder * /*recorder*/,
                                                OH_AVRecorder_State state,
                                                OH_AVRecorder_StateChangeReason /*reason*/,
                                                void *userData)
{
    auto *self = static_cast<QOhosCameraSession *>(userData);
    if (!self)
        return;
    QMetaObject::invokeMethod(self, "onRecorderStateNotification", Qt::QueuedConnection,
                              Q_ARG(int, int(state)));
}

void QOhosCameraSession::recorderErrorCallback(OH_AVRecorder * /*recorder*/, int32_t errorCode,
                                                const char *errorMsg, void *userData)
{
    auto *self = static_cast<QOhosCameraSession *>(userData);
    if (!self)
        return;
    QMetaObject::invokeMethod(self, "onRecorderErrorNotification", Qt::QueuedConnection,
                              Q_ARG(int, errorCode),
                              Q_ARG(QString, QString::fromUtf8(errorMsg ? errorMsg : "")));
}

void QOhosCameraSession::onRecorderStateNotification(int state)
{
    QMediaRecorder::RecorderState mapped = m_recorderState;
    switch (state) {
    case AVRECORDER_STARTED:
        mapped = QMediaRecorder::RecordingState;
        break;
    case AVRECORDER_PAUSED:
        mapped = QMediaRecorder::PausedState;
        break;
    case AVRECORDER_STOPPED:
    case AVRECORDER_IDLE:
    case AVRECORDER_RELEASED:
        mapped = QMediaRecorder::StoppedState;
        break;
    case AVRECORDER_ERROR:
        mapped = QMediaRecorder::StoppedState;
        break;
    default:
        return;
    }
    if (mapped == m_recorderState)
        return;
    m_recorderState = mapped;
    emit recorderStateChanged(int(mapped));
}

void QOhosCameraSession::onRecorderErrorNotification(int code, const QString &message)
{
    emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                               message.isEmpty()
                                       ? tr("Recorder error %1").arg(code)
                                       : message);
}

qint64 QOhosCameraSession::recorderDuration() const
{
    if (m_recorderState == QMediaRecorder::StoppedState)
        return 0;
    if (m_recorderState == QMediaRecorder::PausedState)
        return m_recorderPausedMs;
    if (!m_recorderTimer.isValid())
        return m_recorderPausedMs;
    return m_recorderPausedMs + (m_recorderTimer.elapsed() - m_recorderResumeStartMs);
}

bool QOhosCameraSession::startRecording(const QMediaEncoderSettings &settings,
                                         const QString &location)
{
    if (m_recorder) {
        emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                   tr("Recording already in progress"));
        return false;
    }

    // Audio-only recording when no camera is attached: skip the camera
    // pipeline and let OH_AVRecorder capture audio directly.
    const bool videoEnabled = m_active && m_captureSession;
    Camera_VideoProfile videoProfile{};
    if (videoEnabled) {
        if (!findVideoProfile(settings, &videoProfile)) {
            emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                       tr("No matching camera video profile"));
            return false;
        }
    }

    m_recorder = OH_AVRecorder_Create();
    if (!m_recorder) {
        emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                   tr("OH_AVRecorder_Create failed"));
        return false;
    }

    OH_AVRecorder_SetStateCallback(m_recorder, recorderStateCallback, this);
    OH_AVRecorder_SetErrorCallback(m_recorder, recorderErrorCallback, this);

    QString resolved = location;
    if (QFileInfo(resolved).suffix().isEmpty()) {
        const QString suffix = settings.preferredSuffix();
        if (!suffix.isEmpty())
            resolved.append(QLatin1Char('.')).append(suffix);
        else
            resolved.append(QStringLiteral(".mp4"));
    }

    QByteArray urlBytes = QStringLiteral("fd://").toUtf8();
    int fd = ::open(QFile::encodeName(resolved).constData(),
                    O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                   tr("Could not open output file: %1").arg(resolved));
        destroyRecorder();
        return false;
    }
    urlBytes.append(QByteArray::number(fd));

    OH_AVRecorder_Config config{};
    config.audioSourceType = AVRECORDER_MIC;
    config.profile.audioBitrate = settings.audioBitRate() > 0
            ? settings.audioBitRate() : qualityToAudioBitrate(settings.quality());
    config.profile.audioChannels = settings.audioChannelCount() > 0
            ? settings.audioChannelCount() : 2;
    config.profile.audioCodec = audioCodecToOhos(settings.audioCodec());
    config.profile.audioSampleRate = settings.audioSampleRate() > 0
            ? settings.audioSampleRate() : 48000;
    config.profile.fileFormat = containerToOhos(settings.fileFormat());
    if (videoEnabled) {
        const QSize videoSize{ int(videoProfile.size.width), int(videoProfile.size.height) };
        config.videoSourceType = AVRECORDER_SURFACE_YUV;
        config.profile.videoBitrate = settings.videoBitRate() > 0
                ? settings.videoBitRate() : qualityToVideoBitrate(settings.quality(), videoSize);
        config.profile.videoCodec = videoCodecToOhos(settings.videoCodec());
        config.profile.videoFrameWidth = videoSize.width();
        config.profile.videoFrameHeight = videoSize.height();
        config.profile.videoFrameRate = settings.videoFrameRate() > 0
                ? int(settings.videoFrameRate()) : 30;
    }
    config.profile.isHdr = false;
    config.profile.enableTemporalScale = false;
    config.url = const_cast<char *>(urlBytes.constData());
    config.fileGenerationMode = AVRECORDER_APP_CREATE;
    config.maxDuration = 0;

    if (OH_AVRecorder_Prepare(m_recorder, &config) != AV_ERR_OK) {
        emit recorderErrorOccurred(int(QMediaRecorder::FormatError),
                                   tr("OH_AVRecorder_Prepare failed"));
        ::close(fd);
        destroyRecorder();
        return false;
    }

    if (videoEnabled) {
        if (OH_AVRecorder_GetInputSurface(m_recorder, &m_recorderWindow) != AV_ERR_OK
            || !m_recorderWindow) {
            emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                       tr("OH_AVRecorder_GetInputSurface failed"));
            destroyRecorder();
            return false;
        }

        uint64_t surfaceIdNum = 0;
        if (OH_NativeWindow_GetSurfaceId(m_recorderWindow, &surfaceIdNum) != 0
            || surfaceIdNum == 0) {
            emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                       tr("Failed to obtain recorder surface ID"));
            destroyRecorder();
            return false;
        }
        const QByteArray surfaceId = QByteArray::number(qulonglong(surfaceIdNum));

        if (!attachVideoOutput(videoProfile, surfaceId)) {
            emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                       tr("Failed to attach video output to capture session"));
            destroyRecorder();
            return false;
        }
    }

    if (OH_AVRecorder_Start(m_recorder) != AV_ERR_OK) {
        emit recorderErrorOccurred(int(QMediaRecorder::ResourceError),
                                   tr("OH_AVRecorder_Start failed"));
        detachVideoOutput();
        destroyRecorder();
        return false;
    }

    m_recorderActualLocation = QUrl::fromLocalFile(resolved);
    emit recorderActualLocationChanged(m_recorderActualLocation);
    m_recorderPausedMs = 0;
    m_recorderResumeStartMs = 0;
    m_recorderTimer.restart();
    return true;
}

void QOhosCameraSession::pauseRecording()
{
    if (!m_recorder || m_recorderState != QMediaRecorder::RecordingState)
        return;
    if (OH_AVRecorder_Pause(m_recorder) == AV_ERR_OK) {
        m_recorderPausedMs += (m_recorderTimer.elapsed() - m_recorderResumeStartMs);
    }
}

void QOhosCameraSession::resumeRecording()
{
    if (!m_recorder || m_recorderState != QMediaRecorder::PausedState)
        return;
    if (OH_AVRecorder_Resume(m_recorder) == AV_ERR_OK)
        m_recorderResumeStartMs = m_recorderTimer.elapsed();
}

void QOhosCameraSession::stopRecording()
{
    if (!m_recorder)
        return;
    OH_AVRecorder_Stop(m_recorder);
    detachVideoOutput();
    destroyRecorder();
}

void QOhosCameraSession::destroyRecorder()
{
    if (m_recorder) {
        OH_AVRecorder_Release(m_recorder);
        m_recorder = nullptr;
    }
    m_recorderWindow = nullptr;
    if (m_recorderState != QMediaRecorder::StoppedState) {
        m_recorderState = QMediaRecorder::StoppedState;
        emit recorderStateChanged(int(QMediaRecorder::StoppedState));
    }
    m_recorderTimer.invalidate();
    m_recorderPausedMs = 0;
    m_recorderResumeStartMs = 0;
}

QT_END_NAMESPACE

#include "moc_qohoscamerasession_p.cpp"
