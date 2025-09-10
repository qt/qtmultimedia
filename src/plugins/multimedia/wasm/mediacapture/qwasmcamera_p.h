// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWASMCAMERA_H
#define QWASMCAMERA_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qplatformcamera_p.h>
#include <private/qplatformvideodevices_p.h>
#include <common/qwasmvideooutput_p.h>

#include <QCameraDevice>
#include <QtCore/qloggingcategory.h>

#include <emscripten/val.h>
#include <emscripten/bind.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(qWasmCamera)

class QWasmMediaCaptureSession;

class QWasmCamera : public QPlatformCamera
{
    Q_OBJECT

public:
    explicit QWasmCamera(QCamera *camera);
    ~QWasmCamera();

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    void setCaptureSession(QPlatformMediaCaptureSession *session) override;

    void setFocusMode(QCamera::FocusMode mode) override;
    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

    void setTorchMode(QCamera::TorchMode mode) override;
    bool isTorchModeSupported(QCamera::TorchMode mode) const override;

    void setExposureMode(QCamera::ExposureMode mode) override;
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;
    void setExposureCompensation(float bias) override;

    void setManualExposureTime(float) override;
    int isoSensitivity() const override;
    void setManualIsoSensitivity(int) override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode mode) override;

    void setColorTemperature(int temperature) override;

    QWasmVideoOutput *cameraOutput() { return m_cameraOutput.data(); }
Q_SIGNALS:
    void cameraIsReady();
private:
    void createCamera(const QCameraDevice &camera);
    void updateCameraFeatures();

    QCameraDevice m_cameraDev;
    QWasmMediaCaptureSession *m_CaptureSession = nullptr;
    bool m_cameraActive = false;
    QScopedPointer <QWasmVideoOutput> m_cameraOutput;

    emscripten::val supportedCapabilities = emscripten::val::object(); // browser
    emscripten::val currentCapabilities = emscripten::val::object(); // camera track
    emscripten::val currentSettings = emscripten::val::object(); // camera track

    QCamera::TorchMode m_wasmTorchMode;
    QCamera::ExposureMode m_wasmExposureMode;
    float m_wasmExposureTime;
    float m_wasmExposureCompensation;
    int m_wasmIsoSensitivity;
    QCamera::WhiteBalanceMode m_wasmWhiteBalanceMode;
    int m_wasmColorTemperature;
    bool m_cameraIsReady = false;
    bool m_cameraShouldStartActive = false;
    bool m_shouldBeActive = false;
    QMetaObject::Connection m_readyChangedConnection;
};

QT_END_NAMESPACE

#endif // QWASMCAMERA_H
