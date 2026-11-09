// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlibcamera_camera_p.h"
#include "qlibcamera_stream_p.h"
#include "qlibcamera_controls_p.h"
#include "qlibcamera_format_support_p.h"
#include "qlibcamera_instance_p.h"
#include "qlibcameralogging_p.h"

#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/qcameradevice.h>

#include <QtCore/qscopeguard.h>
#include <QtCore/private/qsystemerror_p.h>

#include <libcamera/camera.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/framebuffer.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/geometry.h>
#include <libcamera/pixel_format.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

#include <algorithm>
#include <cerrno>
#include <chrono>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QLibCamera {

using namespace libcamera;

namespace {

auto makeCameraError(const QString &errorString)
{
    return q23::unexpected(std::tuple{
            QCamera::CameraError,
            errorString,
    });
}

} // namespace

QLibCameraCamera::QLibCameraCamera(QCamera *parent, std::shared_ptr<QLibCameraInstance> instance)
    : QPlatformCamera(parent), m_instance(std::move(instance))
{
}

QLibCameraCamera::~QLibCameraCamera()
{
    stopCapturing();
}

int QLibCameraCamera::cameraPixelFormatScore(QVideoFrameFormat::PixelFormat format,
                                             QVideoFrameFormat::ColorRange /*colorRange*/) const
{
    switch (format) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_YUV420P:
        return 3;
    case QVideoFrameFormat::Format_YUYV:
    case QVideoFrameFormat::Format_UYVY:
        return 2;
    case QVideoFrameFormat::Format_Jpeg:
        return 0; // compressed; needs decoding before it can be turned into a QVideoFrame
    default:
        return 1;
    }
}

void QLibCameraCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;
    if (active && !m_camera) {
        updateError(QCamera::CameraError, u"Camera device is not available"_s);
        return;
    }

    if (m_cameraFormat.isNull())
        m_cameraFormat = findBestCameraFormat(m_cameraDevice);

    if (active) {
        if (StartCapturingResult result = startCapturing(); !result) {
            auto [error, string] = result.error();
            updateError(error, string);
            return;
        }
        m_active = true;
    } else {
        m_active = false;
        stopCapturing();
    }

    Q_EMIT newVideoFrame({});
    Q_EMIT activeChanged(active);
}

void QLibCameraCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    const bool wasActive = m_active;
    stopCapturing();

    m_cameraDevice = camera;
    m_cameraFormat = {};
    m_camera.reset();

    if (!camera.isNull() && m_instance)
        m_camera = m_instance->get(QString::fromUtf8(camera.id()));

    if (m_camera)
        supportedFeaturesChanged(QLibCameraControls::detectSupportedFeatures(m_camera->controls()));
    else
        supportedFeaturesChanged({});

    if (wasActive) {
        m_active = false; // startCapturing() below re-flips this
        setActive(true);
    }
}

bool QLibCameraCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    QCameraFormat newFormat = format.isNull() ? findBestCameraFormat(m_cameraDevice) : format;
    if (newFormat == m_cameraFormat)
        return true;

    m_cameraFormat = newFormat;

    if (m_active) {
        stopCapturing();
        startCapturingReportingErrors();
    }
    return true;
}

QLibCameraCamera::StartCapturingResult QLibCameraCamera::startCapturing()
{
    using namespace QLibCameraFormatSupport;
    namespace ranges = QtMultimediaPrivate::ranges;

    if (!m_camera || m_cameraFormat.isNull())
        return {};

    auto abortGuard = qScopeGuard([this] {
        stopCapturing();
    });

    const int acquireResult = m_camera->acquire();
    switch (acquireResult) {
    case 0:
        break;
    case -EBUSY:
        return makeCameraError(u"Camera is in use"_s);
    case -ENODEV:
        return makeCameraError(u"Camera has been disconnected"_s);
    default:
        return makeCameraError(u"Failed to acquire camera: "_s
                               + QSystemError::stdString(-acquireResult));
    }

    m_streamState = StreamState::Acquired;

    m_config = m_camera->generateConfiguration({ StreamRole::VideoRecording });
    if (!m_config || m_config->empty())
        return makeCameraError(u"Camera does not support video capture"_s);

    StreamConfiguration &streamConfig = m_config->at(0);
    // Prefer the currently-requested QCameraFormat's pixel format if libcamera can still produce
    // it for this size; otherwise fall through to whatever generateConfiguration() proposed.
    const std::vector<PixelFormat> pixelFormats = streamConfig.formats().pixelformats();
    const auto pixelFormatIt = ranges::find_if(pixelFormats, [&](const PixelFormat &pf) {
        return pixelFormatFromLibcamera(pf) == m_cameraFormat.pixelFormat();
    });
    if (pixelFormatIt != pixelFormats.end())
        streamConfig.pixelFormat = *pixelFormatIt;

    streamConfig.size = toSize(m_cameraFormat.resolution());
    streamConfig.bufferCount = 4;

    switch (m_config->validate()) {
    case CameraConfiguration::Valid:
    case CameraConfiguration::Adjusted:
        break;
    case CameraConfiguration::Invalid:
        return makeCameraError(u"Invalid camera configuration"_s);
    }

    const int configureResult = m_camera->configure(m_config.get());
    switch (configureResult) {
    case 0:
        break;
    case -ENODEV:
        return makeCameraError(u"Camera has been disconnected"_s);
    case -EACCES:
        return makeCameraError(u"Camera is not in a state where it can be configured"_s);
    case -EINVAL:
        return makeCameraError(u"Invalid camera configuration"_s);
    default:
        return makeCameraError(u"Failed to configure camera: "_s
                               + QSystemError::stdString(-configureResult));
    }

    // validate()/configure() may adjust the requested configuration; the *adjusted* values are
    // the ones actually in effect, and frameFormat() must describe those, not what was asked for.
    const StreamConfiguration &adjusted = m_config->at(0);
    m_stride = int(adjusted.stride);
    std::tie(m_colorSpace, m_colorRange) = colorSpaceFromLibcamera(adjusted.colorSpace);
    if (const std::optional adjustedFormat = pixelFormatFromLibcamera(adjusted.pixelFormat))
        m_framePixelFormat = *adjustedFormat;

    Stream *stream = adjusted.stream();

    m_allocator = std::make_unique<FrameBufferAllocator>(m_camera);
    if (const int allocated = m_allocator->allocate(stream); allocated < 0) {
        switch (allocated) {
        case -EACCES:
            return makeCameraError(u"Camera is not in a state where buffers can be allocated"_s);
        case -EINVAL:
            return makeCameraError(u"Camera stream is not part of the active configuration"_s);
        case -EBUSY:
            return makeCameraError(u"Capture buffers are already allocated"_s);
        default:
            return makeCameraError(u"Failed to allocate capture buffers: "_s
                                   + QSystemError::stdString(-allocated));
        }
    }

    m_stream = std::make_unique<QLibCameraCameraStream>(m_camera, stream, m_stride, frameFormat(),
                                                        m_cameraFormat.maxFrameRate());

    connect(m_stream.get(), &QLibCameraCameraStream::newVideoFrame, this,
            &QLibCameraCamera::newVideoFrame);

    m_camera->requestCompleted.connect(this, [stream = m_stream.get()](Request *request) {
        // libcamera thread
        stream->handleRequestCompleted(request);
    });

    m_requests.clear();
    for (const std::unique_ptr<FrameBuffer> &buffer : m_allocator->buffers(stream)) {
        std::unique_ptr<Request> request = m_camera->createRequest();
        if (!request)
            return makeCameraError(u"Failed to create capture request"_s);

        const int addBufferResult = request->addBuffer(stream, buffer.get());
        switch (addBufferResult) {
        case 0:
            break;
        case -EEXIST:
            return makeCameraError(u"Capture request already has a buffer for the stream"_s);
        case -EINVAL:
            return makeCameraError(u"Frame buffer does not reference a valid stream"_s);
        default:
            return makeCameraError(u"Failed to add buffer to capture request: "_s
                                   + QSystemError::stdString(-addBufferResult));
        }
        m_requests.push_back(std::move(request));
    }

    using namespace std::chrono;
    ControlList startControls(m_camera->controls());
    if (m_cameraFormat.maxFrameRate() > 0) {
        const auto frameDuration =
                duration_cast<microseconds>(duration<double>(1.0 / m_cameraFormat.maxFrameRate()));
        startControls.set(libcamera::controls::FrameDurationLimits,
                          { frameDuration.count(), frameDuration.count() });
    }

    const int startResult = m_camera->start(&startControls);
    switch (startResult) {
    case 0:
        break;
    case -ENODEV:
        return makeCameraError(u"Camera has been disconnected"_s);
    case -EACCES:
        return makeCameraError(u"Camera is not in a state where it can be started"_s);
    default:
        return makeCameraError(u"Failed to start camera: "_s
                               + QSystemError::stdString(-startResult));
    }
    m_streamState = StreamState::Running;

    for (const std::unique_ptr<Request> &request : m_requests) {
        const int queueResult = m_camera->queueRequest(request.get());
        switch (queueResult) {
        case 0:
            break;
        case -ENODEV:
            return makeCameraError(u"Camera has been disconnected"_s);
        case -EACCES:
            return makeCameraError(u"Camera is not running so requests can't be queued"_s);
        case -EXDEV:
            return makeCameraError(u"Capture request does not belong to this camera"_s);
        case -EINVAL:
            return makeCameraError(u"Capture request is invalid"_s);
        case -ENOMEM:
            return makeCameraError(u"No buffer memory available to queue capture request"_s);
        default:
            return makeCameraError(u"Failed to queue capture request: "_s
                                   + QSystemError::stdString(-queueResult));
        }
    }

    abortGuard.dismiss();
    return {};
}

void QLibCameraCamera::startCapturingReportingErrors()
{
    if (StartCapturingResult result = startCapturing(); !result)
        updateError(std::get<0>(result.error()), std::get<1>(result.error()));
}

void QLibCameraCamera::stopCapturing()
{
    if (m_camera) {
        m_camera->requestCompleted.disconnect(this);
        if (m_streamState == StreamState::Running) {
            const int stopResult = m_camera->stop();
            switch (stopResult) {
            case 0:
            case -EACCES: // the camera is not running
                break;
            case -ENODEV: // the camera has been disconnected
                qCDebug(lcLibCamera) << "Camera disconnected while stopping capture";
                break;
            default:
                qCWarning(lcLibCamera)
                        << "Failed to stop camera:" << QSystemError::stdString(-stopResult);
                break;
            }
        }
    }

    Stream *stream = m_stream ? m_stream->stream() : nullptr;
    m_stream.reset();

    m_requests.clear();
    if (m_allocator && stream) {
        const int freeResult = m_allocator->free(stream);
        switch (freeResult) {
        case 0:
        case -EACCES: // the camera is not in a state where buffers can be freed
        case -EINVAL: // the allocator does not handle this stream
            break;
        default:
            qCWarning(lcLibCamera)
                    << "Failed to free capture buffers:" << QSystemError::stdString(-freeResult);
            break;
        }
    }
    m_allocator.reset();
    m_config.reset();

    if (m_camera && m_streamState != StreamState::Idle) {
        const int releaseResult = m_camera->release();
        switch (releaseResult) {
        case 0:
            break;
        case -EBUSY: // the camera is still running
            qCWarning(lcLibCamera) << "Camera still running while releasing it";
            break;
        default:
            qCWarning(lcLibCamera)
                    << "Failed to release camera:" << QSystemError::stdString(-releaseResult);
            break;
        }
    }
    m_streamState = StreamState::Idle;
}

void QLibCameraCamera::setExposureMode(QCamera::ExposureMode mode)
{
    if (mode != QCamera::ExposureAuto && mode != QCamera::ExposureManual)
        return;
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setAeEnable(pending, mode == QCamera::ExposureAuto);
    });
    exposureModeChanged(mode);
}

bool QLibCameraCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    return mode == QCamera::ExposureAuto || mode == QCamera::ExposureManual;
}

void QLibCameraCamera::setExposureCompensation(float compensation)
{
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setExposureCompensation(pending, compensation);
    });
    exposureCompensationChanged(compensation);
}

void QLibCameraCamera::setManualIsoSensitivity(int iso)
{
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setAnalogueGainForIso(pending, iso);
    });
    isoSensitivityChanged(iso);
}

void QLibCameraCamera::setManualExposureTime(float seconds)
{
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setManualExposureTime(pending, seconds);
    });
    exposureTimeChanged(seconds);
}

void QLibCameraCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setAwbEnable(pending, mode == QCamera::WhiteBalanceAuto);
    });
    if (mode != QCamera::WhiteBalanceAuto)
        setColorTemperature(colorTemperatureForWhiteBalance(mode));
    whiteBalanceModeChanged(mode);
}

void QLibCameraCamera::setColorTemperature(int temperature)
{
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setColorTemperature(pending, temperature);
    });
    colorTemperatureChanged(temperature);
}

void QLibCameraCamera::zoomTo(float newZoomFactor, float /*rate*/)
{
    if (!m_camera)
        return;
    newZoomFactor = std::clamp(newZoomFactor, minZoomFactor(), maxZoomFactor());
    withPendingControls([&](ControlList &pending) {
        QLibCameraControls::setZoomFactor(pending, m_camera->controls(), newZoomFactor);
    });
    zoomFactorChanged(newZoomFactor);
}

QVideoFrameFormat QLibCameraCamera::frameFormat() const
{
    QVideoFrameFormat result = QPlatformCamera::frameFormat();
    result.setColorSpace(m_colorSpace);
    result.setColorRange(m_colorRange);
    return result;
}

} // namespace QLibCamera

QT_END_NAMESPACE

#include "moc_qlibcamera_camera_p.cpp"
