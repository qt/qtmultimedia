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

#include <QHash>
#include <private/qplatformcamera_p.h>
#include <private/qmultimediautils_p.h>
#include "qgstreamermediacapture_p.h"
#include <qgst_p.h>

QT_BEGIN_NAMESPACE

class QGstreamerCamera : public QPlatformCamera
{
    Q_OBJECT
public:
    static QMaybe<QPlatformCamera *> create(QCamera *camera);

    virtual ~QGstreamerCamera();

    bool isActive() const override;
    void setActive(bool active) override;

    void setCamera(const QCameraDevice &camera) override;
    bool setCameraFormat(const QCameraFormat &format) override;

    QGstElement gstElement() const { return gstCameraBin.element(); }
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

    QString v4l2Device() const { return m_v4l2Device; }
    bool isV4L2Camera() const { return !m_v4l2Device.isEmpty(); }

private:
    QGstreamerCamera(QGstElement videotestsrc, QGstElement capsFilter, QGstElement videoconvert,
                     QGstElement videoscale, QCamera *camera);

    void updateCameraProperties();
#if QT_CONFIG(linux_v4l)
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
    int v4l2FileDescriptor = -1;
#endif

    QCameraDevice m_cameraDevice;

    QGstBin gstCameraBin;
    QGstElement gstCamera;
    QGstElement gstCapsFilter;
    QGstElement gstDecode;
    QGstElement gstVideoConvert;
    QGstElement gstVideoScale;

    bool m_active = false;
    QString m_v4l2Device;
};

QT_END_NAMESPACE

#endif // QGSTREAMERCAMERACONTROL_H
