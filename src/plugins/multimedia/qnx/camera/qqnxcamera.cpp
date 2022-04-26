/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
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
#include "qqnxcamera_p.h"
#include "qqnxcameraframebuffer_p.h"
#include "qqnxmediacapturesession_p.h"
#include "qqnxvideosink_p.h"

#include <qcameradevice.h>
#include <qmediadevices.h>

static constexpr camera_focusmode_t qnxFocusMode(QCamera::FocusMode mode)
{
    switch (mode) {
    default:
    case QCamera::FocusModeAuto:
    case QCamera::FocusModeAutoFar:
    case QCamera::FocusModeInfinity:
        return CAMERA_FOCUSMODE_CONTINUOUS_AUTO;
    case QCamera::FocusModeAutoNear:
        return CAMERA_FOCUSMODE_CONTINUOUS_MACRO;
    case QCamera::FocusModeHyperfocal:
        return CAMERA_FOCUSMODE_EDOF;
    case QCamera::FocusModeManual:
        return CAMERA_FOCUSMODE_MANUAL;
    }
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

QQnxCamera::QQnxCamera(QCamera *parent)
    : QPlatformCamera(parent)
{
    setCamera(QMediaDevices::defaultVideoInput());
}

QQnxCamera::~QQnxCamera()
{
    stop();
}

bool QQnxCamera::isActive() const
{
    return m_handle.isOpen() && m_viewfinderActive;
}

void QQnxCamera::setActive(bool active)
{
    if (active)
        start();
    else
        stop();
}

void QQnxCamera::start()
{
    if (isActive())
        return;

    updateCameraFeatures();

    if (camera_set_vf_property(m_handle.get(), CAMERA_IMGPROP_CREATEWINDOW, 0,
                CAMERA_IMGPROP_RENDERTOWINDOW, 0) != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to set camera properties");
        return;
    }

    constexpr camera_vfmode_t mode = CAMERA_VFMODE_DEFAULT;

    if (!supportedVfModes().contains(mode)) {
        qWarning("QQnxCamera: unsupported viewfinder mode");
        return;
    }

    if (camera_set_vf_mode(m_handle.get(), mode) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to configure viewfinder mode");
        return;
    }

    if (camera_start_viewfinder(m_handle.get(), viewfinderCallback,
                statusCallback, this) != CAMERA_EOK) {
        qWarning("QQnxCamera: unable to start viewfinder");
        return;
    }

    m_viewfinderActive = true;

    if (m_session)
        m_videoSink = m_session->videoSink();
}

void QQnxCamera::stop()
{
    if (!isActive())
        return;

    if (camera_stop_viewfinder(m_handle.get()) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to stop camera");

    m_viewfinderActive = false;

    m_videoSink = nullptr;
}

void QQnxCamera::setCamera(const QCameraDevice &camera)
{
    if (m_camera == camera)
        return;

    stop();

    m_handle = {};

    m_camera = camera;
    m_cameraUnit = camera_unit_t(camera.id().toUInt());

    if (!m_handle.open(m_cameraUnit, CAMERA_MODE_RO|CAMERA_MODE_PWRITE))
        qWarning("QQnxCamera: Failed to open camera (0x%x)", m_handle.lastError());
}

bool QQnxCamera::setCameraFormat(const QCameraFormat &format)
{
    if (!m_handle.isOpen())
        return false;

    const camera_error_t error = camera_set_vf_property(m_handle.get(),
            CAMERA_IMGPROP_WIDTH, format.resolution().width(),
            CAMERA_IMGPROP_HEIGHT, format.resolution().height(),
            CAMERA_IMGPROP_FRAMERATE, format.maxFrameRate());

    if (error != CAMERA_EOK) {
        qWarning("QQnxCamera: failed to set camera format");
        return false;
    }

    return true;
}

void QQnxCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    if (m_session == session)
        return;
    m_session = static_cast<QQnxMediaCaptureSession *>(session);
}

bool QQnxCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    return supportedFocusModes().contains(::qnxFocusMode(mode));
}

void QQnxCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (!isActive())
        return;

    const camera_error_t result = camera_set_focus_mode(m_handle.get(), ::qnxFocusMode(mode));

    if (result != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to set focus mode (0x%x)", result);
        return;
    }

    focusModeChanged(mode);
}

void QQnxCamera::setCustomFocusPoint(const QPointF &point)
{
    // get the size of the viewfinder
    int width = 0;
    int height = 0;
    auto result = camera_get_vf_property(m_handle.get(),
                                        CAMERA_IMGPROP_WIDTH, width,
                                        CAMERA_IMGPROP_HEIGHT, height);
    if (result != CAMERA_EOK)
        return;

    // define a 40x40 pixel focus region around the custom focus point
    camera_region_t focusRegion;
    focusRegion.left = qMax(0, static_cast<int>(point.x() * width) - 20);
    focusRegion.top = qMax(0, static_cast<int>(point.y() * height) - 20);
    focusRegion.width = 40;
    focusRegion.height = 40;

    result = camera_set_focus_regions(m_handle.get(), 1, &focusRegion);
    if (result != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to set focus region (0x%x)", result);
        return;
    }
    auto qnxMode = ::qnxFocusMode(focusMode());
    result = camera_set_focus_mode(m_handle.get(), qnxMode);
    if (result != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to set focus region (0x%x)", result);
        return;
    }
    customFocusPointChanged(point);
}

void QQnxCamera::setFocusDistance(float distance)
{
    if (!isActive() || !isFocusModeSupported(QCamera::FocusModeManual))
        return;

    const int maxDistance = maxFocusDistance();

    if (maxDistance < 0)
        return;

    const int qnxDistance = maxDistance * std::min(distance, 1.0f);

    if (camera_set_manual_focus_step(m_handle.get(), qnxDistance) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set focus distance");
}

int QQnxCamera::maxFocusDistance() const
{
    if (!isActive() || !isFocusModeSupported(QCamera::FocusModeManual))
        return -1;

    int maxstep;
    int step;

    if (camera_get_manual_focus_step(m_handle.get(), &maxstep, &step) != CAMERA_EOK) {
        qWarning("QQnxCamera: Unable to query camera focus step");
        return -1;
    }

    return maxstep;
}

void QQnxCamera::zoomTo(float factor, float)
{
    if (!isActive())
        return;

    if (maxZoom <= minZoom)
        return;
    // QNX has an integer based API. Interpolate between the levels according to the factor we get
    const float max = maxZoomFactor();
    const float min = minZoomFactor();
    if (max <= min)
        return;
    factor = qBound(min, factor, max) - min;
    uint zoom = minZoom + (uint)qRound(factor*(maxZoom - minZoom)/(max - min));

    auto error = camera_set_vf_property(m_handle.get(), CAMERA_IMGPROP_ZOOMFACTOR, zoom);
    if (error == CAMERA_EOK)
        zoomFactorChanged(factor);
}

void QQnxCamera::setExposureCompensation(float ev)
{
    if (!isActive())
        return;

    if (camera_set_ev_offset(m_handle.get(), ev) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to setup exposure compensation");
}

int QQnxCamera::isoSensitivity() const
{
    if (!isActive())
        return 0;

    unsigned int isoValue;

    if (camera_get_manual_iso(m_handle.get(), &isoValue) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to query ISO value");
        return 0;
    }

    return isoValue;
}

void QQnxCamera::setManualIsoSensitivity(int value)
{
    if (!isActive())
        return;

    const unsigned int isoValue = std::max(0, value);

    if (camera_set_manual_iso(m_handle.get(), isoValue) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set ISO value");
}

void QQnxCamera::setManualExposureTime(float seconds)
{
    if (!isActive())
        return;

    if (camera_set_manual_shutter_speed(m_handle.get(), seconds) != CAMERA_EOK)
        qWarning("QQnxCamera: Failed to set exposure time");
}

float QQnxCamera::exposureTime() const
{
    if (!isActive())
        return 0;

    double shutterSpeed;

    if (camera_get_manual_shutter_speed(m_handle.get(), &shutterSpeed) != CAMERA_EOK) {
        qWarning("QQnxCamera: Failed to get exposure time");
        return 0;
    }

    return static_cast<float>(shutterSpeed);
}

bool QQnxCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (!whiteBalanceModesChecked) {
        whiteBalanceModesChecked = true;
        unsigned numWhiteBalanceValues = 0;
        auto error = camera_get_supported_manual_white_balance_values(m_handle.get(), 0, &numWhiteBalanceValues,
                                                                      nullptr, &continuousColorTemperatureSupported);
        if (error == CAMERA_EOK) {
            manualColorTemperatureValues.resize(numWhiteBalanceValues);
            auto error = camera_get_supported_manual_white_balance_values(m_handle.get(), numWhiteBalanceValues, &numWhiteBalanceValues,
                                                                          manualColorTemperatureValues.data(),
                                                                          &continuousColorTemperatureSupported);

            minColorTemperature = 1024*1014; // large enough :)
            for (int temp : qAsConst(manualColorTemperatureValues)) {
                minColorTemperature = qMin(minColorTemperature, temp);
                maxColorTemperature = qMax(maxColorTemperature, temp);
            }
        } else {
            maxColorTemperature = 0;
        }
    }

    if (maxColorTemperature != 0)
        return true;
    return mode == QCamera::WhiteBalanceAuto;
}

void QQnxCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (mode == QCamera::WhiteBalanceAuto) {
        camera_set_whitebalance_mode(m_handle.get(), CAMERA_WHITEBALANCEMODE_AUTO);
        return;
    }
    camera_set_whitebalance_mode(m_handle.get(), CAMERA_WHITEBALANCEMODE_MANUAL);
    setColorTemperature(colorTemperatureForWhiteBalance(mode));
}

void QQnxCamera::setColorTemperature(int temperature)
{

    if (maxColorTemperature == 0)
        return;

    unsigned bestTemp = 0;
    if (!continuousColorTemperatureSupported) {
        // find the closest match
        int delta = 1024*1024;
        for (unsigned temp : qAsConst(manualColorTemperatureValues)) {
            int d = qAbs(int(temp) - temperature);
            if (d < delta) {
                bestTemp = temp;
                delta = d;
            }
        }
    } else {
        bestTemp = (unsigned)qBound(minColorTemperature, temperature, maxColorTemperature);
    }

    auto error = camera_set_manual_white_balance(m_handle.get(), bestTemp);
}

camera_handle_t QQnxCamera::handle() const
{
    return m_handle.get();
}

void QQnxCamera::updateCameraFeatures()
{
    whiteBalanceModesChecked = false;

    bool smooth;
    const camera_error_t error = camera_get_zoom_limits(m_handle.get(),
            &minZoom, &maxZoom, &smooth);

    if (error == CAMERA_EOK) {
        double level;
        camera_get_zoom_ratio_from_zoom_level(m_handle.get(), minZoom, &level);
        minimumZoomFactorChanged(level);
        camera_get_zoom_ratio_from_zoom_level(m_handle.get(), maxZoom, &level);
        maximumZoomFactorChanged(level);
    } else {
        minZoom = maxZoom = 1;
    }

    QCamera::Features features = {};

    if (camera_has_feature(m_handle.get(), CAMERA_FEATURE_REGIONFOCUS))
        features |= QCamera::Feature::CustomFocusPoint;

    minimumZoomFactorChanged(minZoom);
    maximumZoomFactorChanged(maxZoom);
    supportedFeaturesChanged(features);
}

QList<camera_vfmode_t> QQnxCamera::supportedVfModes() const
{
    return queryValues(camera_get_supported_vf_modes);
}

QList<camera_res_t> QQnxCamera::supportedVfResolutions() const
{
    return queryValues(camera_get_supported_vf_resolutions);
}

QList<camera_focusmode_t> QQnxCamera::supportedFocusModes() const
{
    return queryValues(camera_get_focus_modes);
}

template <typename T, typename U>
QList<T> QQnxCamera::queryValues(QueryFuncPtr<T,U> func) const
{
    static_assert(std::is_integral_v<U>, "Parameter U must be of integral type");

    if (!isActive())
        return {};

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
    if (!m_videoSink)
        return;

    // process the frame on this thread before locking the mutex
    auto frame = std::make_unique<QQnxCameraFrameBuffer>(buffer);

    // skip a frame if mutex is busy
    if (m_currentFrameMutex.tryLock()) {
        m_currentFrame = std::move(frame);
        m_currentFrameMutex.unlock();

        QMetaObject::invokeMethod(this, "processFrame", Qt::QueuedConnection);
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
    case CAMERA_STATUS_DISCONNECTED:
    case CAMERA_STATUS_POWERDOWN:
    case CAMERA_STATUS_VIDEOVF:
    case CAMERA_STATUS_CAPTURE_ABORTED:
    case CAMERA_STATUS_FILESIZE_WARNING:
    case CAMERA_STATUS_FOCUS_CHANGE:
    case CAMERA_STATUS_RESOURCENOTAVAIL:
    case CAMERA_STATUS_VIEWFINDER_ERROR:
    case CAMERA_STATUS_MM_ERROR:
    case CAMERA_STATUS_FILESIZE_ERROR:
    case CAMERA_STATUS_NOSPACE_ERROR:
    case CAMERA_STATUS_BUFFER_UNDERFLOW:
        Q_EMIT(status, ::statusToString(status));
        stop();
        break;
    default:
        break;
    }
}

void QQnxCamera::processFrame()
{
    QMutexLocker l(&m_currentFrameMutex);

    const QVideoFrame actualFrame(m_currentFrame.get(),
            QVideoFrameFormat(m_currentFrame->size(), m_currentFrame->pixelFormat()));

    m_videoSink->setVideoFrame(actualFrame);
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
