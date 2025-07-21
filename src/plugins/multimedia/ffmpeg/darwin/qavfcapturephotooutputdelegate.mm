// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qavfcapturephotooutputdelegate_p.h>

#include <QtCore/private/qexpected_p.h>
#include <QtCore/qmutex.h>

#include <QtFFmpegMediaPluginImpl/private/qavfcamerarotationtracker_p.h>
#include <QtFFmpegMediaPluginImpl/private/qcvimagevideobuffer_p.h>

#include <QtMultimedia/private/qavfcameradebug_p.h>
#include <QtMultimedia/private/qavfcamerautility_p.h>
#include <QtMultimedia/private/qavfhelpers_p.h>
#include <QtMultimedia/private/qmultimediautils_p.h>
#include <QtMultimedia/private/qvideoframe_p.h>

QT_USE_NAMESPACE

namespace {

struct ImageCaptureErrorPair {
    QImageCapture::Error type;
    QString message;
};

} // Anonymous namespace

[[nodiscard]] static q23::expected<QVideoFrame, ImageCaptureErrorPair> processAvCaptureOutput(
    AVCapturePhoto *,
    QT_MANGLE_NAMESPACE(QAVFCapturePhotoOutputDelegate) *,
    NSError *);

@implementation QT_MANGLE_NAMESPACE(QAVFCapturePhotoOutputDelegate) {
@private
    QFFmpeg::QAVFStillPhotoNotifier m_notifier;

@public
    QFFmpeg::AvfCameraRotationTracker m_qAvfCameraRotationTracker;
}

- (instancetype) init:(AVCaptureDevice *)captureDevice
{
    if (!(self = [super init]))
        return nil;

    Q_ASSERT(captureDevice);

    m_qAvfCameraRotationTracker = QFFmpeg::AvfCameraRotationTracker(captureDevice);

    return self;
}

- (QFFmpeg::QAVFStillPhotoNotifier &)notifier
{
    return self->m_notifier;
}

// The AVCapturePhotoOutput gives no guarantee on which thread may call this function,
// so this function needs to be completely thread safe.
- (void) captureOutput:(AVCapturePhotoOutput *)output
    didFinishProcessingPhoto:(AVCapturePhoto *)photo
    error:(NSError *)nsError
{
    q23::expected<QVideoFrame, ImageCaptureErrorPair> processResult = processAvCaptureOutput(
        photo,
        self,
        nsError);

    if (processResult) {
        emit self->m_notifier.succeeded(std::move(*processResult));
    } else {
        emit self->m_notifier.failed(
            processResult.error().type,
            std::move(processResult.error().message));
    }
}

@end

static q23::expected<QVideoFrame, ImageCaptureErrorPair> processAvCaptureOutput(
    AVCapturePhoto *photo,
    QT_MANGLE_NAMESPACE(QAVFCapturePhotoOutputDelegate) *captureDelegate,
    NSError *nsError)
{
    Q_ASSERT(photo);
    Q_ASSERT(captureDelegate);

    using namespace Qt::Literals::StringLiterals;

    if (nsError) {
        qCWarning(qLcCamera)
            << "Error while finalizing AVCapturePhotoOutput capture:"
            << QString::fromNSString(nsError.localizedDescription);
        return q23::unexpected{ ImageCaptureErrorPair {
            QImageCapture::Error::ResourceError,
            u"Internal error while finalizing still photo capture"_s } };
    }

    if (!photo.pixelBuffer) {
        qCWarning(qLcCamera)
            << "When finalizing AVCapturePhotoOutput capture, pixelBuffer was null";
        return q23::unexpected{ ImageCaptureErrorPair {
            QImageCapture::Error::ResourceError,
            u"Internal error while finalizing still photo capture"_s } };
    }

    QVideoFrameFormat format = QAVFHelpers::videoFormatForImageBuffer(photo.pixelBuffer);
    if (!format.isValid()) {
        qCWarning(qLcCamera)
            << "Unable to determine QVideoFrameFormat based on "
                "AVCapturePhotoOutput result. Likely an issue with the"
                "with the configuration of the still photo capture";
        return q23::unexpected{ ImageCaptureErrorPair {
            QImageCapture::Error::ResourceError,
            u"Internal error while finalizing still photo capture"_s } };
    }

    // Apply rotation data to the format
    const int cameraRotationDegrees = captureDelegate->m_qAvfCameraRotationTracker
        .rotationDegrees();
    format.setRotation(qVideoRotationFromDegrees(cameraRotationDegrees));

    auto sharedPixelBuffer = QAVFHelpers::QSharedCVPixelBuffer(
        photo.pixelBuffer,
        QAVFHelpers::QSharedCVPixelBuffer::RefMode::NeedsRef);
    auto videoFrame = QVideoFramePrivate::createFrame(
        std::make_unique<QFFmpeg::CVImageVideoBuffer>(std::move(sharedPixelBuffer)),
        std::move(format));
    if (!videoFrame.isValid()) {
        qCWarning(qLcCamera) << "Unable to create QVideoFrame from AVCapturePhotoOutput result";
        return q23::unexpected{ ImageCaptureErrorPair {
            QImageCapture::Error::ResourceError,
            u"Internal error while finalizing still photo capture"_s } };
    }

    // If we are the front-facing camera, we need to provide the flip horizontally flag to the
    // frame (not the format).
    AVCaptureDevice *avCaptureDevice = captureDelegate->m_qAvfCameraRotationTracker
        .avCaptureDevice();
    Q_ASSERT(avCaptureDevice);
    videoFrame.setMirrored(avCaptureDevice.position == AVCaptureDevicePositionFront);

    return videoFrame;
}
