// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMERCAMERACONTROL_H
#define QGSTREAMERCAMERACONTROL_H

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
#include <private/qmultimediautils_p.h>
#include <QtCore/private/qexpected_p.h>
#include <QtCore/private/quniquehandle_types_p.h>

#include <mediacapture/qgstreamermediacapturesession_p.h>
#include <common/qgst_p.h>
#include <common/qgstpipeline_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerCameraBase : public QPlatformCamera
{
public:
    using QPlatformCamera::QPlatformCamera;

    virtual QGstElement gstElement() const = 0;
};

class QGstreamerCamera : public QGstreamerCameraBase
{
public:
    static q23::expected<QPlatformCamera *, QString> create(QCamera *camera);

    ~QGstreamerCamera() override;

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    QGstElement gstElement() const override { return gstCameraBin; }
#if QT_CONFIG(gstreamer_photography)
    GstPhotography *photography() const;
#endif

    void setFocusMode(QCamera::FocusMode mode) override;
    bool isFocusModeSupported(QCamera::FocusMode mode) const override;

    void setFlashMode(QCamera::FlashMode mode) override;
    bool isFlashModeSupported(QCamera::FlashMode mode) const override;
    bool isFlashReady() const override;

    void setExposureMode(QCamera::ExposureMode) override;
    bool isExposureModeSupported(QCamera::ExposureMode mode) const override;
    void setExposureCompensation(float) override;
    void setManualIsoSensitivity(int) override;
    int isoSensitivity() const override;
    void setManualExposureTime(float) override;
    float exposureTime() const override;

    bool isWhiteBalanceModeSupported(QCamera::WhiteBalanceMode mode) const override;
    void setWhiteBalanceMode(QCamera::WhiteBalanceMode mode) override;
    void setColorTemperature(int temperature) override;

private:
    QGstreamerCamera(QCamera *camera);

    void updateCameraProperties();

#if QT_CONFIG(linux_v4l)
    bool isV4L2Camera() const;
    void initV4L2Controls();
    int setV4L2ColorTemperature(int temperature);
    bool setV4L2Parameter(quint32 id, qint32 value);
    int getV4L2Parameter(quint32 id) const;

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

    template <typename Functor>
    auto withV4L2DeviceFileDescriptor(Functor &&f) const
    {
        using ReturnType = std::invoke_result_t<Functor, int>;
        Q_ASSERT(isV4L2Camera());

        if (int gstreamerDeviceFd = gstCamera.getInt("device-fd"); gstreamerDeviceFd != -1)
            return f(gstreamerDeviceFd);

        auto v4l2FileDescriptor = QUniqueFileDescriptorHandle{
            QT_OPEN(m_v4l2DevicePath.toLocal8Bit().constData(), O_RDONLY),
        };
        if (!v4l2FileDescriptor) {
            qWarning() << "Unable to open the camera" << m_v4l2DevicePath
                       << "for read to query the parameter info:" << qt_error_string(errno);
            if constexpr (std::is_void_v<ReturnType>)
                return;
            else
                return ReturnType{};
        }
        return f(v4l2FileDescriptor.get());
    }
#endif

    QCameraDevice m_cameraDevice;

    QGstBin gstCameraBin;
    QGstElement gstCamera;
    QGstElement gstCapsFilter;
    QGstElement gstDecode;
    QGstElement gstVideoConvert;
    QGstElement gstVideoScale;

    bool m_active = false;
    QString m_v4l2DevicePath;

    std::optional<QCameraFormat> m_currentCameraFormat;

    template <typename Functor>
    void updateCamera(Functor &&f)
    {
        QGstPipeline pipeline = gstVideoConvert.getPipeline();
        if (pipeline)
            pipeline.setState(GstState::GST_STATE_READY);

        gstVideoConvert.sink().modifyPipelineInIdleProbe([&] {
            f();
        });

        if (pipeline)
            pipeline.setState(GstState::GST_STATE_PLAYING);
    }
};

class QGstreamerCustomCamera : public QGstreamerCameraBase
{
public:
    explicit QGstreamerCustomCamera(QCamera *);
    explicit QGstreamerCustomCamera(QCamera *, QGstElement element);

    QGstElement gstElement() const override { return gstCamera; }
    void setCamera(const QCameraDevice &) override;

    bool isActive() const override;
    void setActive(bool) override;

private:
    QGstElement gstCamera;
    bool m_active{};
    const bool m_userProvidedGstElement;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAMERACONTROL_H
