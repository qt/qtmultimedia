// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2camera_p.h"

#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qv4l2filedescriptor_p.h>
#include <QtMultimedia/private/qv4l2formats_p.h>
#include <QtMultimedia/private/qv4l2memorytransfer_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qsocketnotifier.h>
#include <QtCore/private/qcore_unix_p.h>

#include <sys/time.h>

#include <chrono>
#include <system_error>

QT_BEGIN_NAMESPACE

namespace QtV4L2 {

using namespace Qt::Literals;

Q_STATIC_LOGGING_CATEGORY(qLcV4L2Camera, "qt.multimedia.v4l2.camera");

static std::chrono::microseconds toMicroseconds(const timeval &tv)
{
    return std::chrono::seconds{ tv.tv_sec } + std::chrono::microseconds{ tv.tv_usec };
}

static QString v4l2ErrorMessage(std::error_code error)
{
    if (error == std::errc::device_or_resource_busy)
        return u"Camera is in use"_s;
    if (error == std::errc::no_such_device)
        return u"Camera has been removed"_s;
    if (error == std::errc::not_supported)
        return u"Unsupported V4L2 camera format"_s;
    return u"V4L2 camera error: " + qt_error_string(error.value());
}

QV4L2Camera::QV4L2Camera(QCamera *camera) : QPlatformCamera(camera) { }

QV4L2Camera::~QV4L2Camera()
{
    stopCapturing();
    closeV4L2Fd();
}

bool QV4L2Camera::isActive() const
{
    return m_active;
}

void QV4L2Camera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;

    if (m_cameraFormat.isNull())
        resolveCameraFormat({});

    m_active = active;
    if (m_active) {
        if (auto result = startCapturing(); !result)
            reportError(result.error());

        Q_EMIT newVideoFrame({});
    } else {
        stopCapturing();
        emitEndOfStreamFrame(m_lastFrameStart.value_or(std::chrono::microseconds{}),
                             m_lastFrameEnd);
    }

    Q_EMIT activeChanged(active);
}

void QV4L2Camera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    stopCapturing();
    closeV4L2Fd();

    m_cameraDevice = camera;
    resolveCameraFormat({});

    if (auto result = initV4L2Controls(); !result) {
        reportError(result.error());
        return;
    }

    if (m_active) {
        if (auto result = startCapturing(); !result)
            reportError(result.error());
    }
}

bool QV4L2Camera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    if (!resolveCameraFormat(format))
        return true;

    if (m_active) {
        stopCapturing();
        closeV4L2Fd();

        if (auto result = initV4L2Controls(); !result) {
            reportError(result.error());
            return true;
        }

        if (auto result = startCapturing(); !result)
            reportError(result.error());
    }

    return true;
}

bool QV4L2Camera::resolveCameraFormat(const QCameraFormat &format)
{
    auto fmt = format;
    if (fmt.isNull())
        fmt = findBestCameraFormat(m_cameraDevice);

    if (fmt == m_cameraFormat)
        return false;

    m_cameraFormat = fmt;
    // The device must be reconfigured with the new format before the next stream start.
    m_negotiatedFormat.reset();
    return true;
}

void QV4L2Camera::setFocusMode(QCamera::FocusMode mode)
{
    if (mode == focusMode())
        return;

    bool focusDist = supportedFeatures() & QCamera::Feature::FocusDistance;
    if (!focusDist && !m_v4l2Info.rangedFocus)
        return;

    switch (mode) {
    default:
    case QCamera::FocusModeAuto:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 1);
        if (m_v4l2Info.rangedFocus)
            setV4L2Parameter(V4L2_CID_AUTO_FOCUS_RANGE, V4L2_AUTO_FOCUS_RANGE_AUTO);
        break;
    case QCamera::FocusModeAutoNear:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 1);
        if (m_v4l2Info.rangedFocus)
            setV4L2Parameter(V4L2_CID_AUTO_FOCUS_RANGE, V4L2_AUTO_FOCUS_RANGE_MACRO);
        else if (focusDist)
            setV4L2Parameter(V4L2_CID_FOCUS_ABSOLUTE, m_v4l2Info.minFocus);
        break;
    case QCamera::FocusModeAutoFar:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 1);
        if (m_v4l2Info.rangedFocus)
            setV4L2Parameter(V4L2_CID_AUTO_FOCUS_RANGE, V4L2_AUTO_FOCUS_RANGE_INFINITY);
        break;
    case QCamera::FocusModeInfinity:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 0);
        setV4L2Parameter(V4L2_CID_FOCUS_ABSOLUTE, m_v4l2Info.maxFocus);
        break;
    case QCamera::FocusModeManual:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 0);
        setFocusDistance(focusDistance());
        break;
    }
    focusModeChanged(mode);
}

void QV4L2Camera::setFocusDistance(float d)
{
    int distance = m_v4l2Info.minFocus + int((m_v4l2Info.maxFocus - m_v4l2Info.minFocus) * d);
    setV4L2Parameter(V4L2_CID_FOCUS_ABSOLUTE, distance);
    focusDistanceChanged(d);
}

void QV4L2Camera::zoomTo(float factor, float)
{
    if (m_v4l2Info.maxZoom == m_v4l2Info.minZoom)
        return;
    factor = qBound(1., factor, 2.);
    int zoom = m_v4l2Info.minZoom + (factor - 1.) * (m_v4l2Info.maxZoom - m_v4l2Info.minZoom);
    setV4L2Parameter(V4L2_CID_ZOOM_ABSOLUTE, zoom);
    zoomFactorChanged(factor);
}

bool QV4L2Camera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    if (supportedFeatures() & QCamera::Feature::FocusDistance
        && (mode == QCamera::FocusModeManual || mode == QCamera::FocusModeAutoNear
            || mode == QCamera::FocusModeInfinity))
        return true;

    return mode == QCamera::FocusModeAuto;
}

void QV4L2Camera::setFlashMode(QCamera::FlashMode mode)
{
    if (!m_v4l2Info.flashSupported || mode == QCamera::FlashOn)
        return;
    setV4L2Parameter(V4L2_CID_FLASH_LED_MODE,
                     mode == QCamera::FlashAuto ? V4L2_FLASH_LED_MODE_FLASH
                                                : V4L2_FLASH_LED_MODE_NONE);
    flashModeChanged(mode);
}

bool QV4L2Camera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    if (m_v4l2Info.flashSupported && mode == QCamera::FlashAuto)
        return true;
    return mode == QCamera::FlashOff;
}

bool QV4L2Camera::isFlashReady() const
{
    return getV4L2Parameter(V4L2_CID_FLASH_READY).value_or(0) != 0;
}

void QV4L2Camera::setTorchMode(QCamera::TorchMode mode)
{
    if (!m_v4l2Info.torchSupported || mode == QCamera::TorchAuto)
        return;
    setV4L2Parameter(V4L2_CID_FLASH_LED_MODE,
                     mode == QCamera::TorchOn ? V4L2_FLASH_LED_MODE_TORCH
                                              : V4L2_FLASH_LED_MODE_NONE);
    torchModeChanged(mode);
}

bool QV4L2Camera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (mode == QCamera::TorchOn)
        return m_v4l2Info.torchSupported;
    return mode == QCamera::TorchOff;
}

void QV4L2Camera::setExposureMode(QCamera::ExposureMode mode)
{
    if (m_v4l2Info.autoExposureSupported && m_v4l2Info.manualExposureSupported) {
        if (mode != QCamera::ExposureAuto && mode != QCamera::ExposureManual)
            return;
        int value = mode == QCamera::ExposureAuto ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;
        setV4L2Parameter(V4L2_CID_EXPOSURE_AUTO, value);
        exposureModeChanged(mode);
        return;
    }
}

bool QV4L2Camera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (mode == QCamera::ExposureAuto)
        return true;
    if (m_v4l2Info.manualExposureSupported && m_v4l2Info.autoExposureSupported)
        return mode == QCamera::ExposureManual;
    return false;
}

void QV4L2Camera::setExposureCompensation(float compensation)
{
    if ((m_v4l2Info.minExposureAdjustment != 0 || m_v4l2Info.maxExposureAdjustment != 0)) {
        int value = qBound(m_v4l2Info.minExposureAdjustment, (int)(compensation * 1000),
                           m_v4l2Info.maxExposureAdjustment);
        setV4L2Parameter(V4L2_CID_AUTO_EXPOSURE_BIAS, value);
        exposureCompensationChanged(value / 1000.);
        return;
    }
}

void QV4L2Camera::setManualIsoSensitivity(int iso)
{
    if (!(supportedFeatures() & QCamera::Feature::IsoSensitivity))
        return;
    setV4L2Parameter(V4L2_CID_ISO_SENSITIVITY_AUTO,
                     iso <= 0 ? V4L2_ISO_SENSITIVITY_AUTO : V4L2_ISO_SENSITIVITY_MANUAL);
    if (iso > 0) {
        iso = qBound(minIso(), iso, maxIso());
        setV4L2Parameter(V4L2_CID_ISO_SENSITIVITY, iso);
    }
}

int QV4L2Camera::isoSensitivity() const
{
    if (!(supportedFeatures() & QCamera::Feature::IsoSensitivity))
        return -1;
    return getV4L2Parameter(V4L2_CID_ISO_SENSITIVITY).value_or(-1);
}

void QV4L2Camera::setManualExposureTime(float secs)
{
    if (m_v4l2Info.manualExposureSupported && m_v4l2Info.autoExposureSupported) {
        int exposure =
                qBound(m_v4l2Info.minExposure, qRound(secs * 10000.), m_v4l2Info.maxExposure);
        setV4L2Parameter(V4L2_CID_EXPOSURE_ABSOLUTE, exposure);
        exposureTimeChanged(exposure / 10000.);
        return;
    }
}

float QV4L2Camera::exposureTime() const
{
    return getV4L2Parameter(V4L2_CID_EXPOSURE_ABSOLUTE).value_or(-10000) / 10000.;
}

bool QV4L2Camera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (m_v4l2Info.autoWhiteBalanceSupported && m_v4l2Info.colorTemperatureSupported)
        return true;

    return mode == QCamera::WhiteBalanceAuto;
}

void QV4L2Camera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    Q_ASSERT(isWhiteBalanceModeSupported(mode));

    int temperature = colorTemperatureForWhiteBalance(mode);
    int t = setV4L2ColorTemperature(temperature);
    if (t == 0)
        mode = QCamera::WhiteBalanceAuto;
    whiteBalanceModeChanged(mode);
}

void QV4L2Camera::setColorTemperature(int temperature)
{
    if (temperature == 0) {
        setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
        return;
    }

    Q_ASSERT(isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual));

    int t = setV4L2ColorTemperature(temperature);
    if (t)
        colorTemperatureChanged(t);
}

void QV4L2Camera::readFrame()
{
    Q_ASSERT(m_memoryTransfer);

    auto dequeued = m_memoryTransfer->dequeueFrame();
    if (!dequeued) {
        if (dequeued.error() == std::errc::no_such_device) {
            // camera got removed while being active
            stopCapturing();
            closeV4L2Fd();
        } else if (dequeued.error() != std::errc::resource_unavailable_try_again) {
            qCWarning(qLcV4L2Camera)
                    << "Cannot take buffer:" << qt_error_string(dequeued.error().value());
        }

        return;
    }

    Q_ASSERT(m_negotiatedFormat);

    QVideoFrame frame =
            QVideoFramePrivate::createFrame(std::move(dequeued->videoBuffer), frameFormat());

    const auto timestamp = toMicroseconds(dequeued->timestamp);
    if (!m_firstFrameTime)
        m_firstFrameTime = timestamp;
    const auto frameTime = timestamp - *m_firstFrameTime;
    m_lastFrameStart = frameTime;
    m_lastFrameEnd = frameTime + m_negotiatedFormat->frameDuration;
    frame.setStartTime(frameTime.count());
    frame.setEndTime(m_lastFrameEnd->count());

    Q_EMIT newVideoFrame(frame);
}

void QV4L2Camera::reportError(std::error_code error)
{
    qCWarning(qLcV4L2Camera) << "V4L2 camera error:" << qt_error_string(error.value());
    updateError(QCamera::CameraError, v4l2ErrorMessage(error));
}

std::optional<v4l2_queryctrl> QV4L2Camera::query(uint32_t id) const
{
    auto result = m_v4l2FileDescriptor->queryControl(id);
    if (!result) {
        qCWarning(qLcV4L2Camera) << "Unable to query the V4L2 parameter" << Qt::hex << id
                                 << result.error();
        return std::nullopt;
    }
    return *result;
}

q23::expected<void, std::error_code> QV4L2Camera::initV4L2Controls()
{
    m_v4l2Info = {};
    supportedFeaturesChanged({});

    QCamera::Features features;

    const QByteArray deviceName = m_cameraDevice.id();
    Q_ASSERT(!deviceName.isEmpty());

    closeV4L2Fd();

    const int descriptor = qt_safe_open(deviceName.constData(), O_RDWR);
    if (descriptor == -1) {
        const auto error = std::error_code{ errno, std::system_category() };
        qCWarning(qLcV4L2Camera) << "Unable to open the camera" << deviceName
                                 << "for read to query the parameter info:"
                                 << qt_error_string(error.value());
        return q23::unexpected(error);
    }

    m_v4l2FileDescriptor =
            std::make_shared<QV4L2FileDescriptor>(QUniqueFileDescriptorHandle{ descriptor });

    qCDebug(qLcV4L2Camera) << "FD=" << descriptor;

    if (query(V4L2_CID_AUTO_WHITE_BALANCE)) {
        m_v4l2Info.autoWhiteBalanceSupported = true;
        setV4L2Parameter(V4L2_CID_AUTO_WHITE_BALANCE, true);
    }

    if (const auto control = query(V4L2_CID_WHITE_BALANCE_TEMPERATURE)) {
        m_v4l2Info.minColorTemp = control->minimum;
        m_v4l2Info.maxColorTemp = control->maximum;
        m_v4l2Info.colorTemperatureSupported = true;
        features |= QCamera::Feature::ColorTemperature;
    }

    if (query(V4L2_CID_EXPOSURE_AUTO))
        m_v4l2Info.autoExposureSupported = true;

    if (const auto control = query(V4L2_CID_EXPOSURE_ABSOLUTE)) {
        m_v4l2Info.manualExposureSupported = true;
        m_v4l2Info.minExposure = control->minimum;
        m_v4l2Info.maxExposure = control->maximum;
        features |= QCamera::Feature::ManualExposureTime;
    }

    if (const auto control = query(V4L2_CID_AUTO_EXPOSURE_BIAS)) {
        m_v4l2Info.minExposureAdjustment = control->minimum;
        m_v4l2Info.maxExposureAdjustment = control->maximum;
        features |= QCamera::Feature::ExposureCompensation;
    }

    if (query(V4L2_CID_ISO_SENSITIVITY_AUTO)) {
        if (const auto control = query(V4L2_CID_ISO_SENSITIVITY)) {
            features |= QCamera::Feature::IsoSensitivity;
            minIsoChanged(control->minimum);
            maxIsoChanged(control->maximum);
        }
    }

    if (const auto control = query(V4L2_CID_FOCUS_ABSOLUTE)) {
        m_v4l2Info.minFocus = control->minimum;
        m_v4l2Info.maxFocus = control->maximum;
        features |= QCamera::Feature::FocusDistance;
    }

    if (query(V4L2_CID_AUTO_FOCUS_RANGE))
        m_v4l2Info.rangedFocus = true;

    if (const auto control = query(V4L2_CID_FLASH_LED_MODE)) {
        m_v4l2Info.flashSupported = control->minimum <= V4L2_FLASH_LED_MODE_FLASH
                && control->maximum >= V4L2_FLASH_LED_MODE_FLASH;
        m_v4l2Info.torchSupported = control->minimum <= V4L2_FLASH_LED_MODE_TORCH
                && control->maximum >= V4L2_FLASH_LED_MODE_TORCH;
    }

    if (const auto control = query(V4L2_CID_ZOOM_ABSOLUTE)) {
        m_v4l2Info.minZoom = control->minimum;
        m_v4l2Info.maxZoom = control->maximum;
    }
    // zoom factors are in arbitrary units, so we simply normalize them to go from 1 to 2
    // if they are different
    minimumZoomFactorChanged(1);
    maximumZoomFactorChanged(m_v4l2Info.minZoom != m_v4l2Info.maxZoom ? 2 : 1);

    supportedFeaturesChanged(features);
    return {};
}

void QV4L2Camera::closeV4L2Fd()
{
    Q_ASSERT(!m_memoryTransfer);

    m_v4l2Info = {};
    m_negotiatedFormat.reset();
    m_v4l2FileDescriptor = nullptr;
}

int QV4L2Camera::setV4L2ColorTemperature(int temperature)
{
    if (m_v4l2Info.autoWhiteBalanceSupported) {
        setV4L2Parameter(V4L2_CID_AUTO_WHITE_BALANCE, temperature == 0 ? true : false);
    } else if (temperature == 0) {
        temperature = 5600;
    }

    if (temperature != 0 && m_v4l2Info.colorTemperatureSupported) {
        temperature = qBound(m_v4l2Info.minColorTemp, temperature, m_v4l2Info.maxColorTemp);
        if (!setV4L2Parameter(
                    V4L2_CID_WHITE_BALANCE_TEMPERATURE,
                    qBound(m_v4l2Info.minColorTemp, temperature, m_v4l2Info.maxColorTemp)))
            temperature = 0;
    } else {
        temperature = 0;
    }

    return temperature;
}

q23::expected<void, std::error_code> QV4L2Camera::setV4L2Parameter(quint32 id, qint32 value)
{
    v4l2_control control{ id, value };
    auto result = m_v4l2FileDescriptor->call(VIDIOC_S_CTRL, &control);
    if (!result) {
        qCWarning(qLcV4L2Camera) << "Unable to set the V4L2 parameter" << Qt::hex << id << "to"
                                 << value << result.error();
        return q23::unexpected(result.error());
    }
    return {};
}

std::optional<qint32> QV4L2Camera::getV4L2Parameter(quint32 id) const
{
    v4l2_control control{ id, 0 };
    auto result = m_v4l2FileDescriptor->call(VIDIOC_G_CTRL, &control);
    if (!result) {
        qCWarning(qLcV4L2Camera) << "Unable to get the V4L2 parameter" << Qt::hex << id
                                 << result.error();
        return std::nullopt;
    }
    return control.value;
}

q23::expected<void, std::error_code> QV4L2Camera::setV4L2CameraFormat()
{
    if (m_negotiatedFormat)
        return {};

    Q_ASSERT(m_v4l2FileDescriptor);

    Q_ASSERT(!m_cameraFormat.isNull());
    qCDebug(qLcV4L2Camera) << "XXXXX" << this << m_cameraDevice.id() << m_cameraFormat.pixelFormat()
                           << m_cameraFormat.resolution();

    const auto v4l2Format = v4l2FormatForPixelFormat(m_cameraFormat.pixelFormat());
    if (!v4l2Format) {
        qCWarning(qLcV4L2Camera) << "Unsupported camera pixel format"
                                 << m_cameraFormat.pixelFormat();
        return q23::unexpected(std::make_error_code(std::errc::not_supported));
    }

    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    auto size = m_cameraFormat.resolution();
    fmt.fmt.pix.width = size.width();
    fmt.fmt.pix.height = size.height();
    fmt.fmt.pix.pixelformat = *v4l2Format;
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    qCDebug(qLcV4L2Camera) << "setting camera format to" << size << fmt.fmt.pix.pixelformat;

    auto setFormatResult = m_v4l2FileDescriptor->call(VIDIOC_S_FMT, &fmt);
    if (!setFormatResult) {
        qCWarning(qLcV4L2Camera) << "Couldn't set video format on v4l2 camera"
                                 << setFormatResult.error();
        return q23::unexpected(setFormatResult.error());
    }

    // The driver may have adjusted the requested format, so keep what it negotiated.
    const auto pixFormat = fmt.fmt.pix;

    const auto negotiatedPixelFormat = formatForV4L2Format(pixFormat.pixelformat);
    const bool isCompressed = negotiatedPixelFormat == QVideoFrameFormat::Format_Jpeg;

    if (!negotiatedPixelFormat || pixFormat.sizeimage == 0
        || (pixFormat.bytesperline == 0 && !isCompressed)) {
        qCWarning(qLcV4L2Camera) << "Driver negotiated an unusable camera format"
                                 << pixFormat.pixelformat;
        return q23::unexpected(std::make_error_code(std::errc::not_supported));
    }

    v4l2_streamparm streamParam = {};
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    streamParam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    qreal frameDuration = 1. / m_cameraFormat.maxFrameRate();
    auto [num, den] = qRealToFraction(frameDuration);
    streamParam.parm.capture.timeperframe = { (uint)num, (uint)den };
    if (auto result = m_v4l2FileDescriptor->call(VIDIOC_S_PARM, &streamParam); !result)
        qCWarning(qLcV4L2Camera) << "Couldn't set camera frame rate:" << result.error();

    using namespace std::chrono;
    m_negotiatedFormat = NegotiatedFormat{
        pixFormat,
        duration_cast<microseconds>(duration<qreal>{ frameDuration }),
    };
    return {};
}

q23::expected<void, std::error_code>
QV4L2Camera::initV4L2MemoryTransfer(const v4l2_pix_format &format)
{
    Q_ASSERT(!m_memoryTransfer);

    // QT_V4L2_CAMERA_MEMORY=dmabuf|userptr|mmap selects one strategy, with no fallback.
    const QByteArray forcedStrategy = qgetenv("QT_V4L2_CAMERA_MEMORY");
    const auto isAllowed = [&](QByteArrayView strategy) {
        return forcedStrategy.isEmpty() || forcedStrategy == strategy;
    };

    std::error_code lastError;

#if QT_CONFIG(linux_dmabuf)
    if (isAllowed("dmabuf")) {
        auto transfer = makeDmaBufMemoryTransfer(m_v4l2FileDescriptor, format);
        if (transfer) {
            m_memoryTransfer = std::move(*transfer);
            return {};
        }
        lastError = transfer.error();
    }
#endif

    if (isAllowed("userptr")) {
        auto transfer = makeUserPtrMemoryTransfer(m_v4l2FileDescriptor, format);
        if (transfer) {
            m_memoryTransfer = std::move(*transfer);
            return {};
        }
        lastError = transfer.error();
        if (lastError == std::errc::device_or_resource_busy)
            return q23::unexpected(lastError);
    }

    if (isAllowed("mmap")) {
        qCDebug(qLcV4L2Camera) << "Trying V4L2_MEMORY_MMAP";

        auto transfer = makeMMapMemoryTransfer(m_v4l2FileDescriptor, format);
        if (transfer) {
            m_memoryTransfer = std::move(*transfer);
            return {};
        }
        lastError = transfer.error();
    }

    qCWarning(qLcV4L2Camera) << "Cannot init V4L2 memory transfer:"
                             << qt_error_string(lastError.value());
    return q23::unexpected(lastError);
}

void QV4L2Camera::stopCapturing()
{
    if (!m_memoryTransfer || !m_v4l2FileDescriptor)
        return;

    m_notifier = nullptr;

    if (auto result = m_v4l2FileDescriptor->stopStream(); !result) {
        // TODO: handle the case carefully to avoid possible memory corruption
        if (result.error() != std::errc::no_such_device)
            qCWarning(qLcV4L2Camera) << "failed to stop capture:" << result.error();
    }

    m_memoryTransfer = nullptr;
}

void QV4L2Camera::resetCaptureState()
{
    m_notifier = nullptr;
    m_memoryTransfer = nullptr;
    m_negotiatedFormat.reset();
    m_firstFrameTime.reset();
    m_lastFrameStart.reset();
    m_lastFrameEnd.reset();
}

q23::expected<void, std::error_code> QV4L2Camera::startCapturing()
{
    if (!m_v4l2FileDescriptor)
        return q23::unexpected(std::make_error_code(std::errc::bad_file_descriptor));

    if (auto result = setV4L2CameraFormat(); !result) {
        resetCaptureState();
        return q23::unexpected(result.error());
    }

    Q_ASSERT(m_negotiatedFormat);

    if (auto result = initV4L2MemoryTransfer(m_negotiatedFormat->pixFormat); !result) {
        resetCaptureState();
        return q23::unexpected(result.error());
    }

    if (auto result = m_v4l2FileDescriptor->startStream(); !result) {
        qCWarning(qLcV4L2Camera) << "Couldn't start v4l2 camera stream:" << result.error();
        resetCaptureState();
        return q23::unexpected(result.error());
    }

    m_notifier =
            std::make_unique<QSocketNotifier>(m_v4l2FileDescriptor->get(), QSocketNotifier::Read);
    connect(m_notifier.get(), &QSocketNotifier::activated, m_notifier.get(), [this] {
        this->readFrame();
    });

    m_firstFrameTime.reset();
    m_lastFrameStart.reset();
    m_lastFrameEnd.reset();
    return {};
}

QVideoFrameFormat QV4L2Camera::frameFormat() const
{
    auto result = QPlatformCamera::frameFormat();
    if (!m_negotiatedFormat)
        return result;

    const auto &pixFormat = m_negotiatedFormat->pixFormat;

    result = QVideoFrameFormat(sizeForV4L2Format(pixFormat),
                               formatForV4L2Format(pixFormat.pixelformat));
    result.setStreamFrameRate(m_cameraFormat.maxFrameRate());

    result.setColorSpace(colorSpaceForV4L2Colorspace(v4l2_colorspace(pixFormat.colorspace)));
    return result;
}

} // namespace QtV4L2

QT_END_NAMESPACE

#include "moc_qv4l2camera_p.cpp"
