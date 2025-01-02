// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2camera_p.h"
#include "qv4l2filedescriptor_p.h"
#include "qv4l2memorytransfer_p.h"

#include <private/qcameradevice_p.h>
#include <private/qmultimediautils_p.h>
#include <private/qmemoryvideobuffer_p.h>
#include <private/qcore_unix_p.h>

#include <qsocketnotifier.h>
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcV4L2Camera, "qt.multimedia.ffmpeg.v4l2camera");

static const struct {
    QVideoFrameFormat::PixelFormat fmt;
    uint32_t v4l2Format;
} formatMap[] = {
    // ### How do we handle V4L2_PIX_FMT_H264 and V4L2_PIX_FMT_MPEG4?
    { QVideoFrameFormat::Format_YUV420P,  V4L2_PIX_FMT_YUV420  },
    { QVideoFrameFormat::Format_YUV422P,  V4L2_PIX_FMT_YUV422P },
    { QVideoFrameFormat::Format_YUYV,     V4L2_PIX_FMT_YUYV    },
    { QVideoFrameFormat::Format_UYVY,     V4L2_PIX_FMT_UYVY    },
    { QVideoFrameFormat::Format_XBGR8888, V4L2_PIX_FMT_XBGR32  },
    { QVideoFrameFormat::Format_XRGB8888, V4L2_PIX_FMT_XRGB32  },
    { QVideoFrameFormat::Format_ABGR8888, V4L2_PIX_FMT_ABGR32  },
    { QVideoFrameFormat::Format_ARGB8888, V4L2_PIX_FMT_ARGB32  },
    { QVideoFrameFormat::Format_BGRX8888, V4L2_PIX_FMT_BGR32   },
    { QVideoFrameFormat::Format_RGBX8888, V4L2_PIX_FMT_RGB32   },
    { QVideoFrameFormat::Format_BGRA8888, V4L2_PIX_FMT_BGRA32  },
    { QVideoFrameFormat::Format_RGBA8888, V4L2_PIX_FMT_RGBA32  },
    { QVideoFrameFormat::Format_Y8,       V4L2_PIX_FMT_GREY    },
    { QVideoFrameFormat::Format_Y16,      V4L2_PIX_FMT_Y16     },
    { QVideoFrameFormat::Format_NV12,     V4L2_PIX_FMT_NV12    },
    { QVideoFrameFormat::Format_NV21,     V4L2_PIX_FMT_NV21    },
    { QVideoFrameFormat::Format_Jpeg,     V4L2_PIX_FMT_MJPEG   },
    { QVideoFrameFormat::Format_Jpeg,     V4L2_PIX_FMT_JPEG    },
    { QVideoFrameFormat::Format_Invalid,  0                    },
};

QVideoFrameFormat::PixelFormat formatForV4L2Format(uint32_t v4l2Format)
{
    auto *f = formatMap;
    while (f->v4l2Format) {
        if (f->v4l2Format == v4l2Format)
            return f->fmt;
        ++f;
    }
    return QVideoFrameFormat::Format_Invalid;
}

uint32_t v4l2FormatForPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    auto *f = formatMap;
    while (f->v4l2Format) {
        if (f->fmt == format)
            return f->v4l2Format;
        ++f;
    }
    return 0;
}

QV4L2Camera::QV4L2Camera(QCamera *camera)
    : QPlatformCamera(camera)
{
}

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
    if (m_active)
        startCapturing();
    else
        stopCapturing();

    emit newVideoFrame({});

    emit activeChanged(active);
}

void QV4L2Camera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    stopCapturing();
    closeV4L2Fd();

    m_cameraDevice = camera;
    resolveCameraFormat({});

    initV4L2Controls();

    if (m_active)
        startCapturing();
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

        initV4L2Controls();
        startCapturing();
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
    if (supportedFeatures() & QCamera::Feature::FocusDistance &&
        (mode == QCamera::FocusModeManual || mode == QCamera::FocusModeAutoNear || mode == QCamera::FocusModeInfinity))
        return true;

    return mode == QCamera::FocusModeAuto;
}

void QV4L2Camera::setFlashMode(QCamera::FlashMode mode)
{
    if (!m_v4l2Info.flashSupported || mode == QCamera::FlashOn)
        return;
    setV4L2Parameter(V4L2_CID_FLASH_LED_MODE, mode == QCamera::FlashAuto ? V4L2_FLASH_LED_MODE_FLASH : V4L2_FLASH_LED_MODE_NONE);
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
    struct v4l2_queryctrl queryControl;
    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_WHITE_BALANCE;

    return m_v4l2FileDescriptor && m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl);
}

void QV4L2Camera::setTorchMode(QCamera::TorchMode mode)
{
    if (!m_v4l2Info.torchSupported || mode == QCamera::TorchOn)
        return;
    setV4L2Parameter(V4L2_CID_FLASH_LED_MODE, mode == QCamera::TorchOn ? V4L2_FLASH_LED_MODE_TORCH : V4L2_FLASH_LED_MODE_NONE);
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
        int value = QCamera::ExposureAuto ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;
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
        exposureCompensationChanged(value/1000.);
        return;
    }
}

void QV4L2Camera::setManualIsoSensitivity(int iso)
{
    if (!(supportedFeatures() & QCamera::Feature::IsoSensitivity))
        return;
    setV4L2Parameter(V4L2_CID_ISO_SENSITIVITY_AUTO, iso <= 0 ? V4L2_ISO_SENSITIVITY_AUTO : V4L2_ISO_SENSITIVITY_MANUAL);
    if (iso > 0) {
        iso = qBound(minIso(), iso, maxIso());
        setV4L2Parameter(V4L2_CID_ISO_SENSITIVITY, iso);
    }
    return;
}

int QV4L2Camera::isoSensitivity() const
{
    if (!(supportedFeatures() & QCamera::Feature::IsoSensitivity))
        return -1;
    return getV4L2Parameter(V4L2_CID_ISO_SENSITIVITY);
}

void QV4L2Camera::setManualExposureTime(float secs)
{
    if (m_v4l2Info.manualExposureSupported && m_v4l2Info.autoExposureSupported) {
        int exposure =
                qBound(m_v4l2Info.minExposure, qRound(secs * 10000.), m_v4l2Info.maxExposure);
        setV4L2Parameter(V4L2_CID_EXPOSURE_ABSOLUTE, exposure);
        exposureTimeChanged(exposure/10000.);
        return;
    }
}

float QV4L2Camera::exposureTime() const
{
    return getV4L2Parameter(V4L2_CID_EXPOSURE_ABSOLUTE)/10000.;
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

    auto buffer = m_memoryTransfer->dequeueBuffer();
    if (!buffer) {
        qCWarning(qLcV4L2Camera) << "Cannot take buffer";

        if (errno == ENODEV) {
            // camera got removed while being active
            stopCapturing();
            closeV4L2Fd();
        }

        return;
    }

    auto videoBuffer = new QMemoryVideoBuffer(buffer->data, m_bytesPerLine);
    QVideoFrame frame(videoBuffer, frameFormat());

    auto &v4l2Buffer = buffer->v4l2Buffer;

    if (m_firstFrameTime.tv_sec == -1)
        m_firstFrameTime = v4l2Buffer.timestamp;
    qint64 secs = v4l2Buffer.timestamp.tv_sec - m_firstFrameTime.tv_sec;
    qint64 usecs = v4l2Buffer.timestamp.tv_usec - m_firstFrameTime.tv_usec;
    frame.setStartTime(secs*1000000 + usecs);
    frame.setEndTime(frame.startTime() + m_frameDuration);

    emit newVideoFrame(frame);

    if (!m_memoryTransfer->enqueueBuffer(v4l2Buffer.index))
        qCWarning(qLcV4L2Camera) << "Cannot add buffer";
}

void QV4L2Camera::setCameraBusy()
{
    m_cameraBusy = true;
    emit error(QCamera::CameraError, QLatin1String("Camera is in use"));
}

void QV4L2Camera::initV4L2Controls()
{
    m_v4l2Info = {};
    QCamera::Features features;

    const QByteArray deviceName = m_cameraDevice.id();
    Q_ASSERT(!deviceName.isEmpty());

    closeV4L2Fd();

    const int descriptor = qt_safe_open(deviceName.constData(), O_RDWR);
    if (descriptor == -1) {
        qCWarning(qLcV4L2Camera) << "Unable to open the camera" << deviceName
                                 << "for read to query the parameter info:"
                                 << qt_error_string(errno);
        emit error(QCamera::CameraError, QLatin1String("Cannot open camera"));
        return;
    }

    m_v4l2FileDescriptor = std::make_shared<QV4L2FileDescriptor>(descriptor);

    qCDebug(qLcV4L2Camera) << "FD=" << descriptor;

    struct v4l2_queryctrl queryControl;
    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_WHITE_BALANCE;

    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.autoWhiteBalanceSupported = true;
        setV4L2Parameter(V4L2_CID_AUTO_WHITE_BALANCE, true);
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.minColorTemp = queryControl.minimum;
        m_v4l2Info.maxColorTemp = queryControl.maximum;
        m_v4l2Info.colorTemperatureSupported = true;
        features |= QCamera::Feature::ColorTemperature;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_EXPOSURE_AUTO;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.autoExposureSupported = true;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.manualExposureSupported = true;
        m_v4l2Info.minExposure = queryControl.minimum;
        m_v4l2Info.maxExposure = queryControl.maximum;
        features |= QCamera::Feature::ManualExposureTime;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_EXPOSURE_BIAS;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.minExposureAdjustment = queryControl.minimum;
        m_v4l2Info.maxExposureAdjustment = queryControl.maximum;
        features |= QCamera::Feature::ExposureCompensation;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_ISO_SENSITIVITY_AUTO;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        queryControl.id = V4L2_CID_ISO_SENSITIVITY;
        if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
            features |= QCamera::Feature::IsoSensitivity;
            minIsoChanged(queryControl.minimum);
            maxIsoChanged(queryControl.minimum);
        }
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_FOCUS_ABSOLUTE;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.minExposureAdjustment = queryControl.minimum;
        m_v4l2Info.maxExposureAdjustment = queryControl.maximum;
        features |= QCamera::Feature::FocusDistance;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_FOCUS_RANGE;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.rangedFocus = true;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_FLASH_LED_MODE;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.flashSupported = queryControl.minimum <= V4L2_FLASH_LED_MODE_FLASH
                && queryControl.maximum >= V4L2_FLASH_LED_MODE_FLASH;
        m_v4l2Info.torchSupported = queryControl.minimum <= V4L2_FLASH_LED_MODE_TORCH
                && queryControl.maximum >= V4L2_FLASH_LED_MODE_TORCH;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_ZOOM_ABSOLUTE;
    if (m_v4l2FileDescriptor->call(VIDIOC_QUERYCTRL, &queryControl)) {
        m_v4l2Info.minZoom = queryControl.minimum;
        m_v4l2Info.maxZoom = queryControl.maximum;
    }
    // zoom factors are in arbitrary units, so we simply normalize them to go from 1 to 2
    // if they are different
    minimumZoomFactorChanged(1);
    maximumZoomFactorChanged(m_v4l2Info.minZoom != m_v4l2Info.maxZoom ? 2 : 1);

    supportedFeaturesChanged(features);
}

void QV4L2Camera::closeV4L2Fd()
{
    Q_ASSERT(!m_memoryTransfer);

    m_v4l2Info = {};
    m_cameraBusy = false;
    m_v4l2FileDescriptor = nullptr;
}

int QV4L2Camera::setV4L2ColorTemperature(int temperature)
{
    struct v4l2_control control;
    ::memset(&control, 0, sizeof(control));

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

bool QV4L2Camera::setV4L2Parameter(quint32 id, qint32 value)
{
    v4l2_control control{ id, value };
    if (!m_v4l2FileDescriptor->call(VIDIOC_S_CTRL, &control)) {
        qWarning() << "Unable to set the V4L2 Parameter" << Qt::hex << id << "to" << value << qt_error_string(errno);
        return false;
    }
    return true;
}

int QV4L2Camera::getV4L2Parameter(quint32 id) const
{
    struct v4l2_control control{id, 0};
    if (!m_v4l2FileDescriptor->call(VIDIOC_G_CTRL, &control)) {
        qWarning() << "Unable to get the V4L2 Parameter" << Qt::hex << id << qt_error_string(errno);
        return 0;
    }
    return control.value;
}

void QV4L2Camera::setV4L2CameraFormat()
{
    if (m_v4l2Info.formatInitialized || !m_v4l2FileDescriptor)
        return;

    Q_ASSERT(!m_cameraFormat.isNull());
    qCDebug(qLcV4L2Camera) << "XXXXX" << this << m_cameraDevice.id() << m_cameraFormat.pixelFormat()
                           << m_cameraFormat.resolution();

    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    auto size = m_cameraFormat.resolution();
    fmt.fmt.pix.width = size.width();
    fmt.fmt.pix.height = size.height();
    fmt.fmt.pix.pixelformat = v4l2FormatForPixelFormat(m_cameraFormat.pixelFormat());
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    qCDebug(qLcV4L2Camera) << "setting camera format to" << size << fmt.fmt.pix.pixelformat;

    if (!m_v4l2FileDescriptor->call(VIDIOC_S_FMT, &fmt)) {
        if (errno == EBUSY) {
            setCameraBusy();
            return;
        }
        qWarning() << "Couldn't set video format on v4l2 camera" << strerror(errno);
    }

    m_v4l2Info.formatInitialized = true;
    m_cameraBusy = false;

    m_bytesPerLine = fmt.fmt.pix.bytesperline;
    m_imageSize = std::max(fmt.fmt.pix.sizeimage, m_bytesPerLine * fmt.fmt.pix.height);

    switch (v4l2_colorspace(fmt.fmt.pix.colorspace)) {
    default:
    case V4L2_COLORSPACE_DCI_P3:
        m_colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
        break;
    case V4L2_COLORSPACE_REC709:
        m_colorSpace = QVideoFrameFormat::ColorSpace_BT709;
        break;
    case V4L2_COLORSPACE_JPEG:
        m_colorSpace = QVideoFrameFormat::ColorSpace_AdobeRgb;
        break;
    case V4L2_COLORSPACE_SRGB:
        // ##### is this correct???
        m_colorSpace = QVideoFrameFormat::ColorSpace_BT601;
        break;
    case V4L2_COLORSPACE_BT2020:
        m_colorSpace = QVideoFrameFormat::ColorSpace_BT2020;
        break;
    }

    v4l2_streamparm streamParam = {};
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    streamParam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    auto [num, den] = qRealToFraction(1./m_cameraFormat.maxFrameRate());
    streamParam.parm.capture.timeperframe = { (uint)num, (uint)den };
    m_v4l2FileDescriptor->call(VIDIOC_S_PARM, &streamParam);

    m_frameDuration = 1000000 * streamParam.parm.capture.timeperframe.numerator
            / streamParam.parm.capture.timeperframe.denominator;
}

void QV4L2Camera::initV4L2MemoryTransfer()
{
    if (m_cameraBusy)
        return;

    Q_ASSERT(!m_memoryTransfer);

    m_memoryTransfer = makeUserPtrMemoryTransfer(m_v4l2FileDescriptor, m_imageSize);

    if (m_memoryTransfer)
        return;

    if (errno == EBUSY) {
        setCameraBusy();
        return;
    }

    qCDebug(qLcV4L2Camera) << "Cannot init V4L2_MEMORY_USERPTR; trying V4L2_MEMORY_MMAP";

    m_memoryTransfer = makeMMapMemoryTransfer(m_v4l2FileDescriptor);

    if (!m_memoryTransfer) {
        qCWarning(qLcV4L2Camera) << "Cannot init v4l2 memory transfer," << qt_error_string(errno);
        emit error(QCamera::CameraError, QLatin1String("Cannot init V4L2 memory transfer"));
    }
}

void QV4L2Camera::stopCapturing()
{
    if (!m_memoryTransfer || !m_v4l2FileDescriptor)
        return;

    m_notifier = nullptr;

    if (!m_v4l2FileDescriptor->stopStream()) {
        // TODO: handle the case carefully to avoid possible memory corruption
        if (errno != ENODEV)
            qWarning() << "failed to stop capture";
    }

    m_memoryTransfer = nullptr;
    m_cameraBusy = false;
}

void QV4L2Camera::startCapturing()
{
    if (!m_v4l2FileDescriptor)
        return;

    setV4L2CameraFormat();
    initV4L2MemoryTransfer();

    if (m_cameraBusy || !m_memoryTransfer)
        return;

    if (!m_v4l2FileDescriptor->startStream()) {
        qWarning() << "Couldn't start v4l2 camera stream";
        return;
    }

    m_notifier =
            std::make_unique<QSocketNotifier>(m_v4l2FileDescriptor->get(), QSocketNotifier::Read);
    connect(m_notifier.get(), &QSocketNotifier::activated, this, &QV4L2Camera::readFrame);

    m_firstFrameTime = { -1, -1 };
}

QVideoFrameFormat QV4L2Camera::frameFormat() const
{
    auto result = QPlatformCamera::frameFormat();
    result.setColorSpace(m_colorSpace);
    return result;
}

QT_END_NAMESPACE

#include "moc_qv4l2camera_p.cpp"
