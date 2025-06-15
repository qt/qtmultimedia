// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <mediacapture/qgstreamercamera_p.h>

#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/private/qcameradevice_p.h>
#include <QtCore/qdebug.h>

#include <common/qgst_debug_p.h>
#include <qgstreamervideodevices_p.h>
#include <qgstreamerintegration_p.h>

#if QT_CONFIG(linux_v4l)
#include <linux/videodev2.h>
#include <private/qcore_unix_p.h>
#endif


QT_BEGIN_NAMESPACE

q23::expected<QPlatformCamera *, QString> QGstreamerCamera::create(QCamera *camera)
{
    static const auto error = qGstErrorMessageIfElementsNotAvailable(
            "videotestsrc", "capsfilter", "videoconvert", "videoscale", "identity");
    if (error)
        return q23::unexpected{ *error };

    return new QGstreamerCamera(camera);
}

QGstreamerCamera::QGstreamerCamera(QCamera *camera)
    : QGstreamerCameraBase(camera),
      gstCameraBin{
          QGstBin::create("camerabin"),
      },
      gstCamera{
          QGstElement::createFromFactory("videotestsrc"),
      },
      gstCapsFilter{
          QGstElement::createFromFactory("capsfilter", "videoCapsFilter"),
      },
      gstDecode{
          QGstElement::createFromFactory("identity"),
      },
      gstVideoConvert{
          QGstElement::createFromFactory("videoconvert", "videoConvert"),
      },
      gstVideoScale{
          QGstElement::createFromFactory("videoscale", "videoScale"),
      }
{
    gstCameraBin.add(gstCamera, gstCapsFilter, gstDecode, gstVideoConvert, gstVideoScale);
    qLinkGstElements(gstCamera, gstCapsFilter, gstDecode, gstVideoConvert, gstVideoScale);
    gstCameraBin.addGhostPad(gstVideoScale, "src");
}

QGstreamerCamera::~QGstreamerCamera()
{
    gstCameraBin.setStateSync(GST_STATE_NULL);
}

bool QGstreamerCamera::isActive() const
{
    return m_active;
}

void QGstreamerCamera::setActive(bool active)
{
    if (m_active == active)
        return;
    if (m_cameraDevice.isNull() && active)
        return;

    m_active = active;

    emit activeChanged(active);
}

void QGstreamerCamera::setCamera(const QCameraDevice &camera)
{
    using namespace Qt::Literals;

    if (m_cameraDevice == camera)
        return;

    m_cameraDevice = camera;

    QGstElement gstNewCamera;
    if (camera.isNull()) {
        gstNewCamera = QGstElement::createFromFactory("videotestsrc");
    } else {
        auto *integration = static_cast<QGstreamerIntegration *>(QGstreamerIntegration::instance());
        GstDevice *device = integration->videoDevice(camera.id());

        if (!device) {
            updateError(QCamera::Error::CameraError,
                        u"Failed to create GstDevice for camera: "_s
                                + QString::fromUtf8(camera.id()));
            return;
        }

        gstNewCamera = QGstElement::createFromDevice(device, "camerasrc");
        QUniqueGstStructureHandle properties{
            gst_device_get_properties(device),
        };

        if (properties) {
            QGstStructureView propertiesView{ properties };
            if (propertiesView.name() == "v4l2deviceprovider")
                m_v4l2DevicePath = QString::fromUtf8(propertiesView["device.path"].toString());
        }
    }

    QCameraFormat f = findBestCameraFormat(camera);
    auto caps = QGstCaps::fromCameraFormat(f);
    auto gstNewDecode = QGstElement::createFromFactory(
            f.pixelFormat() == QVideoFrameFormat::Format_Jpeg ? "jpegdec" : "identity");

    m_currentCameraFormat = f;

    updateCamera([&] {
        qUnlinkGstElements(gstCamera, gstCapsFilter, gstDecode, gstVideoConvert);
        gstCameraBin.stopAndRemoveElements(gstCamera, gstDecode);

        gstCapsFilter.set("caps", caps);

        gstCamera = std::move(gstNewCamera);
        gstDecode = std::move(gstNewDecode);

        gstCameraBin.add(gstCamera, gstDecode);
        qLinkGstElements(gstCamera, gstCapsFilter, gstDecode, gstVideoConvert);

        gstCameraBin.syncChildrenState();
    });

    updateCameraProperties();
}

bool QGstreamerCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!format.isNull() && !m_cameraDevice.videoFormats().contains(format))
        return false;

    QCameraFormat f = format;
    if (f.isNull())
        f = findBestCameraFormat(m_cameraDevice);

    if (f == m_currentCameraFormat)
        return true;

    m_currentCameraFormat = f;

    auto caps = QGstCaps::fromCameraFormat(f);

    auto newGstDecode = QGstElement::createFromFactory(
            f.pixelFormat() == QVideoFrameFormat::Format_Jpeg ? "jpegdec" : "identity");

    updateCamera([&] {
        qUnlinkGstElements(gstCamera, gstCapsFilter, gstDecode, gstVideoConvert);
        gstCameraBin.stopAndRemoveElements(gstDecode);

        gstCapsFilter.set("caps", caps);

        gstDecode = std::move(newGstDecode);

        gstCameraBin.add(gstDecode);
        qLinkGstElements(gstCamera, gstCapsFilter, gstDecode, gstVideoConvert);
        gstCameraBin.syncChildrenState();
    });

    return true;
}

void QGstreamerCamera::updateCameraProperties()
{
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        initV4L2Controls();
        return;
    }
#endif
#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography())
        gst_photography_set_white_balance_mode(p, GST_PHOTOGRAPHY_WB_MODE_AUTO);
    QCamera::Features f = QCamera::Feature::ColorTemperature | QCamera::Feature::ExposureCompensation |
                          QCamera::Feature::IsoSensitivity | QCamera::Feature::ManualExposureTime;
    supportedFeaturesChanged(f);
#endif

}

#if QT_CONFIG(gstreamer_photography)
GstPhotography *QGstreamerCamera::photography() const
{
    if (gstCamera && GST_IS_PHOTOGRAPHY(gstCamera.element()))
        return GST_PHOTOGRAPHY(gstCamera.element());
    return nullptr;
}
#endif

void QGstreamerCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (mode == focusMode())
        return;

#if QT_CONFIG(gstreamer_photography)
    auto p = photography();
    if (p) {
        GstPhotographyFocusMode photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_CONTINUOUS_NORMAL;

        switch (mode) {
        case QCamera::FocusModeAutoNear:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MACRO;
            break;
        case QCamera::FocusModeAutoFar:
            // not quite, but hey :)
            Q_FALLTHROUGH();
        case QCamera::FocusModeHyperfocal:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_HYPERFOCAL;
            break;
        case QCamera::FocusModeInfinity:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_INFINITY;
            break;
        case QCamera::FocusModeManual:
            photographyMode = GST_PHOTOGRAPHY_FOCUS_MODE_MANUAL;
            break;
        default: // QCamera::FocusModeAuto:
            break;
        }

        if (gst_photography_set_focus_mode(p, photographyMode))
            focusModeChanged(mode);
    }
#endif
}

bool QGstreamerCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
#if QT_CONFIG(gstreamer_photography)
    if (photography())
        return true;
#endif
    return mode == QCamera::FocusModeAuto;
}

void QGstreamerCamera::setFlashMode(QCamera::FlashMode mode)
{
    Q_UNUSED(mode);

#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        GstPhotographyFlashMode flashMode;
        gst_photography_get_flash_mode(p, &flashMode);

        switch (mode) {
        case QCamera::FlashAuto:
            flashMode = GST_PHOTOGRAPHY_FLASH_MODE_AUTO;
            break;
        case QCamera::FlashOff:
            flashMode = GST_PHOTOGRAPHY_FLASH_MODE_OFF;
            break;
        case QCamera::FlashOn:
            flashMode = GST_PHOTOGRAPHY_FLASH_MODE_ON;
            break;
        }

        if (gst_photography_set_flash_mode(p, flashMode))
            flashModeChanged(mode);
    }
#endif
}

bool QGstreamerCamera::isFlashModeSupported(QCamera::FlashMode mode) const
{
#if QT_CONFIG(gstreamer_photography)
    if (photography())
        return true;
#endif

    return mode == QCamera::FlashAuto;
}

bool QGstreamerCamera::isFlashReady() const
{
#if QT_CONFIG(gstreamer_photography)
    if (photography())
        return true;
#endif

    return false;
}

void QGstreamerCamera::setExposureMode(QCamera::ExposureMode mode)
{
    Q_UNUSED(mode);
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera() && v4l2AutoExposureSupported && v4l2ManualExposureSupported) {
        if (mode != QCamera::ExposureAuto && mode != QCamera::ExposureManual)
            return;
        int value = QCamera::ExposureAuto ? V4L2_EXPOSURE_AUTO : V4L2_EXPOSURE_MANUAL;
        setV4L2Parameter(V4L2_CID_EXPOSURE_AUTO, value);
        exposureModeChanged(mode);
        return;
    }
#endif

#if QT_CONFIG(gstreamer_photography)
    auto *p = photography();
    if (!p)
        return;

    GstPhotographySceneMode sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_AUTO;

    switch (mode) {
    case QCamera::ExposureManual:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_MANUAL;
        break;
    case QCamera::ExposurePortrait:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_PORTRAIT;
        break;
    case QCamera::ExposureSports:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SPORT;
        break;
    case QCamera::ExposureNight:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_NIGHT;
        break;
    case QCamera::ExposureAuto:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_AUTO;
        break;
    case QCamera::ExposureLandscape:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_LANDSCAPE;
        break;
    case QCamera::ExposureSnow:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SNOW;
        break;
    case QCamera::ExposureBeach:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_BEACH;
        break;
    case QCamera::ExposureAction:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_ACTION;
        break;
    case QCamera::ExposureNightPortrait:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_NIGHT_PORTRAIT;
        break;
    case QCamera::ExposureTheatre:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_THEATRE;
        break;
    case QCamera::ExposureSunset:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_SUNSET;
        break;
    case QCamera::ExposureSteadyPhoto:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_STEADY_PHOTO;
        break;
    case QCamera::ExposureFireworks:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_FIREWORKS;
        break;
    case QCamera::ExposureParty:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_PARTY;
        break;
    case QCamera::ExposureCandlelight:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_CANDLELIGHT;
        break;
    case QCamera::ExposureBarcode:
        sceneMode = GST_PHOTOGRAPHY_SCENE_MODE_BARCODE;
        break;
    default:
        return;
    }

    if (gst_photography_set_scene_mode(p, sceneMode))
        exposureModeChanged(mode);
#endif
}

bool QGstreamerCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (mode == QCamera::ExposureAuto)
        return true;
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera() && v4l2ManualExposureSupported && v4l2AutoExposureSupported)
        return mode == QCamera::ExposureManual;
#endif
#if QT_CONFIG(gstreamer_photography)
    if (photography())
        return true;
#endif

    return false;
}

void QGstreamerCamera::setExposureCompensation(float compensation)
{
    Q_UNUSED(compensation);
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera() && (v4l2MinExposureAdjustment != 0 || v4l2MaxExposureAdjustment != 0)) {
        int value = qBound(v4l2MinExposureAdjustment, (int)(compensation*1000), v4l2MaxExposureAdjustment);
        setV4L2Parameter(V4L2_CID_AUTO_EXPOSURE_BIAS, value);
        exposureCompensationChanged(value/1000.);
        return;
    }
#endif

#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        if (gst_photography_set_ev_compensation(p, compensation))
            exposureCompensationChanged(compensation);
    }
#endif
}

void QGstreamerCamera::setManualIsoSensitivity(int iso)
{
    Q_UNUSED(iso);
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        if (!(supportedFeatures() & QCamera::Feature::IsoSensitivity))
            return;
        setV4L2Parameter(V4L2_CID_ISO_SENSITIVITY_AUTO, iso <= 0 ? V4L2_ISO_SENSITIVITY_AUTO : V4L2_ISO_SENSITIVITY_MANUAL);
        if (iso > 0) {
            iso = qBound(minIso(), iso, maxIso());
            setV4L2Parameter(V4L2_CID_ISO_SENSITIVITY, iso);
        }
        return;
    }
#endif
#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        if (gst_photography_set_iso_speed(p, iso))
            isoSensitivityChanged(iso);
    }
#endif
}

int QGstreamerCamera::isoSensitivity() const
{
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        if (!(supportedFeatures() & QCamera::Feature::IsoSensitivity))
            return -1;
        return getV4L2Parameter(V4L2_CID_ISO_SENSITIVITY);
    }
#endif
#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        guint speed = 0;
        if (gst_photography_get_iso_speed(p, &speed))
            return speed;
    }
#endif
    return 100;
}

void QGstreamerCamera::setManualExposureTime(float secs)
{
    Q_UNUSED(secs);
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera() && v4l2ManualExposureSupported && v4l2AutoExposureSupported) {
        int exposure = qBound(v4l2MinExposure, qRound(secs*10000.), v4l2MaxExposure);
        setV4L2Parameter(V4L2_CID_EXPOSURE_ABSOLUTE, exposure);
        exposureTimeChanged(exposure/10000.);
        return;
    }
#endif

#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        if (gst_photography_set_exposure(p, guint(secs*1000000)))
            exposureTimeChanged(secs);
    }
#endif
}

float QGstreamerCamera::exposureTime() const
{
#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        return getV4L2Parameter(V4L2_CID_EXPOSURE_ABSOLUTE)/10000.;
    }
#endif
#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        guint32 exposure = 0;
        if (gst_photography_get_exposure(p, &exposure))
            return exposure/1000000.;
    }
#endif
    return -1;
}

bool QGstreamerCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (mode == QCamera::WhiteBalanceAuto)
        return true;

#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        if (v4l2AutoWhiteBalanceSupported && v4l2ColorTemperatureSupported)
            return true;
    }
#endif
#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        Q_UNUSED(p);
        switch (mode) {
        case QCamera::WhiteBalanceAuto:
        case QCamera::WhiteBalanceSunlight:
        case QCamera::WhiteBalanceCloudy:
        case QCamera::WhiteBalanceShade:
        case QCamera::WhiteBalanceSunset:
        case QCamera::WhiteBalanceTungsten:
        case QCamera::WhiteBalanceFluorescent:
            return true;
        case QCamera::WhiteBalanceManual: {
            GstPhotographyInterface *iface = GST_PHOTOGRAPHY_GET_INTERFACE(p);
            if (iface->set_color_temperature && iface->get_color_temperature)
                return true;
            break;
        }
        default:
            break;
        }
    }
#endif

    return mode == QCamera::WhiteBalanceAuto;
}

void QGstreamerCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    Q_ASSERT(isWhiteBalanceModeSupported(mode));

#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        int temperature = colorTemperatureForWhiteBalance(mode);
        int t = setV4L2ColorTemperature(temperature);
        if (t == 0)
            mode = QCamera::WhiteBalanceAuto;
        whiteBalanceModeChanged(mode);
        return;
    }
#endif

#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        GstPhotographyWhiteBalanceMode gstMode = GST_PHOTOGRAPHY_WB_MODE_AUTO;
        switch (mode) {
        case QCamera::WhiteBalanceSunlight:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_DAYLIGHT;
            break;
        case QCamera::WhiteBalanceCloudy:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_CLOUDY;
            break;
        case QCamera::WhiteBalanceShade:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_SHADE;
            break;
        case QCamera::WhiteBalanceSunset:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_SUNSET;
            break;
        case QCamera::WhiteBalanceTungsten:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_TUNGSTEN;
            break;
        case QCamera::WhiteBalanceFluorescent:
            gstMode = GST_PHOTOGRAPHY_WB_MODE_FLUORESCENT;
            break;
        case QCamera::WhiteBalanceAuto:
        default:
            break;
        }
        if (gst_photography_set_white_balance_mode(p, gstMode)) {
            whiteBalanceModeChanged(mode);
            return;
        }
    }
#endif
}

void QGstreamerCamera::setColorTemperature(int temperature)
{
    if (temperature == 0) {
        setWhiteBalanceMode(QCamera::WhiteBalanceAuto);
        return;
    }

    Q_ASSERT(isWhiteBalanceModeSupported(QCamera::WhiteBalanceManual));

#if QT_CONFIG(linux_v4l)
    if (isV4L2Camera()) {
        int t = setV4L2ColorTemperature(temperature);
        if (t)
            colorTemperatureChanged(t);
        return;
    }
#endif

#if QT_CONFIG(gstreamer_photography)
    if (auto *p = photography()) {
        GstPhotographyInterface *iface = GST_PHOTOGRAPHY_GET_INTERFACE(p);
        Q_ASSERT(iface->set_color_temperature);
        iface->set_color_temperature(p, temperature);
        return;
    }
#endif
}

#if QT_CONFIG(linux_v4l)
bool QGstreamerCamera::isV4L2Camera() const
{
    return !m_v4l2DevicePath.isEmpty();
}

void QGstreamerCamera::initV4L2Controls()
{
    v4l2AutoWhiteBalanceSupported = false;
    v4l2ColorTemperatureSupported = false;
    QCamera::Features features{};

    Q_ASSERT(!m_v4l2DevicePath.isEmpty());


    withV4L2DeviceFileDescriptor([&](int fd) {
        struct v4l2_queryctrl queryControl = {};
        queryControl.id = V4L2_CID_AUTO_WHITE_BALANCE;

        if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            v4l2AutoWhiteBalanceSupported = true;
            setV4L2Parameter(V4L2_CID_AUTO_WHITE_BALANCE, true);
        }

        queryControl = {};
        queryControl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
        if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            v4l2MinColorTemp = queryControl.minimum;
            v4l2MaxColorTemp = queryControl.maximum;
            v4l2ColorTemperatureSupported = true;
            features |= QCamera::Feature::ColorTemperature;
        }

        queryControl = {};
        queryControl.id = V4L2_CID_EXPOSURE_AUTO;
        if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            v4l2AutoExposureSupported = true;
        }

        queryControl = {};
        queryControl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
        if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            v4l2ManualExposureSupported = true;
            v4l2MinExposure = queryControl.minimum;
            v4l2MaxExposure = queryControl.maximum;
            features |= QCamera::Feature::ManualExposureTime;
        }

        queryControl = {};
        queryControl.id = V4L2_CID_AUTO_EXPOSURE_BIAS;
        if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            v4l2MinExposureAdjustment = queryControl.minimum;
            v4l2MaxExposureAdjustment = queryControl.maximum;
            features |= QCamera::Feature::ExposureCompensation;
        }

        queryControl = {};
        queryControl.id = V4L2_CID_ISO_SENSITIVITY_AUTO;
        if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
            queryControl.id = V4L2_CID_ISO_SENSITIVITY;
            if (::ioctl(fd, VIDIOC_QUERYCTRL, &queryControl) == 0) {
                features |= QCamera::Feature::IsoSensitivity;
                minIsoChanged(queryControl.minimum);
                maxIsoChanged(queryControl.minimum);
            }
        }
    });

    supportedFeaturesChanged(features);
}

int QGstreamerCamera::setV4L2ColorTemperature(int temperature)
{
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

bool QGstreamerCamera::setV4L2Parameter(quint32 id, qint32 value)
{
    return withV4L2DeviceFileDescriptor([&](int fd) {
        v4l2_control control{ id, value };
        if (::ioctl(fd, VIDIOC_S_CTRL, &control) != 0) {
            qWarning() << "Unable to set the V4L2 Parameter" << Qt::hex << id << "to" << value
                       << qt_error_string(errno);
            return false;
        }
        return true;
    });
}

int QGstreamerCamera::getV4L2Parameter(quint32 id) const
{
    return withV4L2DeviceFileDescriptor([&](int fd) {
        v4l2_control control{ id, 0 };
        if (::ioctl(fd, VIDIOC_G_CTRL, &control) != 0) {
            qWarning() << "Unable to get the V4L2 Parameter" << Qt::hex << id
                       << qt_error_string(errno);
            return 0;
        }
        return control.value;
    });
}

#endif // QT_CONFIG(linux_v4l)

QGstreamerCustomCamera::QGstreamerCustomCamera(QCamera *camera)
    : QGstreamerCameraBase{
          camera,
      },
      m_userProvidedGstElement{
          false,
      }
{
}

QGstreamerCustomCamera::QGstreamerCustomCamera(QCamera *camera, QGstElement element)
    : QGstreamerCameraBase{
          camera,
      },
      gstCamera{
          std::move(element),
      },
      m_userProvidedGstElement{
          true,
      }
{
}

void QGstreamerCustomCamera::setCamera(const QCameraDevice &device)
{
    if (m_userProvidedGstElement)
        return;

    gstCamera = QGstBin::createFromPipelineDescription(device.id(), /*name=*/nullptr,
                                                       /* ghostUnlinkedPads=*/true);
}

bool QGstreamerCustomCamera::isActive() const
{
    return m_active;
}

void QGstreamerCustomCamera::setActive(bool active)
{
    if (m_active == active)
        return;

    m_active = active;

    emit activeChanged(active);
}

QT_END_NAMESPACE
