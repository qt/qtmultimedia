// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QFFMPEGCAMERA_H
#define QFFMPEGCAMERA_H

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
#include <private/qplatformmediaintegration_p.h>

#include <qfilesystemwatcher.h>
#include <qsocketnotifier.h>
#include <qmutex.h>

QT_BEGIN_NAMESPACE

class QV4L2CameraDevices : public QPlatformVideoDevices
{
    Q_OBJECT
public:
    QV4L2CameraDevices(QPlatformMediaIntegration *integration);

    QList<QCameraDevice> videoDevices() const override;

public Q_SLOTS:
    void checkCameras();

private:
    bool doCheckCameras();

private:
    QList<QCameraDevice> m_cameras;
    QFileSystemWatcher m_deviceWatcher;
};

struct QV4L2CameraBuffers
{
public:
    ~QV4L2CameraBuffers();

    void release(int index);
    void unmapBuffers();

    QAtomicInt ref;
    QMutex mutex;
    struct MappedBuffer {
        void *data;
        qsizetype size;
    };
    QList<MappedBuffer> mappedBuffers;
    int v4l2FileDescriptor = -1;
};

class Q_MULTIMEDIA_EXPORT QV4L2Camera : public QPlatformCamera
{
    Q_OBJECT

public:
    explicit QV4L2Camera(QCamera *parent);
    ~QV4L2Camera();

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

    void releaseBuffer(int index);

private Q_SLOTS:
    void readFrame();

private:
    void setCameraBusy();

    bool m_active = false;

    QCameraDevice m_cameraDevice;

    void initV4L2Controls();
    void closeV4L2Fd();
    int setV4L2ColorTemperature(int temperature);
    bool setV4L2Parameter(quint32 id, qint32 value);
    int getV4L2Parameter(quint32 id) const;

    void setV4L2CameraFormat();
    void initMMap();
    void startCapturing();
    void stopCapturing();

    QSocketNotifier *notifier = nullptr;
    QExplicitlySharedDataPointer<QV4L2CameraBuffers> d;

    bool v4l2AutoWhiteBalanceSupported = false;
    bool v4l2ColorTemperatureSupported = false;
    bool v4l2AutoExposureSupported = false;
    bool v4l2ManualExposureSupported = false;
    qint32 v4l2MinColorTemp = 5600; // Daylight...
    qint32 v4l2MaxColorTemp = 5600;
    qint32 v4l2MinExposure = 0;
    qint32 v4l2MaxExposure = 0;
    qint32 v4l2MinExposureAdjustment = 0;
    qint32 v4l2MaxExposureAdjustment = 0;
    qint32 v4l2MinFocus = 0;
    qint32 v4l2MaxFocus = 0;
    qint32 v4l2RangedFocus = false;
    bool v4l2FlashSupported = false;
    bool v4l2TorchSupported = false;
    int v4l2MinZoom = 0;
    int v4l2MaxZoom = 0;
    timeval firstFrameTime = {-1, -1};
    int bytesPerLine = -1;
    QVideoFrameFormat::ColorSpace colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    qint64 frameDuration = -1;
    bool cameraBusy = false;
};

QT_END_NAMESPACE


#endif  // QFFMPEGCAMERA_H

