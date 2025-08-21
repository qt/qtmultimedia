// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAVFCAMERA_P_H
#define QAVFCAMERA_P_H

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

#include <QtMultimedia/private/qavfcamerabase_p.h>

#include <QtFFmpegMediaPluginImpl/private/qavfcamerarotationtracker_p.h>
#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

#include <QtMultimedia/private/qplatformmediacapture_p.h>

#include <dispatch/dispatch.h>

Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureSession);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceInput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureVideoDataOutput);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDevice);
Q_FORWARD_DECLARE_OBJC_CLASS(QAVFSampleBufferDelegate);
Q_FORWARD_DECLARE_OBJC_CLASS(AVCaptureDeviceRotationCoordinator);

QT_BEGIN_NAMESPACE

struct VideoTransformation;

class QAVFCamera : public QAVFCameraBase
{
    Q_OBJECT

public:
    explicit QAVFCamera(QCamera *parent);
    ~QAVFCamera();

    void setCaptureSession(QPlatformMediaCaptureSession *) override;

    std::optional<int> ffmpegHWPixelFormat() const override;

    int cameraPixelFormatScore(QVideoFrameFormat::PixelFormat pixelFmt,
                               QVideoFrameFormat::ColorRange colorRange) const override;

    QVideoFrameFormat frameFormat() const override;

protected:
    void onActiveChanged(bool active) override;
    void onCameraDeviceChanged(const QCameraDevice &device) override;
    bool tryApplyCameraFormat(const QCameraFormat&) override;

private:
    void updateCameraFormat(const QCameraFormat&);
    void refreshAvCaptureSessionInputDevice();
    void setPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat, uint32_t inputCvPixFormat);
    [[nodiscard]] QSize adjustedResolution(const QCameraFormat& format) const;

    void updateRotationTracking();
    void clearRotationTracking();
    int getCurrentRotationAngleDegrees() const;

    QMediaCaptureSession *m_qMediaCaptureSession = nullptr;
    AVCaptureSession *m_avCaptureSession = nullptr;
    AVCaptureDeviceInput *m_avCaptureDeviceVideoInput = nullptr;
    AVCaptureVideoDataOutput *m_avCaptureVideoDataOutput = nullptr;
    QAVFSampleBufferDelegate *m_qAvfSampleBufferDelegate = nullptr;
    dispatch_queue_t m_delegateQueue;
    AVPixelFormat m_hwPixelFormat = AV_PIX_FMT_NONE;
    // The current CVPixelFormat used by the AVCaptureVideoDataOutput.
    // This can in some cases be different from the AVCaptureDeviceFormat
    // used by the camera.
    uint32_t m_cvPixelFormat = 0;

    std::optional<QFFmpeg::AvfCameraRotationTracker> m_qAvfCameraRotationTracker;
};

QT_END_NAMESPACE


#endif // QAVFCAMERA_P_H
