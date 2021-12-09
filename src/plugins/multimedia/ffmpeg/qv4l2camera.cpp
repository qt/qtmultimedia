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

#include "qv4l2camera_p.h"

#include <qdir.h>
#include <qmutex.h>
#include <qendian.h>
#include <private/qcameradevice_p.h>
#include <private/qabstractvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>
#include <private/qmultimediautils_p.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <private/qcore_unix_p.h>
#include <sys/mman.h>

#include <linux/videodev2.h>

QT_BEGIN_NAMESPACE

static struct Holder {
    ~Holder() { delete instance; }
    QBasicMutex mutex;
    QV4L2CameraDevices *instance = nullptr;
} holder;

QV4L2CameraDevices::QV4L2CameraDevices(QPlatformMediaIntegration *integration)
    : integration(integration)
{
    deviceWatcher.addPath(QLatin1String("/dev"));
    connect(&deviceWatcher, &QFileSystemWatcher::directoryChanged, this, &QV4L2CameraDevices::checkCameras);
    doCheckCameras();
}

QV4L2CameraDevices *QV4L2CameraDevices::instance(QPlatformMediaIntegration *integration)
{
    QMutexLocker locker(&holder.mutex);
    if (!holder.instance)
        holder.instance = new QV4L2CameraDevices(integration);
    return holder.instance;
}

QList<QCameraDevice> QV4L2CameraDevices::cameraDevices() const
{
    QMutexLocker locker(&holder.mutex);
    return cameras;
}

void QV4L2CameraDevices::checkCameras()
{
    QMutexLocker locker(&holder.mutex);
    doCheckCameras();
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
    // ###### Re-enable jpeg for better performance
//    { QVideoFrameFormat::Format_Jpeg,     V4L2_PIX_FMT_MJPEG   },
//    { QVideoFrameFormat::Format_Jpeg,     V4L2_PIX_FMT_JPEG    },
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
        struct v4l2_fmtdesc formatDesc;

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

        formatDesc = {
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE
        };

        while (!ioctl(fd, VIDIOC_ENUM_FMT, &formatDesc)) {
            auto pixelFmt = formatForV4L2Format(formatDesc.pixelformat);
            qDebug() << "    " << pixelFmt;

            if (pixelFmt == QVideoFrameFormat::Format_Invalid) {
                ++formatDesc.index;
                continue;
            }

//            qDebug() << "frame sizes:";
            v4l2_frmsizeenum frameSize = {
                .pixel_format = formatDesc.pixelformat
            };
            while (!ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frameSize)) {
                if (frameSize.type != V4L2_FRMSIZE_TYPE_DISCRETE)
                    continue;

                QSize resolution(frameSize.discrete.width, frameSize.discrete.height);
                float min = 1e10;
                float max = 0;

                v4l2_frmivalenum frameInterval = {
                    .pixel_format = formatDesc.pixelformat,
                    .width = frameSize.discrete.width,
                    .height = frameSize.discrete.height
                };
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

    integration->videoInputsChanged();
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
        return data;
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

    for (const auto &b : qAsConst(mappedBuffers)) {
        munmap(b.data, b.size);
    }
    mappedBuffers.clear();
}



void QV4L2CameraBuffers::release(int index)
{
    QMutexLocker locker(&mutex);
    if (v4l2FileDescriptor < 0)
        return;

    struct v4l2_buffer buf = {};

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;

    if (ioctl(v4l2FileDescriptor, VIDIOC_QBUF, &buf) < 0)
        qWarning() << "Couldn't release V4L2 buffer" << errno << strerror(errno) << index;
}

QV4L2Camera::QV4L2Camera(QCamera *camera)
    : QPlatformCamera(camera)
{
}

QV4L2Camera::~QV4L2Camera()
{
    setActive(false);
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
    if (m_active) {
        stopCapturing();
        closeV4L2Fd();
    }

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

    // ####
}

bool QV4L2Camera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    return mode == QCamera::FocusModeAuto;
}

void QV4L2Camera::setFlashMode(QCamera::FlashMode mode)
{
    Q_UNUSED(mode);

}

bool QV4L2Camera::isFlashModeSupported(QCamera::FlashMode mode) const
{
    return mode == QCamera::FlashAuto;
}

bool QV4L2Camera::isFlashReady() const
{
    return false;
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
    struct v4l2_buffer buf = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP
    };

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_DQBUF, &buf) < 0) {
        if (errno != EAGAIN)
            qWarning() << "error calling VIDIOC_DQBUF";
    }

    Q_ASSERT(buf.index < d->mappedBuffers.size());
    int i = buf.index;

//    auto textureDesc = QVideoTextureHelper::textureDescription(m_format.pixelFormat());

    QV4L2VideoBuffer *buffer = new QV4L2VideoBuffer(d.get(), i);
    buffer->data.nPlanes = 1;
    buffer->data.bytesPerLine[0] = bytesPerLine;
    buffer->data.data[0] = (uchar *)d->mappedBuffers.at(i).data;
    buffer->data.size[0] = d->mappedBuffers.at(i).size;
    QVideoFrameFormat fmt(m_cameraFormat.resolution(), m_cameraFormat.pixelFormat());
    fmt.setYCbCrColorSpace(colorSpace);
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

void QV4L2Camera::initV4L2Controls()
{
    v4l2AutoWhiteBalanceSupported = false;
    v4l2ColorTemperatureSupported = false;
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

    supportedFeaturesChanged(features);
}

void QV4L2Camera::closeV4L2Fd()
{
    if (d && d->v4l2FileDescriptor >= 0) {
        QMutexLocker locker(&d->mutex);
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

    struct v4l2_format fmt = { .type = V4L2_BUF_TYPE_VIDEO_CAPTURE };

    auto size = m_cameraFormat.resolution();
    fmt.fmt.pix.width = size.width();
    fmt.fmt.pix.height = size.height();
    fmt.fmt.pix.pixelformat = v4l2FormatForPixelFormat(m_cameraFormat.pixelFormat());
    fmt.fmt.pix.field = V4L2_FIELD_ANY;

    qDebug() << "setting camera format to" << size;

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_S_FMT, &fmt) < 0)
        qWarning() << "Couldn't set video format on v4l2 camera";

    bytesPerLine = fmt.fmt.pix.bytesperline;

    switch (v4l2_colorspace(fmt.fmt.pix.colorspace)) {
    default:
    case V4L2_COLORSPACE_DCI_P3:
        colorSpace = QVideoFrameFormat::YCbCr_Undefined;
        break;
    case V4L2_COLORSPACE_REC709:
        colorSpace = QVideoFrameFormat::YCbCr_BT709;
        break;
    case V4L2_COLORSPACE_JPEG:
        colorSpace = QVideoFrameFormat::YCbCr_JPEG;
        break;
    case V4L2_COLORSPACE_SRGB:
        // ##### is this correct???
        colorSpace = QVideoFrameFormat::YCbCr_BT601;
        break;
    case V4L2_COLORSPACE_BT2020:
        colorSpace = QVideoFrameFormat::YCbCr_BT2020;
        break;
    }

    v4l2_streamparm streamParam = {
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE
    };
    streamParam.parm.capture.capability = V4L2_CAP_TIMEPERFRAME;
    int num, den;
    qt_real_to_fraction(1./m_cameraFormat.maxFrameRate(), &num, &den);
    streamParam.parm.capture.timeperframe = { (uint)num, (uint)den };
    ioctl(d->v4l2FileDescriptor, VIDIOC_S_PARM, &streamParam);

    frameDuration = 1000000*streamParam.parm.capture.timeperframe.numerator
                    /streamParam.parm.capture.timeperframe.denominator;
}

void QV4L2Camera::initMMap()
{
    struct v4l2_requestbuffers req = {
        .count = 4,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
        .memory = V4L2_MEMORY_MMAP
    };

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_REQBUFS, &req) < 0) {
        qWarning() << "requesting mmap'ed buffers failed";
        return;
    }

    if (req.count < 2) {
        qWarning() << "Can't map 2 or more buffers";
        return;
    }

    for (uint32_t n = 0; n < req.count; ++n) {
        struct v4l2_buffer buf = {
            .index       = n,
            .type        = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory      = V4L2_MEMORY_MMAP
        };


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
    delete notifier;
    notifier = nullptr;

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(d->v4l2FileDescriptor, VIDIOC_STREAMOFF, &type) < 0)
        qWarning() << "failed to stop capture";
}

void QV4L2Camera::startCapturing()
{
    // #### better to use the user data method instead of mmap???
    unsigned int i;

    for (i = 0; i < d->mappedBuffers.size(); ++i) {
        struct v4l2_buffer buf = {
            .index = i,
            .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
            .memory = V4L2_MEMORY_MMAP,
        };

        if (ioctl(d->v4l2FileDescriptor, VIDIOC_QBUF, &buf) < 0) {
            qWarning() << "failed to setup mapped buffer";
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
