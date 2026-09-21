// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef HWFRAMECONTEXT_P_H
#define HWFRAMECONTEXT_P_H

#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeghwaccel_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>

#include <QtMultimedia/qvideoframe.h>
#include <QtMultimedia/private/qvideoframe_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>
#include <QtMultimediaTestLib/private/qvideoframetestutils_p.h>

#include <QtCore/private/qexpected_p.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>

#include <memory>

extern "C" {
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
}

using namespace QFFmpeg;
using namespace Qt::Literals;

// A hw-frames context plus one hw AVFrame filled with a known per-plane byte pattern, suitable
// for exercising a texture converter or QVideoFrame::map() without a real decoder.
struct HwFrameContext
{
    HWAccelUPtr hwAccel;
    AVBufferUPtr framesContext;
    AVFrameUPtr hwFrame;

    QSize size;
    QVideoFrameFormat::PixelFormat pixelFormat;
    QList<QByteArray> planeTexels; // the fill pattern each plane was uniformly populated with
};

// Creates an HwFrameContext for a hw device of type "deviceType", with hw pixel format
// "hwFormat" and decoded (sw) pixel format "swFormat". Each entry in "planeTexels" is the byte
// pattern one plane is uniformly filled with (e.g. {luma}, {chromaU, chromaV} for NV12).
// "configureFramesContext" is invoked on the allocated AVHWFramesContext right before
// av_hwframe_ctx_init(), so that callers can set backend-specific fields (e.g. VAAPI surface
// attributes).
template <typename ConfigureFramesContext>
q23::expected<HwFrameContext, QString>
createHwFrameContext(AVHWDeviceType deviceType, AVPixelFormat hwFormat, AVPixelFormat swFormat,
                     QSize size, QList<QByteArray> planeTexels,
                     ConfigureFramesContext configureFramesContext)
{
    const QVideoFrameFormat::PixelFormat pixelFormat =
            QFFmpegVideoBuffer::toQtPixelFormat(swFormat);
    // QFFmpegVideoBuffer::map() round-trips a hw frame's sw_format through toQtPixelFormat() and
    // back via convertSWFrame(); a format whose round trip isn't the identity gets silently
    // sws_scale'd, which would make a pixel-exact comparison against planeTexels meaningless.
    Q_ASSERT(QFFmpegVideoBuffer::toAVPixelFormat(pixelFormat) == swFormat);

    HWAccelUPtr hwAccel = HWAccel::create(deviceType);
    if (!hwAccel)
        return q23::unexpected(u"No usable hw device of the requested type on this system"_s);

    AVBufferUPtr framesContext(av_hwframe_ctx_alloc(hwAccel->hwDeviceContextAsBuffer()));
    if (!framesContext)
        return q23::unexpected(u"av_hwframe_ctx_alloc failed"_s);

    auto *ctx = reinterpret_cast<AVHWFramesContext *>(framesContext->data);
    ctx->format = hwFormat;
    ctx->sw_format = swFormat;
    ctx->width = size.width();
    ctx->height = size.height();

    configureFramesContext(*ctx);

    if (av_hwframe_ctx_init(framesContext.get()) < 0)
        return q23::unexpected(
                u"av_hwframe_ctx_init failed (format unsupported by this driver?)"_s);

    AVFrameUPtr swFrame = makeAVFrame();
    swFrame->format = swFormat;
    swFrame->width = size.width();
    swFrame->height = size.height();
    if (av_frame_get_buffer(swFrame.get(), 32) < 0)
        return q23::unexpected(u"av_frame_get_buffer failed"_s);

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    Q_ASSERT(desc->nplanes == planeTexels.size());
    for (int plane = 0; plane < desc->nplanes; ++plane) {
        const int width = desc->widthForPlane(size.width(), plane);
        const int height = desc->heightForPlane(size.height(), plane);
        QtMultimediaTest::fillPlaneRows(swFrame->data[plane], swFrame->linesize[plane], width,
                                        height, planeTexels[plane]);
    }

    AVFrameUPtr hwFrame = makeAVFrame();
    if (av_hwframe_get_buffer(framesContext.get(), hwFrame.get(), 0) < 0)
        return q23::unexpected(u"av_hwframe_get_buffer failed"_s);

    if (av_hwframe_transfer_data(hwFrame.get(), swFrame.get(), 0) < 0)
        return q23::unexpected(u"av_hwframe_transfer_data failed"_s);

    return HwFrameContext{ std::move(hwAccel), std::move(framesContext), std::move(hwFrame), size,
                           pixelFormat,        std::move(planeTexels) };
}

// Convenience wrapper for a two-plane 4:2:0 8-bit format (NV12): a full-size luma plane and a
// half-size interleaved-chroma plane.
template <typename ConfigureFramesContext>
q23::expected<HwFrameContext, QString>
createNv12HwFrameContext(AVHWDeviceType deviceType, AVPixelFormat hwFormat, QSize size,
                         quint8 lumaValue, quint8 chromaUValue, quint8 chromaVValue,
                         ConfigureFramesContext configureFramesContext)
{
    return createHwFrameContext(deviceType, hwFormat, AV_PIX_FMT_NV12, size,
                                { QByteArray(1, char(lumaValue)),
                                  QtMultimediaTest::makeBytes({ chromaUValue, chromaVValue }) },
                                configureFramesContext);
}

// Convenience wrapper for a two-plane 4:2:0 10-bit format (P010): each plane's samples are
// 16-bit, with the 10-bit value left-shifted into the high bits (FFmpeg's P010 convention).
template <typename ConfigureFramesContext>
q23::expected<HwFrameContext, QString>
createP010HwFrameContext(AVHWDeviceType deviceType, AVPixelFormat hwFormat, QSize size,
                         quint16 lumaValue, quint16 chromaUValue, quint16 chromaVValue,
                         ConfigureFramesContext configureFramesContext)
{
    const auto toBytes = [](quint16 value) {
        const quint16 shifted = quint16(value << 6);
        return QByteArray(reinterpret_cast<const char *>(&shifted), sizeof(shifted));
    };
    return createHwFrameContext(
            deviceType, hwFormat, AV_PIX_FMT_P010, size,
            { toBytes(lumaValue), toBytes(chromaUValue) + toBytes(chromaVValue) },
            configureFramesContext);
}

// Convenience wrapper for a single-plane packed 8-bit format (BGRA8888).
template <typename ConfigureFramesContext>
q23::expected<HwFrameContext, QString>
createSinglePlaneHwFrameContext(AVHWDeviceType deviceType, AVPixelFormat hwFormat, QSize size,
                                QByteArray texel, ConfigureFramesContext configureFramesContext)
{
    return createHwFrameContext(deviceType, hwFormat, AV_PIX_FMT_BGRA, size, { std::move(texel) },
                                configureFramesContext);
}

// Constructs a QFFmpegVideoBuffer/QVideoFrame around "hwFrame" (as QFFmpeg::VideoRenderer does)
// and maps it, verifying every plane's content against "planeTexels" -- exercising exactly the
// CPU (QVideoFrame::map()) path, independent of any GPU texture importer. Returns an error
// describing the mismatch, if any.
inline q23::expected<void, QString> verifyMappedHwFrame(AVFrameUPtr hwFrame,
                                                        const QList<QByteArray> &planeTexels)
{
    auto buffer = std::make_unique<QFFmpegVideoBuffer>(std::move(hwFrame));
    const QVideoFrameFormat::PixelFormat pixelFormat = buffer->pixelFormat();
    QVideoFrameFormat format(buffer->size(), pixelFormat);
    QVideoFrame frame = QVideoFramePrivate::createFrame(std::move(buffer), format);

    if (!frame.map(QVideoFrame::ReadOnly))
        return q23::unexpected(u"QVideoFrame::map() failed"_s);

    const auto *desc = QVideoTextureHelper::textureDescription(pixelFormat);
    if (desc->nplanes != planeTexels.size())
        return q23::unexpected(u"Unexpected plane count: %1"_s.arg(desc->nplanes));

    for (int plane = 0; plane < desc->nplanes; ++plane) {
        const int width = desc->widthForPlane(frame.size().width(), plane);
        const int height = desc->heightForPlane(frame.size().height(), plane);
        const q23::expected<void, QString> result = QtMultimediaTest::comparePlaneRows(
                frame.bits(plane), frame.bytesPerLine(plane), width, height, planeTexels[plane]);
        if (!result)
            return q23::unexpected(u"plane %1: %2"_s.arg(plane).arg(result.error()));
    }

    frame.unmap();
    return {};
}

#endif // HWFRAMECONTEXT_P_H
