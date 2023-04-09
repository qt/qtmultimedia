// Copyright (C) 2022 The Qt Company Ltd and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwasmcamera_p.h"
#include "qmediadevices.h"
#include <qcameradevice.h>
#include "private/qabstractvideobuffer_p.h"
#include "private/qplatformvideosink_p.h"
#include <private/qmemoryvideobuffer_p.h>
#include <private/qvideotexturehelper_p.h>

#include "qwasmmediacapturesession_p.h"
#include <common/qwasmvideooutput_p.h>

#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>
#include <QUuid>
#include <QTimer>

#include <private/qstdweb_p.h>

Q_LOGGING_CATEGORY(qWasmCamera, "qt.multimedia.wasm.camera")

QWasmCamera::QWasmCamera(QCamera *camera)
    : QPlatformCamera(camera), m_cameraOutput(new QWasmVideoOutput)
{
}

QWasmCamera::~QWasmCamera() = default;

bool QWasmCamera::isActive() const
{
    return m_cameraActive;
}

void QWasmCamera::setActive(bool active)
{
    if (!m_CaptureSession) {
        emit error(QCamera::CameraError, QStringLiteral("video surface error"));
        return;
    }

    m_cameraOutput->setSurface(m_CaptureSession->videoSink());

    m_cameraActive = active;

    if (m_cameraActive)
        m_cameraOutput->start();
    else
        m_cameraOutput->pause();

    updateCameraFeatures();
    emit activeChanged(active);
}

void QWasmCamera::setCamera(const QCameraDevice &camera)
{
    m_cameraOutput->setVideoMode(QWasmVideoOutput::Camera);

    constexpr QSize initialSize(0, 0);
    constexpr QRect initialRect(QPoint(0, 0), initialSize);
    m_cameraOutput->createVideoElement(camera.id().toStdString()); // videoElementId
    m_cameraOutput->createOffscreenElement(initialSize);
    m_cameraOutput->updateVideoElementGeometry(initialRect);

    const auto cameras = QMediaDevices::videoInputs();
    if (std::find(cameras.begin(), cameras.end(), camera) != cameras.end()) {
        m_cameraDev = camera;
        createCamera(m_cameraDev);
        return;
    }

    if (cameras.count() > 0) {
        m_cameraDev = camera;
        createCamera(m_cameraDev);
    } else {
        emit error(QCamera::CameraError, QStringLiteral("Failed to find a camera"));
    }
}

bool QWasmCamera::setCameraFormat(const QCameraFormat &format)
{
    m_cameraFormat = format;

    return true;
}

void QWasmCamera::setCaptureSession(QPlatformMediaCaptureSession *session)
{
    QWasmMediaCaptureSession *captureSession = static_cast<QWasmMediaCaptureSession *>(session);
    if (m_CaptureSession == captureSession)
        return;

    m_CaptureSession = captureSession;
}

void QWasmCamera::setFocusMode(QCamera::FocusMode mode)
{
    if (!isFocusModeSupported(mode))
        return;

    static constexpr std::string_view focusModeString = "focusMode";
    if (mode == QCamera::FocusModeManual)
        m_cameraOutput->setDeviceSetting(focusModeString.data(), emscripten::val("manual"));
    if (mode == QCamera::FocusModeAuto)
        m_cameraOutput->setDeviceSetting(focusModeString.data(), emscripten::val("continuous"));
    focusModeChanged(mode);
}

bool QWasmCamera::isFocusModeSupported(QCamera::FocusMode mode) const
{
    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return false;

    emscripten::val focusMode = caps["focusMode"];
    if (focusMode.isUndefined())
        return false;

    std::vector<std::string> focalModes;

    for (int i = 0; i < focusMode["length"].as<int>(); i++)
        focalModes.push_back(focusMode[i].as<std::string>());

    // Do we need to take into account focusDistance
    // it is not always available, and what distance
    // would be far/near

    bool found = false;
    switch (mode) {
    case QCamera::FocusModeAuto:
        return std::find(focalModes.begin(), focalModes.end(), "continuous") != focalModes.end()
                || std::find(focalModes.begin(), focalModes.end(), "single-shot")
                        != focalModes.end();
    case QCamera::FocusModeAutoNear:
    case QCamera::FocusModeAutoFar:
    case QCamera::FocusModeHyperfocal:
    case QCamera::FocusModeInfinity:
        break;
    case QCamera::FocusModeManual:
        found = std::find(focalModes.begin(), focalModes.end(), "manual") != focalModes.end();
    };
    return found;
}

void QWasmCamera::setTorchMode(QCamera::TorchMode mode)
{
    if (!isTorchModeSupported(mode))
        return;

    if (m_wasmTorchMode == mode)
        return;

    static constexpr std::string_view torchModeString = "torchMode";
    bool hasChanged = false;
    switch (mode) {
    case QCamera::TorchOff:
        m_cameraOutput->setDeviceSetting(torchModeString.data(), emscripten::val(false));
        hasChanged = true;
        break;
    case QCamera::TorchOn:
        m_cameraOutput->setDeviceSetting(torchModeString.data(), emscripten::val(true));
        hasChanged = true;
        break;
    case QCamera::TorchAuto:
        break;
    };
    m_wasmTorchMode = mode;
    if (hasChanged)
        torchModeChanged(m_wasmTorchMode);
}

bool QWasmCamera::isTorchModeSupported(QCamera::TorchMode mode) const
{
    if (!m_cameraIsReady)
        return false;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return false;

    emscripten::val exposureMode = caps["torch"];
    if (exposureMode.isUndefined())
        return false;

    return (mode != QCamera::TorchAuto);
}

void QWasmCamera::setExposureMode(QCamera::ExposureMode mode)
{
    // TODO manually come up with exposureTime values ?
    if (!isExposureModeSupported(mode))
        return;

    if (m_wasmExposureMode == mode)
        return;

    bool hasChanged = false;
    static constexpr std::string_view exposureModeString = "exposureMode";
    switch (mode) {
    case QCamera::ExposureManual:
        m_cameraOutput->setDeviceSetting(exposureModeString.data(), emscripten::val("manual"));
        hasChanged = true;
        break;
    case QCamera::ExposureAuto:
        m_cameraOutput->setDeviceSetting(exposureModeString.data(), emscripten::val("continuous"));
        hasChanged = true;
        break;
    default:
        break;
    };

    if (hasChanged) {
        m_wasmExposureMode = mode;
        exposureModeChanged(m_wasmExposureMode);
    }
}

bool QWasmCamera::isExposureModeSupported(QCamera::ExposureMode mode) const
{
    if (!m_cameraIsReady)
        return false;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return false;

    emscripten::val exposureMode = caps["exposureMode"];
    if (exposureMode.isUndefined())
        return false;

    std::vector<std::string> exposureModes;

    for (int i = 0; i < exposureMode["length"].as<int>(); i++)
        exposureModes.push_back(exposureMode[i].as<std::string>());

    bool found = false;
    switch (mode) {
    case QCamera::ExposureAuto:
        found = std::find(exposureModes.begin(), exposureModes.end(), "continuous")
                != exposureModes.end();
        break;
    case QCamera::ExposureManual:
        found = std::find(exposureModes.begin(), exposureModes.end(), "manual")
                != exposureModes.end();
        break;
    default:
        break;
    };

    return found;
}

void QWasmCamera::setExposureCompensation(float bias)
{
    if (!m_cameraIsReady)
        return;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return;

    emscripten::val exposureComp = caps["exposureCompensation"];
    if (exposureComp.isUndefined())
        return;
    if (m_wasmExposureCompensation == bias)
        return;

    static constexpr std::string_view exposureCompensationModeString = "exposureCompensation";
    m_cameraOutput->setDeviceSetting(exposureCompensationModeString.data(), emscripten::val(bias));
    m_wasmExposureCompensation = bias;
    emit exposureCompensationChanged(m_wasmExposureCompensation);
}

void QWasmCamera::setManualExposureTime(float secs)
{
    if (m_wasmExposureTime == secs)
        return;

    if (!m_cameraIsReady)
        return;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    emscripten::val exposureTime = caps["exposureTime"];
    if (exposureTime.isUndefined())
        return;
    static constexpr std::string_view exposureTimeString = "exposureTime";
    m_cameraOutput->setDeviceSetting(exposureTimeString.data(), emscripten::val(secs));
    m_wasmExposureTime = secs;
    emit exposureTimeChanged(m_wasmExposureTime);
}

int QWasmCamera::isoSensitivity() const
{
    if (!m_cameraIsReady)
        return 0;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return false;

    emscripten::val isoSpeed = caps["iso"];
    if (isoSpeed.isUndefined())
        return 0;

    return isoSpeed.as<double>();
}

void QWasmCamera::setManualIsoSensitivity(int sens)
{
    if (!m_cameraIsReady)
        return;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return;

    emscripten::val isoSpeed = caps["iso"];
    if (isoSpeed.isUndefined())
        return;
    if (m_wasmIsoSensitivity == sens)
        return;
    static constexpr std::string_view isoString = "iso";
    m_cameraOutput->setDeviceSetting(isoString.data(), emscripten::val(sens));
    m_wasmIsoSensitivity = sens;
    emit isoSensitivityChanged(m_wasmIsoSensitivity);
}

bool QWasmCamera::isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const
{
    if (!m_cameraIsReady)
        return false;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return false;

    emscripten::val whiteBalanceMode = caps["whiteBalanceMode"];
    if (whiteBalanceMode.isUndefined())
        return false;

    if (mode == QCamera::WhiteBalanceAuto || mode == QCamera::WhiteBalanceManual)
        return true;

    return false;
}

void QWasmCamera::setWhiteBalanceMode(QCamera::WhiteBalanceMode mode)
{
    if (!isWhiteBalanceModeSupported(mode))
        return;

    if (m_wasmWhiteBalanceMode == mode)
        return;

    bool hasChanged = false;
    static constexpr std::string_view whiteBalanceModeString = "whiteBalanceMode";
    switch (mode) {
    case QCamera::WhiteBalanceAuto:
        m_cameraOutput->setDeviceSetting(whiteBalanceModeString.data(), emscripten::val("auto"));
        hasChanged = true;
        break;
    case QCamera::WhiteBalanceManual:
        m_cameraOutput->setDeviceSetting(whiteBalanceModeString.data(), emscripten::val("manual"));
        hasChanged = true;
        break;
    default:
        break;
    };

    if (hasChanged) {
        m_wasmWhiteBalanceMode = mode;
        emit whiteBalanceModeChanged(m_wasmWhiteBalanceMode);
    }
}

void QWasmCamera::setColorTemperature(int temperature)
{
    if (!m_cameraIsReady)
        return;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return;

    emscripten::val whiteBalanceMode = caps["colorTemperature"];
    if (whiteBalanceMode.isUndefined())
        return;
    if(m_wasmColorTemperature == temperature)
        return;

    static constexpr std::string_view colorBalanceString = "colorTemperature";
    m_cameraOutput->setDeviceSetting(colorBalanceString.data(), emscripten::val(temperature));
    m_wasmColorTemperature = temperature;
    colorTemperatureChanged(m_wasmColorTemperature);
}

void QWasmCamera::createCamera(const QCameraDevice &camera)
{
    m_cameraOutput->addCameraSourceElement(camera.id().toStdString());
    // getUserMedia is async
    QTimer::singleShot(100,[this, &camera] () {
        m_cameraIsReady = m_cameraOutput->isCameraReady();
    });
}

void QWasmCamera::updateCameraFeatures()
{
    if (!m_cameraIsReady)
        return;

    emscripten::val caps = m_cameraOutput->getDeviceCapabilities();
    if (caps.isUndefined())
        return;

    QCamera::Features cameraFeatures;

    if (!caps["colorTemperature"].isUndefined())
        cameraFeatures |= QCamera::Feature::ColorTemperature;

    if (!caps["exposureCompensation"].isUndefined())
        cameraFeatures |= QCamera::Feature::ExposureCompensation;

    if (!caps["iso"].isUndefined())
        cameraFeatures |= QCamera::Feature::IsoSensitivity;

    if (!caps["exposureTime"].isUndefined())
        cameraFeatures |= QCamera::Feature::ManualExposureTime;

    if (!caps["focusDistance"].isUndefined())
        cameraFeatures |= QCamera::Feature::FocusDistance;

    supportedFeaturesChanged(cameraFeatures);
}
