// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#undef QT_NO_CONTEXTLESS_CONNECT // Remove after porting connect() calls

#include "qqnxplatformcamera_p.h"
#include "qqnxcameraframebuffer_p.h"
#include "qqnxmediacapturesession_p.h"
#include "qqnxvideosink_p.h"

#include <qcameradevice.h>
#include <qmediadevices.h>

#include <private/qmediastoragelocation_p.h>
#include <private/qvideoframe_p.h>

#include <camera/camera_api.h>
#include <camera/camera_3a.h>

#include <algorithm>
#include <array>

#include <dlfcn.h>

struct FocusModeMapping
{
    QCamera::FocusMode qt;
    camera_focusmode_t qnx;
};

constexpr std::array<FocusModeMapping, 6> focusModes {{
    { QCamera::FocusModeAuto,       CAMERA_FOCUSMODE_CONTINUOUS_AUTO    },
    { QCamera::FocusModeAutoFar,    CAMERA_FOCUSMODE_CONTINUOUS_AUTO    },
    { QCamera::FocusModeInfinity,   CAMERA_FOCUSMODE_CONTINUOUS_AUTO    },
    { QCamera::FocusModeAutoNear,   CAMERA_FOCUSMODE_CONTINUOUS_MACRO   },
    { QCamera::FocusModeHyperfocal, CAMERA_FOCUSMODE_EDOF               },
    { QCamera::FocusModeManual,     CAMERA_FOCUSMODE_MANUAL             },
}};

template <typename Mapping, typename From, typename To, size_t N>
static constexpr To convert(const std::array<Mapping, N> &mapping,
        From Mapping::* from, To Mapping::* to, From value, To defaultValue)
{
    for (const Mapping &m : mapping) {
        const auto fromValue = m.*from;
        const auto toValue = m.*to;

        if (value == fromValue)
            return toValue;
    }

    return defaultValue;
}

static constexpr camera_focusmode_t qnxFocusMode(QCamera::FocusMode mode)
{
    return convert(focusModes, &FocusModeMapping::qt,
            &FocusModeMapping::qnx, mode, CAMERA_FOCUSMODE_CONTINUOUS_AUTO);
}

static constexpr QCamera::FocusMode qtFocusMode(camera_focusmode_t mode)
{
    return convert(focusModes, &FocusModeMapping::qnx,
            &FocusModeMapping::qt, mode, QCamera::FocusModeAuto);
}

QT_BEGIN_NAMESPACE

QQnxPlatformCamera::QQnxPlatformCamera(QCamera *parent)
    : QPlatformCamera(parent)
{
    if (parent)
        setCamera(parent->cameraDevice());
    else
        setCamera(QMediaDevices::defaultVideoInput());
}

QQnxPlatformCamera::~QQnxPlatformCamera()
{
    stop();
}

bool QQnxPlatformCamera::isActive() const
{
    return m_qnxCamera && m_qnxCamera->isActive();
}

void QQnxPlatformCamera::setActive(bool active)
{
    if (active)
        start();
    else
        stop();
}

void QQnxPlatformCamera::start()
{
    if (!m_qnxCamera || isActive())
        return;

    if (m_session)
        m_videoSink = m_session->videoSink();

    m_qnxCamera->start();

    Q_EMIT activeChanged(true);
}

void QQnxPlatformCamera::stop()
{
    if (!m_qnxCamera)
        return;

    m_qnxCamera->stop();

    m_videoSink = nullptr;

    Q_EMIT activeChanged(false);
}

void QQnxPlatformCamera::setCamera(const QCameraDevice &camera)
{
    if (m_cameraDevice == camera)
        return;

    const auto cameraUnit = static_cast<camera_unit_t>(camera.id().toUInt());

    m_qnxCamera = std::make_unique<QQnxCamera>(cameraUnit);

    connect(m_qnxCamera.get(), &QQnxCamera::focusModeChanged,
            [this](camera_focusmode_t mode) { Q_EMIT focusModeChanged(qtFocusMode(mode)); });
    connect(m_qnxCamera.get(), &QQnxCamera::customFocusPointChanged,
            this, &QQnxPlatformCamera::customFocusPointChanged);
    connect(m_qnxCamera.get(), &QQnxCamera::frameAvailable,
            this, &QQnxPlatformCamera::onFrameAvailable, Qt::QueuedConnection);

    m_cameraDevice = camera;

    updateCameraFeatures();
}

bool QQnxPlatformCamera::setCameraFormat(const QCameraFormat &format)
{
    const QSize resolution = format.resolution();

    if (resolution.isEmpty()) {
        qWarning("QQnxPlatformCamera: invalid resolution requested");
        return false;
    }

    return m_qnxCamera->setCameraFormat(resolution.width(),
            resolution.height(), format.maxFrameRate());
}

void QQnxPlatformCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    if (m_session == session)
        return;

    m_session = static_cast<QQnxMediaCaptureSession *>(session);
}

bool QQnxPlatformCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    if (!m_qnxCamera)
        return false;

    return m_qnxCamera->supportedFocusModes().contains(::qnxFocusMode(mode));
}

void QQnxPlatformCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (!m_qnxCamera)
        return;

    m_qnxCamera->setFocusMode(::qnxFocusMode(mode));
}

void QQnxPlatformCamera::setCustomFocusPoint(const QPointF &point)
{
    if (!m_qnxCamera)
        return;

    m_qnxCamera->setCustomFocusPoint(point);
}

void QQnxPlatformCamera::setFocusDistance(float distance)
{
    if (!m_qnxCamera)
        return;

    const int maxDistance = m_qnxCamera->maxFocusStep();

    if (maxDistance < 0)
        return;

    const int qnxDistance = maxDistance * std::min(distance, 1.0f);

    m_qnxCamera->setManualFocusStep(qnxDistance);
}

void QQnxPlatformCamera::zoomTo(float factor, float)
{
    if (!m_qnxCamera)
        return;

    const uint32_t minZoom = m_qnxCamera->minimumZoomLevel();
    const uint32_t maxZoom = m_qnxCamera->maximumZoomLevel();

    if (maxZoom <= minZoom)
        return;

    // QNX has an integer based API. Interpolate between the levels according to the factor we get
    const float max = maxZoomFactor();
    const float min = minZoomFactor();

    if (max <= min)
        return;

    factor = qBound(min, factor, max) - min;

    const uint32_t zoom = minZoom
        + static_cast<uint32_t>(qRound(factor*(maxZoom - minZoom)/(max - min)));

    if (m_qnxCamera->setZoomFactor(zoom))
        zoomFactorChanged(factor);
}

void QQnxPlatformCamera::setExposureCompensation(float ev)
{
    if (!m_qnxCamera)
        return;

    m_qnxCamera->setEvOffset(ev);
}

int QQnxPlatformCamera::isoSensitivity() const
{
    if (!m_qnxCamera)
        return 0;

    return m_qnxCamera->manualIsoSensitivity();
}

void QQnxPlatformCamera::setManualIsoSensitivity(int value)
{
    if (!m_qnxCamera)
        return;

    const uint32_t isoValue = std::max(0, value);

    m_qnxCamera->setManualIsoSensitivity(isoValue);
}

void QQnxPlatformCamera::setManualExposureTime(float seconds)
{
    if (!m_qnxCamera)
        return;

    m_qnxCamera->setManualExposureTime(seconds);
}

float QQnxPlatformCamera::exposureTime() const
{
    if (!m_qnxCamera)
        return 0.0;

    return static_cast<float>(m_qnxCamera->manualExposureTime());
}

bool QQnxPlatformCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (m_maxColorTemperature != 0)
        return true;

    return mode == QCamera::WhiteBalanceAuto;
}

void QQnxPlatformCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (!m_qnxCamera)
        return;

    if (mode == QCamera::WhiteBalanceAuto) {
        m_qnxCamera->setWhiteBalanceMode(CAMERA_WHITEBALANCEMODE_AUTO);
    } else {
        m_qnxCamera->setWhiteBalanceMode(CAMERA_WHITEBALANCEMODE_MANUAL);
        setColorTemperature(colorTemperatureForWhiteBalance(mode));
    }
}

void QQnxPlatformCamera::setColorTemperature(int temperature)
{
    if (!m_qnxCamera)
        return;

    const auto normalizedTemp = std::clamp<uint32_t>(std::max(0, temperature),
            m_minColorTemperature, m_maxColorTemperature);

    if (m_qnxCamera->hasContinuousWhiteBalanceValues()) {
        m_qnxCamera->setManualWhiteBalance(normalizedTemp);
    } else {
        uint32_t delta = std::numeric_limits<uint32_t>::max();
        uint32_t closestTemp = 0;

        for (uint32_t value : m_qnxCamera->supportedWhiteBalanceValues()) {
            const auto &[min, max] = std::minmax(value, normalizedTemp);
            const uint32_t currentDelta = max - min;

            if (currentDelta < delta) {
                closestTemp = value;
                delta = currentDelta;
            }
        }

        m_qnxCamera->setManualWhiteBalance(closestTemp);
    }
}

bool QQnxPlatformCamera::startVideoRecording()
{
    if (!m_qnxCamera) {
        qWarning("QQnxPlatformCamera: cannot start video recording - no no camera assigned");
        return false;
    }

    if (!isVideoEncodingSupported()) {
        qWarning("QQnxPlatformCamera: cannot start video recording - not supported");
        return false;
    }

    if (!m_qnxCamera->isActive()) {
        qWarning("QQnxPlatformCamera: cannot start video recording - camera not started");
        return false;
    }

    const QString container = m_encoderSettings.preferredSuffix();
    const QString location = QMediaStorageLocation::generateFileName(m_outputUrl.toLocalFile(),
            QStandardPaths::MoviesLocation, container);

#if 0
    {
        static void *libScreen = nullptr;

        if (!libScreen)
            libScreen = dlopen("/usr/lib/libscreen.so.1", RTLD_GLOBAL);
    }
#endif

    qDebug() << "Recording to" << location;
    return m_qnxCamera->startVideoRecording(location);
}

void QQnxPlatformCamera::requestVideoFrame(VideoFrameCallback cb)
{
    m_videoFrameRequests.emplace_back(std::move(cb));
}

bool QQnxPlatformCamera::isVideoEncodingSupported() const
{
    return m_qnxCamera && m_qnxCamera->hasFeature(CAMERA_FEATURE_VIDEO);
}

void QQnxPlatformCamera::setOutputUrl(const QUrl &url)
{
    m_outputUrl = url;
}

void QQnxPlatformCamera::setMediaEncoderSettings(const QMediaEncoderSettings &settings)
{
    m_encoderSettings = settings;
}

void QQnxPlatformCamera::updateCameraFeatures()
{
    if (!m_qnxCamera)
        return;

    QCamera::Features features = {};

    if (m_qnxCamera->hasFeature(CAMERA_FEATURE_REGIONFOCUS))
        features |= QCamera::Feature::CustomFocusPoint;

    supportedFeaturesChanged(features);

    minimumZoomFactorChanged(m_qnxCamera->minimumZoomLevel());
    maximumZoomFactorChanged(m_qnxCamera->maximumZoomLevel());

    const QList<uint32_t> wbValues = m_qnxCamera->supportedWhiteBalanceValues();

    if (wbValues.isEmpty()) {
        m_minColorTemperature = m_maxColorTemperature = 0;
    } else {
        const auto &[minTemp, maxTemp] = std::minmax_element(wbValues.begin(), wbValues.end());

        m_minColorTemperature = *minTemp;
        m_maxColorTemperature = *maxTemp;
    }
}

void QQnxPlatformCamera::onFrameAvailable()
{
    if (!m_videoSink)
        return;

    std::unique_ptr<QQnxCameraFrameBuffer> currentFrameBuffer = m_qnxCamera->takeCurrentFrame();

    if (!currentFrameBuffer)
        return;

    QVideoFrameFormat format(currentFrameBuffer->size(), currentFrameBuffer->pixelFormat());
    const QVideoFrame actualFrame =
            QVideoFramePrivate::createFrame(std::move(currentFrameBuffer), std::move(format));

    m_videoSink->setVideoFrame(actualFrame);

    if (!m_videoFrameRequests.empty()) {
        VideoFrameCallback cb = std::move(m_videoFrameRequests.front());
        m_videoFrameRequests.pop_front();
        cb(actualFrame);
    }
}

QT_END_NAMESPACE

#include "moc_qqnxplatformcamera_p.cpp"
