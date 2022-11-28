// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4l2camera_p.h"

#include <qdir.h>
#include <qmutex.h>
#include <qendian.h>
#include <private/qcameradevice_p.h>
#include <private/qabstractvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <private/qmultimediautils_p.h>
#include <private/qplatformmediadevices_p.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <private/qcore_unix_p.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

QT_BEGIN_NAMESPACE

QV4L2CameraDevices::QV4L2CameraDevices(QPlatformMediaIntegration *integration)
    : QPlatformVideoDevices(integration)
{
    deviceWatcher.addPath(QLatin1String("/dev"));
    connect(&deviceWatcher, &QFileSystemWatcher::directoryChanged, this, &QV4L2CameraDevices::checkCameras);
    doCheckCameras();
}

QList<QCameraDevice> QV4L2CameraDevices::videoDevices() const
{
    return cameras;
}

void QV4L2CameraDevices::checkCameras()
{
    doCheckCameras();
    videoInputsChanged();
}

const struct {
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

static QVideoFrameFormat::PixelFormat formatForV4L2Format(uint32_t v4l2Format)
{
    auto *f = formatMap;
    while (f->v4l2Format) {
        if (f->v4l2Format == v4l2Format)
            return f->fmt;
        ++f;
    }
    return QVideoFrameFormat::Format_Invalid;
}

static uint32_t v4l2FormatForPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    auto *f = formatMap;
    while (f->v4l2Format) {
        if (f->fmt == format)
            return f->v4l2Format;
        ++f;
    }
    return 0;
}


void QV4L2CameraDevices::doCheckCameras()
{
    cameras.clear();

    QDir dir(QLatin1String("/dev"));
    const auto devices = dir.entryList(QDir::System);

    bool first = true;

    for (auto device : devices) {
//        qDebug() << "device:" << device;
        if (!device.startsWith(QLatin1String("video")))
            continue;

        QByteArray file = QFile::encodeName(dir.filePath(device));
        int fd = open(file.constData(), O_RDONLY);
        if (fd < 0)
            continue;

        QCameraDevicePrivate *camera = nullptr;
        v4l2_fmtdesc formatDesc = {};

        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
            goto fail;

        if (cap.device_caps & V4L2_CAP_META_CAPTURE)
            goto fail;
        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
            goto fail;
        if (!(cap.capabilities & V4L2_CAP_STREAMING))
            goto fail;

        camera = new QCameraDevicePrivate;
        camera->id = file;
        camera->description = QString::fromUtf8((const char *)cap.card);
//        qDebug() << "found camera" << camera->id << camera->description;

        formatDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while (!ioctl(fd, VIDIOC_ENUM_FMT, &formatDesc)) {
            auto pixelFmt = formatForV4L2Format(formatDesc.pixelformat);
            qDebug() << "    " << pixelFmt;

            if (pixelFmt == QVideoFrameFormat::Format_Invalid) {
                ++formatDesc.index;
                continue;
            }

//            qDebug() << "frame sizes:";
            v4l2_frmsizeenum frameSize = {};
            frameSize.pixel_format = formatDesc.pixelformat;

            while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize)) {
                if (frameSize.type != V4L2_FRMSIZE_TYPE_DISCRETE)
                    continue;

                QSize resolution(frameSize.discrete.width, frameSize.discrete.height);
                float min = 1e10;
                float max = 0;

                v4l2_frmivalenum frameInterval = {};
                frameInterval.pixel_format = formatDesc.pixelformat;
                frameInterval.width = frameSize.discrete.width;
                frameInterval.height = frameSize.discrete.height;

                while (!ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frameInterval)) {
                    if (frameInterval.type != V4L2_FRMIVAL_TYPE_DISCRETE)
                        continue;
                    ++frameInterval.index;
                    float rate = float(frameInterval.discrete.denominator)/float(frameInterval.discrete.numerator);
                    if (rate > max)
                        max = rate;
                    if (rate < min)
                        min = rate;
                }

//                qDebug() << "    " << resolution << min << max;
                ++frameSize.index;

                if (min <= max) {
                    QCameraFormatPrivate *fmt = new QCameraFormatPrivate;
                    fmt->pixelFormat = pixelFmt;
                    fmt->resolution = resolution;
                    fmt->minFrameRate = min;
                    fmt->maxFrameRate = max;
                    camera->videoFormats.append(fmt->create());
                    camera->photoResolutions.append(resolution);
                }
            }

            ++formatDesc.index;
        }

        // first camera is default
        camera->isDefault = first;
        first = false;

        cameras.append(camera->create());

        close(fd);
        continue;

      fail:
        if (camera)
              delete camera;
        close(fd);
    }
}

class QV4L2VideoBuffer : public QAbstractVideoBuffer
{
public:
    QV4L2VideoBuffer(QV4L2CameraBuffers *d, int index)
        : QAbstractVideoBuffer(QVideoFrame::NoHandle, nullptr)
        , index(index)
        , d(d)
    {}
    ~QV4L2VideoBuffer()
    {
        d->release(index);
    }

    QVideoFrame::MapMode mapMode() const override { return m_mode; }
    MapData map(QVideoFrame::MapMode mode) override {
        m_mode = mode;
        return d->v4l2FileDescriptor >= 0 ? data : MapData{};
    }
    void unmap() override {
        m_mode = QVideoFrame::NotMapped;
    }

    QVideoFrame::MapMode m_mode = QVideoFrame::NotMapped;
    MapData data;
    int index = 0;
    QExplicitlySharedDataPointer<QV4L2CameraBuffers> d;
};

QV4L2CameraBuffers::~QV4L2CameraBuffers()
{
    QMutexLocker locker(&mutex);
    Q_ASSERT(v4l2FileDescriptor < 0);
    unmapBuffers();
}



void QV4L2CameraBuffers::release(int index)
{
    QMutexLocker locker(&mutex);
    if (v4l2FileDescriptor < 0 || index >= mappedBuffers.size())
        return;

    struct v4l2_buffer buf = {};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    if (ioctl(v4l2FileDescriptor, VIDIOC_QBUF, &buf) < 0)
        qWarning() << "Couldn't release V4L2 buffer" << errno << strerror(errno) << index;
}

void QV4L2CameraBuffers::unmapBuffers()
{
    for (const auto &b : std::as_const(mappedBuffers))
        munmap(b.data, b.size);
    mappedBuffers.clear();
}

QV4L2Camera::QV4L2Camera(QCamera *camera)
    : QPlatformCamera(camera)
{
}

QV4L2Camera::~QV4L2Camera()
{
    setActive(false);
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
        setV4L2CameraFormat();
        initMMap();
        startCapturing();
    } else {
        stopCapturing();
    }
    emit newVideoFrame({});

    emit activeChanged(active);
}

void QV4L2Camera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;
    if (m_active)
        stopCapturing();

    closeV4L2Fd();

    m_cameraDevice = camera;
    resolveCameraFormat({});

    initV4L2Controls();

    if (m_active) {
        setV4L2CameraFormat();
        initMMap();
        startCapturing();
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
        initV4L2Controls();
        setV4L2CameraFormat();
        initMMap();
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
    if (!focusDist && !v4l2RangedFocus)
        return;

    switch (mode) {
    default:
    case QCamera::FocusModeAuto:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 1);
        if (v4l2RangedFocus)
            setV4L2Parameter(V4L2_CID_AUTO_FOCUS_RANGE, V4L2_AUTO_FOCUS_RANGE_AUTO);
        break;
    case QCamera::FocusModeAutoNear:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 1);
        if (v4l2RangedFocus)
            setV4L2Parameter(V4L2_CID_AUTO_FOCUS_RANGE, V4L2_AUTO_FOCUS_RANGE_MACRO);
        else if (focusDist)
            setV4L2Parameter(V4L2_CID_FOCUS_ABSOLUTE, v4l2MinFocus);
        break;
    case QCamera::FocusModeAutoFar:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 1);
        if (v4l2RangedFocus)
            setV4L2Parameter(V4L2_CID_AUTO_FOCUS_RANGE, V4L2_AUTO_FOCUS_RANGE_INFINITY);
        break;
    case QCamera::FocusModeInfinity:
        setV4L2Parameter(V4L2_CID_FOCUS_AUTO, 0);
        setV4L2Parameter(V4L2_CID_FOCUS_ABSOLUTE, v4l2MaxFocus);
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
    int distance = v4l2MinFocus + int((v4l2MaxFocus - v4l2MinFocus)*d);
    setV4L2Parameter(V4L2_CID_FOCUS_ABSOLUTE, distance);
    focusDistanceChanged(d);
}

void QV4L2Camera::zoomTo(float factor, float)
{
    if (v4l2MaxZoom == v4l2MinZoom)
        return;
    factor = qBound(1., factor, 2.);
    int zoom = v4l2MinZoom + (factor - 1.)*(v4l2MaxZoom - v4l2MinZoom);
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
    if (!v4l2FlashSupported || mode == QCamera::FlashOn)
        return;
    setV4L2Parameter(V4L2_CID_FLASH_LED_MODE, mode == QCamera::FlashAuto ? V4L2_FLASH_LED_MODE_FLASH : V4L2_FLASH_LED_MODE_NONE);
    flashModeChanged(mode);
}

bool QV4L2Camera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    if (v4l2FlashSupported && mode == QCamera::FlashAuto)
        return true;
    return mode == QCamera::FlashOff;
}

bool QV4L2Camera::isFlashReady() const
{
    struct v4l2_queryctrl queryControl;
    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_WHITE_BALANCE;

    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0)
        return true;

    return false;
}

void QV4L2Camera::setTorchMode(QCamera::TorchMode mode)
{
    if (!v4l2TorchSupported || mode == QCamera::TorchOn)
        return;
    setV4L2Parameter(V4L2_CID_FLASH_LED_MODE, mode == QCamera::TorchOn ? V4L2_FLASH_LED_MODE_TORCH : V4L2_FLASH_LED_MODE_NONE);
    torchModeChanged(mode);
}

bool QV4L2Camera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (mode == QCamera::TorchOn)
        return v4l2TorchSupported;
    return mode == QCamera::TorchOff;
}

void QV4L2Camera::setExposureMode(QCamera::ExposureMode mode)
{
    if (v4l2AutoExposureSupported && v4l2ManualExposureSupported) {
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
    if (v4l2ManualExposureSupported && v4l2AutoExposureSupported)
        return mode == QCamera::ExposureManual;
    return false;
}

void QV4L2Camera::setExposureCompensation(float compensation)
{
    if ((v4l2MinExposureAdjustment != 0 || v4l2MaxExposureAdjustment != 0)) {
        int value = qBound(v4l2MinExposureAdjustment, (int)(compensation*1000), v4l2MaxExposureAdjustment);
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
    if (v4l2ManualExposureSupported && v4l2AutoExposureSupported) {
        int exposure = qBound(v4l2MinExposure, qRound(secs*10000.), v4l2MaxExposure);
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
    if (v4l2AutoWhiteBalanceSupported && v4l2ColorTemperatureSupported)
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
    if (!d)
        return;

    v4l2_buffer buf = {};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_DQBUF, &buf) < 0) {
        if (errno == ENODEV) {
            // camera got removed while being active
            stopCapturing();
            closeV4L2Fd();
            return;
        }
        if (errno != EAGAIN)
            qWarning() << "error calling VIDIOC_DQBUF" << errno << strerror(errno);
    }

    Q_ASSERT(qsizetype(buf.index) < d->mappedBuffers.size());
    int i = buf.index;

//    auto textureDesc = QVideoTextureHelper::textureDescription(m_format.pixelFormat());

    QV4L2VideoBuffer *buffer = new QV4L2VideoBuffer(d.get(), i);
    buffer->data.nPlanes = 1;
    buffer->data.bytesPerLine[0] = bytesPerLine;
    buffer->data.data[0] = (uchar *)d->mappedBuffers.at(i).data;
    buffer->data.size[0] = d->mappedBuffers.at(i).size;
    QVideoFrameFormat fmt(m_cameraFormat.resolution(), m_cameraFormat.pixelFormat());
    fmt.setColorSpace(colorSpace);
//    qDebug() << "got a frame" << d->mappedBuffers.at(i).data << d->mappedBuffers.at(i).size << fmt << i;
    QVideoFrame frame(buffer, fmt);

    if (firstFrameTime.tv_sec == -1)
        firstFrameTime = buf.timestamp;
    qint64 secs = buf.timestamp.tv_sec - firstFrameTime.tv_sec;
    qint64 usecs = buf.timestamp.tv_usec - firstFrameTime.tv_usec;
    frame.setStartTime(secs*1000000 + usecs);
    frame.setEndTime(frame.startTime() + frameDuration);

    emit newVideoFrame(frame);
}

void QV4L2Camera::setCameraBusy()
{
    cameraBusy = true;
    error(QCamera::CameraError, tr("Camera is in use."));
}

void QV4L2Camera::initV4L2Controls()
{
    v4l2AutoWhiteBalanceSupported = false;
    v4l2ColorTemperatureSupported = false;
    v4l2RangedFocus = false;
    v4l2FlashSupported = false;
    v4l2TorchSupported = false;
    QCamera::Features features;


    const QByteArray deviceName = m_cameraDevice.id();
    Q_ASSERT(!deviceName.isEmpty());

    closeV4L2Fd();
    Q_ASSERT(!d);

    d = new QV4L2CameraBuffers;

    d->v4l2FileDescriptor = qt_safe_open(deviceName.constData(), O_RDWR);
    if (d->v4l2FileDescriptor == -1) {
        qWarning() << "Unable to open the camera" << deviceName
                   << "for read to query the parameter info:" << qt_error_string(errno);
        return;
    }
    qDebug() << "FD=" << d->v4l2FileDescriptor;

    struct v4l2_queryctrl queryControl;
    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_WHITE_BALANCE;

    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2AutoWhiteBalanceSupported = true;
        setV4L2Parameter(V4L2_CID_AUTO_WHITE_BALANCE, true);
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2MinColorTemp = queryControl.minimum;
        v4l2MaxColorTemp = queryControl.maximum;
        v4l2ColorTemperatureSupported = true;
        features |= QCamera::Feature::ColorTemperature;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_EXPOSURE_AUTO;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2AutoExposureSupported = true;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2ManualExposureSupported = true;
        v4l2MinExposure = queryControl.minimum;
        v4l2MaxExposure = queryControl.maximum;
        features |= QCamera::Feature::ManualExposureTime;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_EXPOSURE_BIAS;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2MinExposureAdjustment = queryControl.minimum;
        v4l2MaxExposureAdjustment = queryControl.maximum;
        features |= QCamera::Feature::ExposureCompensation;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_ISO_SENSITIVITY_AUTO;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        queryControl.id = V4L2_CID_ISO_SENSITIVITY;
        if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            features |= QCamera::Feature::IsoSensitivity;
            minIsoChanged(queryControl.minimum);
            maxIsoChanged(queryControl.minimum);
        }
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_FOCUS_ABSOLUTE;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2MinExposureAdjustment = queryControl.minimum;
        v4l2MaxExposureAdjustment = queryControl.maximum;
        features |= QCamera::Feature::FocusDistance;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_AUTO_FOCUS_RANGE;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2RangedFocus = true;
    }

    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_FLASH_LED_MODE;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2FlashSupported = queryControl.minimum <= V4L2_FLASH_LED_MODE_FLASH && queryControl.maximum >= V4L2_FLASH_LED_MODE_FLASH;
        v4l2TorchSupported = queryControl.minimum <= V4L2_FLASH_LED_MODE_TORCH && queryControl.maximum >= V4L2_FLASH_LED_MODE_TORCH;
    }

    v4l2MinZoom = 0;
    v4l2MaxZoom = 0;
    ::memset(&queryControl, 0, sizeof(queryControl));
    queryControl.id = V4L2_CID_ZOOM_ABSOLUTE;
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYCTRL, &queryControl) == 0) {
        v4l2MinZoom = queryControl.minimum;
        v4l2MaxZoom = queryControl.maximum;
    }
    // zoom factors are in arbitrary units, so we simply normalize them to go from 1 to 2
    // if they are different
    minimumZoomFactorChanged(1);
    maximumZoomFactorChanged(v4l2MinZoom != v4l2MaxZoom ? 2 : 1);

    supportedFeaturesChanged(features);
}

void QV4L2Camera::closeV4L2Fd()
{
    if (d && d->v4l2FileDescriptor >= 0) {
        QMutexLocker locker(&d->mutex);
        d->unmapBuffers();
        qt_safe_close(d->v4l2FileDescriptor);
        d->v4l2FileDescriptor = -1;
    }
    d = nullptr;
}

int QV4L2Camera::setV4L2ColorTemperature(int temperature)
{
    struct v4l2_control control;
    ::memset(&control, 0, sizeof(control));

    if (v4l2AutoWhiteBalanceSupported) {
        setV4L2Parameter(V4L2_CID_AUTO_WHITE_BALANCE, temperature == 0 ? true : false);
    } else if (temperature == 0) {
        temperature = 5600;
    }

    if (temperature != 0 && v4l2ColorTemperatureSupported) {
        temperature = qBound(v4l2MinColorTemp, temperature, v4l2MaxColorTemp);
        if (!setV4L2Parameter(V4L2_CID_WHITE_BALANCE_TEMPERATURE, qBound(v4l2MinColorTemp, temperature, v4l2MaxColorTemp)))
            temperature = 0;
    } else {
        temperature = 0;
    }

    return temperature;
}

bool QV4L2Camera::setV4L2Parameter(quint32 id, qint32 value)
{
    struct v4l2_control control{id, value};
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_S_CTRL, &control) != 0) {
        qWarning() << "Unable to set the V4L2 Parameter" << Qt::hex << id << "to" << value << qt_error_string(errno);
        return false;
    }
    return true;
}

int QV4L2Camera::getV4L2Parameter(quint32 id) const
{
    struct v4l2_control control{id, 0};
    if (::ioctl(d->v4l2FileDescriptor, VIDIOC_G_CTRL, &control) != 0) {
        qWarning() << "Unable to get the V4L2 Parameter" << Qt::hex << id << qt_error_string(errno);
        return 0;
    }
    return control.value;
}

void QV4L2Camera::setV4L2CameraFormat()
{
    Q_ASSERT(!m_cameraFormat.isNull());
    qDebug() << "XXXXX" << this << m_cameraDevice.id() << m_cameraFormat.pixelFormat() << m_cameraFormat.resolution();

    v4l2_format fmt = {};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    auto size = m_cameraFormat.resolution();
    fmt.fmt.pix.width = size.width();
    fmt.fmt.pix.height = size.height();
    fmt.fmt.pix.pixelformat = v4l2FormatForPixelFormat(m_cameraFormat.pixelFormat());
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    qDebug() << "setting camera format to" << size;

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_S_FMT, &fmt) < 0) {
        if (errno == EBUSY) {
            setCameraBusy();
            return;
        }
        qWarning() << "Couldn't set video format on v4l2 camera" << strerror(errno);
    }

    bytesPerLine = fmt.fmt.pix.bytesperline;

    switch (v4l2_colorspace(fmt.fmt.pix.colorspace)) {
    default:
    case V4L2_COLORSPACE_DCI_P3:
        colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
        break;
    case V4L2_COLORSPACE_REC709:
        colorSpace = QVideoFrameFormat::ColorSpace_BT709;
        break;
    case V4L2_COLORSPACE_JPEG:
        colorSpace = QVideoFrameFormat::ColorSpace_AdobeRgb;
        break;
    case V4L2_COLORSPACE_SRGB:
        // ##### is this correct???
        colorSpace = QVideoFrameFormat::ColorSpace_BT601;
        break;
    case V4L2_COLORSPACE_BT2020:
        colorSpace = QVideoFrameFormat::ColorSpace_BT2020;
        break;
    }

    v4l2_streamparm streamParam = {};
    streamParam.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    streamParam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    auto [num, den] = qRealToFraction(1./m_cameraFormat.maxFrameRate());
    streamParam.parm.capture.timeperframe = { (uint)num, (uint)den };
    ioctl(d->v4l2FileDescriptor, VIDIOC_S_PARM, &streamParam);

    frameDuration = 1000000*streamParam.parm.capture.timeperframe.numerator
                    /streamParam.parm.capture.timeperframe.denominator;
}

void QV4L2Camera::initMMap()
{
    if (cameraBusy)
        return;

    v4l2_requestbuffers req = {};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_REQBUFS, &req) < 0) {
        if (errno == EBUSY)
            setCameraBusy();
        qWarning() << "requesting mmap'ed buffers failed" << strerror(errno);
        return;
    }

    if (req.count < 2) {
        qWarning() << "Can't map 2 or more buffers";
        return;
    }

    for (uint32_t n = 0; n < req.count; ++n) {
        v4l2_buffer buf = {};
        buf.index = n;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(d->v4l2FileDescriptor, VIDIOC_QUERYBUF, &buf) != 0) {
            qWarning() << "Can't map buffer" << n;
            return;
        }

        QV4L2CameraBuffers::MappedBuffer buffer;
        buffer.size = buf.length;
        buffer.data = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                           d->v4l2FileDescriptor, buf.m.offset);

        if (buffer.data == MAP_FAILED) {
            qWarning() << "mmap failed" << n << buf.length << buf.m.offset;
            return;
        }

        d->mappedBuffers.append(buffer);
    }

}

void QV4L2Camera::stopCapturing()
{
    if (!d)
        return;

    delete notifier;
    notifier = nullptr;

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_STREAMOFF, &type) < 0) {
        if (errno != ENODEV)
            qWarning() << "failed to stop capture";
    }
    cameraBusy = false;
}

void QV4L2Camera::startCapturing()
{
    if (cameraBusy)
        return;

    // #### better to use the user data method instead of mmap???
    qsizetype i;

    for (i = 0; i < d->mappedBuffers.size(); ++i) {
        v4l2_buffer buf = {};
        buf.index = i;
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(d->v4l2FileDescriptor, VIDIOC_QBUF, &buf) < 0) {
            qWarning() << "failed to set up mapped buffer";
            return;
        }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(d->v4l2FileDescriptor, VIDIOC_STREAMON, &type) < 0)
        qWarning() << "failed to start capture";

    notifier = new QSocketNotifier(d->v4l2FileDescriptor, QSocketNotifier::Read);
    connect(notifier, &QSocketNotifier::activated, this, &QV4L2Camera::readFrame);

    firstFrameTime = { -1, -1 };
}

QT_END_NAMESPACE
