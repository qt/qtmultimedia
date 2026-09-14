// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef NV12HWFRAMECONTEXT_P_H
#define NV12HWFRAMECONTEXT_P_H

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qsize.h>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
}

#include <optional>

using namespace QFFmpeg;

// A hw-frames context plus one hw AVFrame filled with a known NV12 pattern,
// suitable for exercising a texture converter without a real decoder.
struct Nv12HwFrameContext
{
    HWAccelUPtr hwAccel;
    AVBufferUPtr framesContext;
    AVFrameUPtr hwFrame;

    QSize size;
    quint8 lumaValue = 0;
    quint8 chromaUValue = 0;
    quint8 chromaVValue = 0;
};

// Creates an Nv12HwFrameContext for a hw device of type "deviceType", with hw pixel format
// "hwFormat". "configureFramesContext" is invoked on the allocated AVHWFramesContext right
// before av_hwframe_ctx_init(), so that callers can set backend-specific fields (e.g. VAAPI
// surface attributes).
template <typename ConfigureFramesContext>
std::optional<Nv12HwFrameContext>
createNv12HwFrameContext(AVHWDeviceType deviceType, AVPixelFormat hwFormat, QSize size,
                         quint8 lumaValue, quint8 chromaUValue, quint8 chromaVValue,
                         ConfigureFramesContext configureFramesContext)
{
    HWAccelUPtr hwAccel = HWAccel::create(deviceType);
    if (!hwAccel)
        return std::nullopt;

    AVBufferUPtr framesContext(av_hwframe_ctx_alloc(hwAccel->hwDeviceContextAsBuffer()));
    if (!framesContext)
        return std::nullopt;

    auto *ctx = reinterpret_cast<AVHWFramesContext *>(framesContext->data);
    ctx->format = hwFormat;
    ctx->sw_format = AV_PIX_FMT_NV12;
    ctx->width = size.width();
    ctx->height = size.height();

    configureFramesContext(*ctx);

    if (av_hwframe_ctx_init(framesContext.get()) < 0)
        return std::nullopt;

    AVFrameUPtr swFrame = makeAVFrame();
    swFrame->format = AV_PIX_FMT_NV12;
    swFrame->width = size.width();
    swFrame->height = size.height();
    if (av_frame_get_buffer(swFrame.get(), 32) < 0)
        return std::nullopt;

    const QSpan<uint8_t> lumaPlane{
        swFrame->data[0],
        qsizetype(swFrame->linesize[0]) * size.height(),
    };
    ranges::fill(lumaPlane, lumaValue);

    QByteArray uvTexel;
    uvTexel += char(chromaUValue);
    uvTexel += char(chromaVValue);
    const QByteArray chromaRow = uvTexel.repeated(swFrame->linesize[1] / 2);
    const QSpan<const char> chromaRowSpan{ chromaRow.constData(), chromaRow.size() };

    const int chromaHeight = (size.height() + 1) / 2;
    for (int row = 0; row < chromaHeight; ++row)
        ranges::copy(chromaRowSpan, swFrame->data[1] + row * swFrame->linesize[1]);

    AVFrameUPtr hwFrame = makeAVFrame();
    if (av_hwframe_get_buffer(framesContext.get(), hwFrame.get(), 0) < 0)
        return std::nullopt;

    if (av_hwframe_transfer_data(hwFrame.get(), swFrame.get(), 0) < 0)
        return std::nullopt;

    Nv12HwFrameContext result;
    result.hwAccel = std::move(hwAccel);
    result.framesContext = std::move(framesContext);
    result.hwFrame = std::move(hwFrame);
    result.size = size;
    result.lumaValue = lumaValue;
    result.chromaUValue = chromaUValue;
    result.chromaVValue = chromaVValue;
    return result;
}

#endif // NV12HWFRAMECONTEXT_P_H
