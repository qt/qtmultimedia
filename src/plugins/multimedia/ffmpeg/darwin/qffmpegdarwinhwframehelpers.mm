// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtFFmpegMediaPluginImpl/private/qffmpegdarwinhwframehelpers_p.h>

#include <QtMultimedia/private/qvideoframe_p.h>

#define AVMediaType XAVMediaType
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#undef AVMediaType

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QFFmpeg {

namespace {

// Helper function to allocate a FFmpeg HwFrame, from a given CVPixelBufferRef.
// The FFmpeg HwFrame becomes a owning reference for this CVPixelBufferRef.
// Make sure this is compatible with the layout used in ffmpeg's hwcontext_videotoolbox
[[nodiscard]] q23::expected<AVFrameUPtr, QString> allocHWFrame(
    AVBufferRef *hwContext,
    QAVFHelpers::QSharedCVPixelBuffer sharedPixBuf)
{
    Q_ASSERT(sharedPixBuf);

    AVHWFramesContext *ctx = (AVHWFramesContext *)hwContext->data;

    if (ctx->width != (int)CVPixelBufferGetWidth(sharedPixBuf.get())
        || ctx->height != (int)CVPixelBufferGetHeight(sharedPixBuf.get()))
        return q23::unexpected{
            u"Size of given CVPixelBufferRef does not match the FFmpeg hw frame context"_s };

    auto frame = QFFmpeg::makeAVFrame();
    if (!frame)
        return q23::unexpected{ u"Failed to allocate FFmpeg AVFrame"_s };

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
    return frame;
}

} // Anonymous namespace end

q23::expected<QVideoFrame, QString> qVideoFrameFromCvPixelBuffer(
    const QFFmpeg::HWAccel &hwAccel,
    std::chrono::microseconds presentationTimeStamp,
    const QAVFHelpers::QSharedCVPixelBuffer &imageBuffer,
    QVideoFrameFormat format)
{
    q23::expected<AVFrameUPtr, QString> avFrameResult = allocHWFrame(
        hwAccel.hwFramesContextAsBuffer(),
        imageBuffer);
    if (!avFrameResult)
        return q23::unexpected{ u"Failed to allocate FFmpeg HwFrame"_s };
    AVFrameUPtr &avFrame = *avFrameResult;

    avFrame->pts = presentationTimeStamp.count();

    return QVideoFramePrivate::createFrame(
        std::make_unique<QFFmpegVideoBuffer>(std::move(avFrame)),
        format);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
