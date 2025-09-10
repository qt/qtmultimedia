// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4L2CAMERA_H
#define QV4L2CAMERA_H

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

#include <QtMultimedia/private/qplatformcamera_p.h>
#include <sys/time.h>

QT_BEGIN_NAMESPACE

class QV4L2FileDescriptor;
class QV4L2MemoryTransfer;
class QSocketNotifier;

struct V4L2CameraInfo
{
    bool formatInitialized = false;

    bool autoWhiteBalanceSupported = false;
    bool colorTemperatureSupported = false;
    bool autoExposureSupported = false;
    bool manualExposureSupported = false;
    bool flashSupported = false;
    bool torchSupported = false;
    qint32 minColorTemp = 5600; // Daylight...
    qint32 maxColorTemp = 5600;
    qint32 minExposure = 0;
    qint32 maxExposure = 0;
    qint32 minExposureAdjustment = 0;
    qint32 maxExposureAdjustment = 0;
    qint32 minFocus = 0;
    qint32 maxFocus = 0;
    qint32 rangedFocus = false;

    int minZoom = 0;
    int maxZoom = 0;
};

QVideoFrameFormat::PixelFormat formatForV4L2Format(uint32_t v4l2Format);
uint32_t v4l2FormatForPixelFormat(QVideoFrameFormat::PixelFormat format);

class QV4L2Camera : public QPlatformCamera
{
    Q_OBJECT

public:
    explicit QV4L2Camera(QCamera *parent);
    ~QV4L2Camera() override;

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;
    bool resolveCameraFormat(const QCameraFormat &format);

    bool isFocusModeSupported(QCamera::FocusMode mode) const override;
    void setFocusMode(QCamera::FocusMode /*mode*/) override;

//    void setCustomFocusPoint(const QPointF &/*point*/) override;
    void setFocusDistance(float) override;
    void zoomTo(float /*newZoomFactor*/, float /*rate*/ = -1.) override;

    void setFlashMode(QCamera::FlashMode /*mode*/) override;
    bool isFlashModeSupported(QCamera::FlashMode mode) const override;
    bool isFlashReady() const override;

    void setTorchMode(QCamera::TorchMode /*mode*/) override;
    bool isTorchModeSupported(QCamera::TorchMode mode) const override;

    void setExposureMode(QCamera::ExposureMode) override;
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;
    void setExposureCompensation(float) override;
    int isoSensitivity() const override;
    void setManualIsoSensitivity(int) override;
    void setManualExposureTime(float) override;
    float exposureTime() const override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode /*mode*/) override;
    void setColorTemperature(int /*temperature*/) override;

    QVideoFrameFormat frameFormat() const override;

private Q_SLOTS:
    void readFrame();

private:
    void setCameraBusy();
    void initV4L2Controls();
    void closeV4L2Fd();
    int setV4L2ColorTemperature(int temperature);
    bool setV4L2Parameter(quint32 id, qint32 value);
    int getV4L2Parameter(quint32 id) const;

    void setV4L2CameraFormat();
    void initV4L2MemoryTransfer();
    void startCapturing();
    void stopCapturing();

private:
    bool m_active = false;
    QCameraDevice m_cameraDevice;

    std::unique_ptr<QSocketNotifier> m_notifier;
    std::unique_ptr<QV4L2MemoryTransfer> m_memoryTransfer;
    std::shared_ptr<QV4L2FileDescriptor> m_v4l2FileDescriptor;

    V4L2CameraInfo m_v4l2Info;

    timeval m_firstFrameTime = { -1, -1 };
    quint32 m_bytesPerLine = 0;
    quint32 m_imageSize = 0;
    QVideoFrameFormat::ColorSpace m_colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    qint64 m_frameDuration = -1;
    bool m_cameraBusy = false;
};

QT_END_NAMESPACE

#endif // QV4L2CAMERA_H
