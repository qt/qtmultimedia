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

#include <QtCore/private/qexpected_p.h>

#include <QtFFmpegMediaPluginImpl/private/qavfcamerarotationtracker_p.h>
#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType
#include <QtFFmpegMediaPluginImpl/private/qavfsamplebufferdelegate_p.h>

#import <AVFoundation/AVFoundation.h>
#include <dispatch/dispatch.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

class QAVFCamera : public QAVFCameraBase
{
    Q_OBJECT

public:
    explicit QAVFCamera(QCamera &parent);
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
    void clearAvCaptureSessionInputDevice();
    [[nodiscard]] q23::expected<void, QString> setupAvCaptureSessionInputDevice(AVCaptureDevice *);
    void clearAvCaptureVideoDataOutput();
    [[nodiscard]] q23::expected<void, QString> setupAvCaptureVideoDataOutput(AVCaptureDevice *);
    [[nodiscard]] q23::expected<void, QString> tryApplyFormatToCaptureSession(
        AVCaptureDevice *,
        AVCaptureDeviceFormat *,
        const QCameraFormat &);
    void clearRotationTracking();
    void setupRotationTracking(AVCaptureDevice *);
    void clearCaptureSessionConfiguration();
    [[nodiscard]] q23::expected<void, QString> tryConfigureCaptureSession(
        const QCameraDevice &,
        const QCameraFormat &);
    [[nodiscard]] q23::expected<void, QString> tryConfigureCaptureSession(
        AVCaptureDevice *,
        const QCameraFormat &);
    [[nodiscard]] q23::expected<void, QString> tryConfigureCaptureSession(
        AVCaptureDevice *,
        AVCaptureDeviceFormat *,
        const QCameraFormat &);

    [[nodiscard]] QSize adjustedResolution(const QCameraFormat& format) const;

    [[nodiscard]] int getCurrentRotationAngleDegrees() const;

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

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QAVFCAMERA_P_H
