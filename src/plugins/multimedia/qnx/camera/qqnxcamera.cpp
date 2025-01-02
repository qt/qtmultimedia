// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qqnxcamera_p.h"
#include "qqnxcameraframebuffer_p.h"
#include "qqnxmediacapturesession_p.h"
#include "qqnxvideosink_p.h"

#include <qcameradevice.h>
#include <qmediadevices.h>

#include <private/qmediastoragelocation_p.h>

QDebug &operator<<(QDebug &d, const QQnxCamera::VideoFormat &f)
{
    d << "VideoFormat - width=" << f.width
      << "height=" << f.height
      << "rotation=" << f.rotation
      << "frameRate=" << f.frameRate
      << "frameType=" << f.frameType;

    return d;
}

static QString statusToString(camera_devstatus_t status)
{
    switch (status) {
    case CAMERA_STATUS_DISCONNECTED:
        return QStringLiteral("No user is connected to the camera");
    case CAMERA_STATUS_POWERDOWN:
        return QStringLiteral("Power down");
    case CAMERA_STATUS_VIDEOVF:
        return QStringLiteral("The video viewfinder has started");
    case CAMERA_STATUS_CAPTURE_ABORTED:
        return QStringLiteral("The capture of a still image failed and was aborted");
    case CAMERA_STATUS_FILESIZE_WARNING:
        return QStringLiteral("Time-remaining threshold has been exceeded");
    case CAMERA_STATUS_FOCUS_CHANGE:
        return QStringLiteral("The focus has changed on the camera");
    case CAMERA_STATUS_RESOURCENOTAVAIL:
        return QStringLiteral("The camera is about to free resources");
    case CAMERA_STATUS_VIEWFINDER_ERROR:
        return QStringLiteral(" An unexpected error was encountered while the "
                "viewfinder was active");
    case CAMERA_STATUS_MM_ERROR:
        return QStringLiteral("The recording has stopped due to a memory error or multimedia "
                "framework error");
    case CAMERA_STATUS_FILESIZE_ERROR:
        return QStringLiteral("A file has exceeded the maximum size.");
    case CAMERA_STATUS_NOSPACE_ERROR:
        return QStringLiteral("Not enough disk space");
    case CAMERA_STATUS_BUFFER_UNDERFLOW:
        return QStringLiteral("The viewfinder is out of buffers");
    default:
        break;
    }

    return {};
}

QT_BEGIN_NAMESPACE

QQnxCamera::QQnxCamera(camera_unit_t unit, QObject *parent)
    : QObject(parent)
    , m_cameraUnit(unit)
{
    if (!m_handle.open(m_cameraUnit, CAMERA_MODE_RW))
        qWarning("QQnxCamera: Failed to open camera (0x%x)", m_handle.lastError());

    if (camera_set_vf_mode(m_handle.get(), CAMERA_VFMODE_VIDEO) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to configure viewfinder mode");
        return;
    }

    if (camera_set_vf_property(m_handle.get(), CAMERA_IMGPROP_CREATEWINDOW, 0,
                CAMERA_IMGPROP_RENDERTOWINDOW, 0) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to set camera properties");
        return;
    }

    updateZoomLimits();
    updateSupportedWhiteBalanceValues();

    m_valid = true;
}

QQnxCamera::~QQnxCamera()
{
    stop();
}

camera_unit_t QQnxCamera::unit() const
{
    return m_cameraUnit;
}

QString QQnxCamera::name() const
{
    char name[CAMERA_LOCATION_NAMELEN];

    if (camera_get_location_property(m_cameraUnit,
                CAMERA_LOCATION_NAME, &name, CAMERA_LOCATION_END) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to obtain camera name");
        return {};
    }

    return QString::fromUtf8(name);
}

bool QQnxCamera::isValid() const
{
    return m_valid;
}

bool QQnxCamera::isActive() const
{
    return m_handle.isOpen() && m_viewfinderActive;
}

void QQnxCamera::start()
{
    if (isActive())
        return;

    if (camera_start_viewfinder(m_handle.get(), viewfinderCallback,
                statusCallback, this) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to start viewfinder");
        return;
    }

    m_viewfinderActive = true;
}

void QQnxCamera::stop()
{
    if (!isActive())
        return;

    if (m_recordingVideo)
        stopVideoRecording();

    if (camera_stop_viewfinder(m_handle.get()) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to stop camera");

    m_viewfinderActive = false;
}

bool QQnxCamera::setCameraFormat(uint32_t width, uint32_t height, double frameRate)
{
    if (!m_handle.isOpen())
        return false;

    const camera_error_t error = camera_set_vf_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, width,
            CAMERA_IMGPROP_HEIGHT, height,
            CAMERA_IMGPROP_FRAMERATE, frameRate);

    if (error != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to set camera format");
        return false;
    }

    return true;
}

bool QQnxCamera::isFocusModeSupported(camera_focusmode_t mode) const
{
    return supportedFocusModes().contains(mode);
}

bool QQnxCamera::setFocusMode(camera_focusmode_t mode)
{
    if (!isActive())
        return false;

    const camera_error_t result = camera_set_focus_mode(m_handle.get(), mode);

    if (result != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to set focus mode (0x%x)", result);
        return false;
    }

    focusModeChanged(mode);

    return true;
}

camera_focusmode_t QQnxCamera::focusMode() const
{
    if (!isActive())
        return CAMERA_FOCUSMODE_OFF;

    camera_focusmode_t mode;

    const camera_error_t result = camera_get_focus_mode(m_handle.get(), &mode);

    if (result != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to set focus mode (0x%x)", result);
        return CAMERA_FOCUSMODE_OFF;
    }

    return mode;
}

QQnxCamera::VideoFormat QQnxCamera::vfFormat() const
{
    VideoFormat f = {};

    if (camera_get_vf_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, &f.width,
            CAMERA_IMGPROP_HEIGHT, &f.height,
            CAMERA_IMGPROP_ROTATION, &f.rotation,
            CAMERA_IMGPROP_FRAMERATE, &f.frameRate,
            CAMERA_IMGPROP_FORMAT, &f.frameType) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to query video finder frameType");
    }

    return f;
}

void QQnxCamera::setVfFormat(const VideoFormat &f)
{
    const bool active = isActive();

    if (active)
        stop();

    if (camera_set_vf_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, f.width,
            CAMERA_IMGPROP_HEIGHT, f.height,
            CAMERA_IMGPROP_ROTATION, f.rotation,
            CAMERA_IMGPROP_FRAMERATE, f.frameRate,
            CAMERA_IMGPROP_FORMAT, f.frameType) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to set video finder frameType");
    }

    if (active)
        start();
}

QQnxCamera::VideoFormat QQnxCamera::recordingFormat() const
{
    VideoFormat f = {};

    if (camera_get_video_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, &f.width,
            CAMERA_IMGPROP_HEIGHT, &f.height,
            CAMERA_IMGPROP_ROTATION, &f.rotation,
            CAMERA_IMGPROP_FRAMERATE, &f.frameRate,
            CAMERA_IMGPROP_FORMAT, &f.frameType) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to query recording frameType");
    }

    return f;
}

void QQnxCamera::setRecordingFormat(const VideoFormat &f)
{
    if (camera_set_video_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, f.width,
            CAMERA_IMGPROP_HEIGHT, f.height,
            CAMERA_IMGPROP_ROTATION, f.rotation,
            CAMERA_IMGPROP_FRAMERATE, f.frameRate,
            CAMERA_IMGPROP_FORMAT, f.frameType) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to set recording frameType");
    }
}

void QQnxCamera::setCustomFocusPoint(const QPointF &point)
{
    const QSize vfSize = viewFinderSize();

    if (vfSize.isEmpty())
        return;

    const auto toUint32 = [](double value) {
        return static_cast<uint32_t>(std::max(0.0, value));
    };

    // define a 40x40 pixel focus region around the custom focus point
    constexpr int pixelSize = 40;

    const auto left = toUint32(point.x() * vfSize.width() - pixelSize / 2);
    const auto top = toUint32(point.y() * vfSize.height() - pixelSize / 2);

    camera_region_t focusRegion {
        .left = left,
        .top = top,
        .width = pixelSize,
        .height = pixelSize,
        .extra = 0
    };

    if (camera_set_focus_regions(m_handle.get(), 1, &focusRegion) != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to set focus region");
        return;
    }

    if (setFocusMode(focusMode()))
        customFocusPointChanged(point);
}

void QQnxCamera::setManualFocusStep(int step)
{
    if (!isActive()) {
        qWarning("QQnxCamera: Failed to set focus distance - view finder not active");
        return;
    }

    if (!isFocusModeSupported(CAMERA_FOCUSMODE_MANUAL)) {
        qWarning("QQnxCamera: Failed to set focus distance - manual focus mode not supported");
        return;
    }

    if (camera_set_manual_focus_step(m_handle.get(), step) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set focus distance");
}

int QQnxCamera::manualFocusStep() const
{
    return focusStep().step;
}

int QQnxCamera::maxFocusStep() const
{
    return focusStep().maxStep;
}

QQnxCamera::FocusStep QQnxCamera::focusStep() const
{
    constexpr FocusStep invalidStep { -1, -1 };

    if (!isActive()) {
        qWarning("QQnxCamera: Failed to query max focus distance - view finder not active");
        return invalidStep;
    }

    if (!isFocusModeSupported(CAMERA_FOCUSMODE_MANUAL)) {
        qWarning("QQnxCamera: Failed to query max focus distance - "
                "manual focus mode not supported");
        return invalidStep;
    }

    FocusStep focusStep;

    if (camera_get_manual_focus_step(m_handle.get(),
                &focusStep.maxStep, &focusStep.step) != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to query camera focus step");
        return invalidStep;
    }

    return focusStep;
}


QSize QQnxCamera::viewFinderSize() const
{
    // get the size of the viewfinder
    int width = 0;
    int height = 0;

    if (camera_get_vf_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, width,
            CAMERA_IMGPROP_HEIGHT, height) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to query view finder size");
        return {};
    }

    return { width, height };
}

uint32_t QQnxCamera::minimumZoomLevel() const
{
    return m_minZoom;
}

uint32_t QQnxCamera::maximumZoomLevel() const
{
    return m_maxZoom;
}

bool QQnxCamera::isSmoothZoom() const
{
    return m_smoothZoom;
}

double QQnxCamera::zoomRatio(uint32_t zoomLevel) const
{
    double ratio;

    if (camera_get_zoom_ratio_from_zoom_level(m_handle.get(), zoomLevel, &ratio) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to query zoom ratio from zoom level");
        return 0.0;
    }

    return ratio;
}

bool QQnxCamera::setZoomFactor(uint32_t factor)
{
    if (camera_set_vf_property(m_handle.get(), CAMERA_IMGPROP_ZOOMFACTOR, factor) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to set zoom factor");
        return false;
    }

    return true;
}

void QQnxCamera::setEvOffset(float ev)
{
    if (!isActive())
        return;

    if (camera_set_ev_offset(m_handle.get(), ev) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set up exposure compensation");
}

uint32_t QQnxCamera::manualIsoSensitivity() const
{
    if (!isActive())
        return 0;

    uint32_t isoValue;

    if (camera_get_manual_iso(m_handle.get(), &isoValue) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to query ISO value");
        return 0;
    }

    return isoValue;
}

void QQnxCamera::setManualIsoSensitivity(uint32_t value)
{
    if (!isActive())
        return;

    if (camera_set_manual_iso(m_handle.get(), value) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set ISO value");
}

void QQnxCamera::setManualExposureTime(double seconds)
{
    if (!isActive())
        return;

    if (camera_set_manual_shutter_speed(m_handle.get(), seconds) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set exposure time");
}

double QQnxCamera::manualExposureTime() const
{
    if (!isActive())
        return 0.0;

    double shutterSpeed;

    if (camera_get_manual_shutter_speed(m_handle.get(), &shutterSpeed) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to get exposure time");
        return 0.0;
    }

    return shutterSpeed;
}

bool QQnxCamera::hasFeature(camera_feature_t feature) const
{
    return camera_has_feature(m_handle.get(), feature);
}

void QQnxCamera::setWhiteBalanceMode(camera_whitebalancemode_t mode)
{
    if (!isActive())
        return;

    if (camera_set_whitebalance_mode(m_handle.get(), mode) != CAMERA_EOK)
        qWarning("QQnxCamera: failed to set whitebalance mode");
}

camera_whitebalancemode_t QQnxCamera::whiteBalanceMode() const
{
    if (!isActive())
        return CAMERA_WHITEBALANCEMODE_OFF;

    camera_whitebalancemode_t mode;

    if (camera_get_whitebalance_mode(m_handle.get(), &mode) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to get white balance mode");
        return CAMERA_WHITEBALANCEMODE_OFF;
    }

    return mode;
}

void QQnxCamera::setManualWhiteBalance(uint32_t value)
{
    if (!isActive())
        return;

    if (camera_set_manual_white_balance(m_handle.get(), value) != CAMERA_EOK)
        qWarning("QQnxCamera: failed to set manual white balance");
}

uint32_t QQnxCamera::manualWhiteBalance() const
{
    if (!isActive())
        return 0;

    uint32_t value;

    if (camera_get_manual_white_balance(m_handle.get(), &value) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to get manual white balance");
        return 0;
    }

    return value;
}

bool QQnxCamera::startVideoRecording(const QString &filename)
{
    // when preview is video, we must ensure that the recording properties
    // match the view finder properties
    if (hasFeature(CAMERA_FEATURE_PREVIEWISVIDEO)) {
        VideoFormat newFormat = vfFormat();

        const QList<camera_frametype_t> recordingTypes = supportedRecordingFrameTypes();

        // find a suitable matching frame type in case the current view finder
        // frametype is not supported
        if (newFormat.frameType != recordingFormat().frameType
                && !recordingTypes.contains(newFormat.frameType)) {

            bool found = false;

            for (const camera_frametype_t type : supportedVfFrameTypes()) {
                if (recordingTypes.contains(type)) {
                    newFormat.frameType = type;
                    found = true;
                    break;
                }
            }

            if (found) {
                m_originalVfFormat = vfFormat();

                // reconfigure and restart the view finder
                setVfFormat(newFormat);
            } else {
                qWarning("QQnxCamera: failed to find suitable frame type for recording - aborting");
                return false;
            }
        }

        setRecordingFormat(newFormat);
    }

    if (camera_start_video(m_handle.get(), qPrintable(filename),
                nullptr, nullptr, nullptr) == CAMERA_EOK) {
        m_recordingVideo = true;
    } else {
        qWarning("QQnxCamera: failed to start video encoding");
    }

    return m_recordingVideo;
}

void QQnxCamera::stopVideoRecording()
{
    m_recordingVideo = false;

    if (camera_stop_video(m_handle.get()) != CAMERA_EOK)
        qWarning("QQnxCamera: error when stopping video recording");

    // restore original vf format
    if (m_originalVfFormat) {
        setVfFormat(*m_originalVfFormat);
        m_originalVfFormat.reset();
    }
}

bool QQnxCamera::isVideoEncodingSupported() const
{
    if (!isActive())
        return false;

    return camera_has_feature(m_handle.get(), CAMERA_FEATURE_VIDEO);
}

camera_handle_t QQnxCamera::handle() const
{
    return m_handle.get();
}

void QQnxCamera::updateZoomLimits()
{
    bool smooth;

    if (camera_get_zoom_limits(m_handle.get(), &m_minZoom, &m_maxZoom, &smooth) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to update zoom limits - using default values");
        m_minZoom = m_maxZoom = 0;
    }
}

void QQnxCamera::updateSupportedWhiteBalanceValues()
{
    uint32_t numSupported = 0;

    const camera_error_t result = camera_get_supported_manual_white_balance_values(
            m_handle.get(), 0, &numSupported, nullptr, &m_continuousWhiteBalanceValues);

    if (result != CAMERA_EOK) {
        if (result == CAMERA_EOPNOTSUPP)
            qWarning("QQnxCamera: white balance not supported");
        else
            qWarning("QQnxCamera: unable to query manual white balance value count");

        m_supportedWhiteBalanceValues.clear();

        return;
    }

    m_supportedWhiteBalanceValues.resize(numSupported);

    if (camera_get_supported_manual_white_balance_values(m_handle.get(),
                m_supportedWhiteBalanceValues.size(),
                &numSupported,
                m_supportedWhiteBalanceValues.data(),
                &m_continuousWhiteBalanceValues) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to query manual white balance values");

        m_supportedWhiteBalanceValues.clear();
    }
}

QList<camera_vfmode_t> QQnxCamera::supportedVfModes() const
{
    return queryValues(camera_get_supported_vf_modes);
}

QList<camera_res_t> QQnxCamera::supportedVfResolutions() const
{
    return queryValues(camera_get_supported_vf_resolutions);
}

QList<camera_frametype_t> QQnxCamera::supportedVfFrameTypes() const
{
    return queryValues(camera_get_supported_vf_frame_types);
}

QList<camera_focusmode_t> QQnxCamera::supportedFocusModes() const
{
    return queryValues(camera_get_focus_modes);
}

QList<double> QQnxCamera::specifiedVfFrameRates(camera_frametype_t frameType,
        camera_res_t resolution) const
{
    uint32_t numSupported = 0;

    if (camera_get_specified_vf_framerates(m_handle.get(), frameType, resolution,
                0, &numSupported, nullptr, nullptr) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to query specified framerates count");
        return {};
    }

    QList<double> values(numSupported);

    if (camera_get_specified_vf_framerates(m_handle.get(), frameType, resolution,
                values.size(), &numSupported, values.data(), nullptr) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to query specified framerates values");
        return {};
    }

    return values;
}

QList<camera_frametype_t> QQnxCamera::supportedRecordingFrameTypes() const
{
    return queryValues(camera_get_video_frame_types);
}

QList<uint32_t> QQnxCamera::supportedWhiteBalanceValues() const
{
    return m_supportedWhiteBalanceValues;
}

bool QQnxCamera::hasContinuousWhiteBalanceValues() const
{
    return m_continuousWhiteBalanceValues;
}

QList<camera_unit_t> QQnxCamera::supportedUnits()
{
    unsigned int numSupported = 0;

    if (camera_get_supported_cameras(0, &numSupported, nullptr) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to query supported camera unit count");
        return {};
    }

    QList<camera_unit_t> cameraUnits(numSupported);

    if (camera_get_supported_cameras(cameraUnits.size(), &numSupported,
                cameraUnits.data()) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to enumerate supported camera units");
        return {};
    }

    return cameraUnits;
}

template <typename T, typename U>
QList<T> QQnxCamera::queryValues(QueryFuncPtr<T,U> func) const
{
    static_assert(std::is_integral_v<U>, "Parameter U must be of integral type");

    U numSupported = 0;

    if (func(m_handle.get(), 0, &numSupported, nullptr) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to query camera value count");
        return {};
    }

    QList<T> values(numSupported);

    if (func(m_handle.get(), values.size(), &numSupported, values.data()) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to query camera values");
        return {};
    }

    return values;
}

void QQnxCamera::handleVfBuffer(camera_buffer_t *buffer)
{
    // process the frame on this thread before locking the mutex
    auto frame = std::make_unique<QQnxCameraFrameBuffer>(buffer);

    // skip a frame if mutex is busy
    if (m_currentFrameMutex.tryLock()) {
        m_currentFrame = std::move(frame);
        m_currentFrameMutex.unlock();

        Q_EMIT frameAvailable();
    }
}

void QQnxCamera::handleVfStatus(camera_devstatus_t status, uint16_t extraData)
{
    QMetaObject::invokeMethod(this, "handleStatusChange", Qt::QueuedConnection,
            Q_ARG(camera_devstatus_t, status),
            Q_ARG(uint16_t, extraData));
}

void QQnxCamera::handleStatusChange(camera_devstatus_t status, uint16_t extraData)
{
    Q_UNUSED(extraData);

    switch (status) {
    case CAMERA_STATUS_BUFFER_UNDERFLOW:
    case CAMERA_STATUS_CAPTURECOMPLETE:
    case CAMERA_STATUS_CAPTURE_ABORTED:
    case CAMERA_STATUS_CONNECTED:
    case CAMERA_STATUS_DISCONNECTED:
    case CAMERA_STATUS_FILESIZE_ERROR:
    case CAMERA_STATUS_FILESIZE_LIMIT_WARNING:
    case CAMERA_STATUS_FILESIZE_WARNING:
    case CAMERA_STATUS_FLASH_LEVEL_CHANGE:
    case CAMERA_STATUS_FOCUS_CHANGE:
    case CAMERA_STATUS_FRAME_DROPPED:
    case CAMERA_STATUS_LOWLIGHT:
    case CAMERA_STATUS_MM_ERROR:
    case CAMERA_STATUS_NOSPACE_ERROR:
    case CAMERA_STATUS_PHOTOVF:
    case CAMERA_STATUS_POWERDOWN:
    case CAMERA_STATUS_POWERUP:
    case CAMERA_STATUS_RESOURCENOTAVAIL:
    case CAMERA_STATUS_UNKNOWN:
    case CAMERA_STATUS_VIDEOLIGHT_CHANGE:
    case CAMERA_STATUS_VIDEOLIGHT_LEVEL_CHANGE:
    case CAMERA_STATUS_VIDEOVF:
    case CAMERA_STATUS_VIDEO_PAUSE:
    case CAMERA_STATUS_VIDEO_RESUME:
    case CAMERA_STATUS_VIEWFINDER_ACTIVE:
    case CAMERA_STATUS_VIEWFINDER_ERROR:
    case CAMERA_STATUS_VIEWFINDER_FREEZE:
    case CAMERA_STATUS_VIEWFINDER_SUSPEND:
    case CAMERA_STATUS_VIEWFINDER_UNFREEZE:
    case CAMERA_STATUS_VIEWFINDER_UNSUSPEND:
        qDebug() << "QQnxCamera:" << ::statusToString(status);
        break;
    }
}

std::unique_ptr<QQnxCameraFrameBuffer> QQnxCamera::takeCurrentFrame()
{
    QMutexLocker l(&m_currentFrameMutex);

    return std::move(m_currentFrame);
}

void QQnxCamera::viewfinderCallback(camera_handle_t handle, camera_buffer_t *buffer, void *arg)
{
    Q_UNUSED(handle);

    auto *camera = static_cast<QQnxCamera*>(arg);
    camera->handleVfBuffer(buffer);
}

void QQnxCamera::statusCallback(camera_handle_t handle, camera_devstatus_t status,
        uint16_t extraData, void *arg)
{
    Q_UNUSED(handle);

    auto *camera = static_cast<QQnxCamera*>(arg);
    camera->handleVfStatus(status, extraData);
}

QT_END_NAMESPACE

#include "moc_qqnxcamera_p.cpp"
