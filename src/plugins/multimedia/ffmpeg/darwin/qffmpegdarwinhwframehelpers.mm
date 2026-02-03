// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qffmpegdarwinhwframehelpers_p.h>

#include <QtMultimedia/private/qvideoframe_p.h>

#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

namespace {

// Make sure this is compatible with the layout used in ffmpeg's hwcontext_videotoolbox
[[nodiscard]] AVFrameUPtr allocHWFrame(
    AVBufferRef *hwContext,
    QAVFHelpers::QSharedCVPixelBuffer sharedPixBuf)
{
    Q_ASSERT(sharedPixBuf);

    AVHWFramesContext *ctx = (AVHWFramesContext *)hwContext->data;
    auto frame = QFFmpeg::makeAVFrame();
    frame->hw_frames_ctx = av_buffer_ref(hwContext);
    frame->extended_data = frame->data;

    CVPixelBufferRef pixbuf = sharedPixBuf.release();
    auto releasePixBufFn = [](void* opaquePtr, uint8_t *) {
        CVPixelBufferRelease(static_cast<CVPixelBufferRef>(opaquePtr));
    };
    frame->buf[0] = av_buffer_create(nullptr, 0, releasePixBufFn, pixbuf, 0);

    // It is convention to use 4th data plane for hardware frames.
    frame->data[3] = (uint8_t *)pixbuf;
    frame->width = ctx->width;
    frame->height = ctx->height;
    frame->format = AV_PIX_FMT_VIDEOTOOLBOX;
    if (frame->width != (int)CVPixelBufferGetWidth(pixbuf)
        || frame->height != (int)CVPixelBufferGetHeight(pixbuf)) {

        // This can happen while changing camera format
        return nullptr;
    }
    return frame;
}

} // Anonymous namespace end

QVideoFrame qVideoFrameFromCvPixelBuffer(
    const QFFmpeg::HWAccel &hwAccel,
    qint64 presentationTimeStamp,
    const QAVFHelpers::QSharedCVPixelBuffer &imageBuffer,
    QVideoFrameFormat format)
{
    AVFrameUPtr avFrame = allocHWFrame(
        hwAccel.hwFramesContextAsBuffer(),
        imageBuffer);
    if (!avFrame)
        return {};

#ifdef USE_SW_FRAMES
    {
        auto swFrame = QFFmpeg::makeAVFrame();
        /* retrieve data from GPU to CPU */
        const int ret = av_hwframe_transfer_data(swFrame.get(), avFrame.get(), 0);
        if (ret < 0) {
            qWarning() << "Error transferring the data to system memory:" << ret;
        } else {
            avFrame = std::move(swFrame);
        }
    }
#endif

    avFrame->pts = presentationTimeStamp;

    return QVideoFramePrivate::createFrame(
        std::make_unique<QFFmpegVideoBuffer>(std::move(avFrame)),
        format);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
