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
#include <QtFFmpegMediaPluginImpl/private/qavfcapturephotooutputdelegate_p.h>
#include <QtFFmpegMediaPluginImpl/private/qavfsamplebufferdelegate_p.h>
#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/private/qplatformmediacapture_p.h>
#include <QtMultimedia/qimagecapture.h>

#import <dispatch/dispatch.h>

#include <optional>

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

    [[nodiscard]] q23::expected<void, QString> requestStillPhotoCapture();

signals:
    void stillPhotoSucceeded(QVideoFrame image);
    void stillPhotoFailed(QImageCapture::Error error, QString errorMsg);

protected:
    void onActiveChanged(bool active) override;
    void onCameraDeviceChanged(const QCameraDevice &, const QCameraFormat &) override;
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

    void onStillPhotoDelegateSucceeded(const QVideoFrame &image);
    void onStillPhotoDelegateFailed(QImageCapture::Error errType, const QString &errMsg);

    [[nodiscard]] QSize adjustedResolution(const QCameraFormat& format) const;

    [[nodiscard]] int getCurrentRotationAngleDegrees() const;

    QMediaCaptureSession *m_qMediaCaptureSession = nullptr;
    AVFScopedPointer<AVCaptureSession> m_avCaptureSession;
    AVFScopedPointer<AVCapturePhotoOutput> m_avCapturePhotoOutput;
    AVFScopedPointer<AVCaptureDeviceInput> m_avCaptureDeviceVideoInput;
    AVFScopedPointer<AVCaptureVideoDataOutput> m_avCaptureVideoDataOutput;
    AVFScopedPointer<QAVFSampleBufferDelegate> m_qAvfSampleBufferDelegate;
    AVPixelFormat m_hwPixelFormat = AV_PIX_FMT_NONE;
    // The current CVPixelFormat used by the AVCaptureVideoDataOutput.
    // This can in some cases be different from the AVCaptureDeviceFormat
    // used by the camera.
    uint32_t m_cvPixelFormat = 0;

    std::optional<QFFmpeg::AvfCameraRotationTracker> m_qAvfCameraRotationTracker;

    // Will be non-null whenever a still photo is in progress.
    //
    // TODO: It can be problematic if we change QMediaCaptureSession in the midst of a capture.
    // We might end up signaling a different QImageCapture than the one that requested
    // the capture. We should likely cancel any on-going still photo captures when this
    // happens.
    AVFScopedPointer<QAVFCapturePhotoOutputDelegate> m_qAvfCapturePhotoOutputDelegate;
    [[nodiscard]] bool stillPhotoCaptureInProgress() const
    {
        return m_qAvfCapturePhotoOutputDelegate.data();
    }

    AVFScopedPointer<dispatch_queue_t> m_delegateQueue;
};

} // namespace QFFmpeg

QT_END_NAMESPACE

#endif // QAVFCAMERA_P_H
