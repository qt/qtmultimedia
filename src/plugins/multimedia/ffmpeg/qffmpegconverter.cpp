// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegconverter_p.h"
#include "qffmpeg_p.h"
#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/qvideoframe.h>
#include <QtCore/qloggingcategory.h>
#include <private/qvideotexturehelper_p.h>

extern "C" {
#include <libswscale/swscale.h>
}

QT_BEGIN_NAMESPACE

namespace {

Q_LOGGING_CATEGORY(lc, "qt.multimedia.ffmpeg.converter");


// Converts to FFmpeg pixel format. This function differs from
// QFFmpegVideoBuffer::toAVPixelFormat which only covers the subset
// of pixel formats required for encoding. Here we need to cover more
// pixel formats to be able to generate test images for decoding/display
AVPixelFormat toAVPixelFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
    default:
    case QVideoFrameFormat::Format_Invalid:
        return AV_PIX_FMT_NONE;
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
        return AV_PIX_FMT_NONE; // TODO: Fixme (No corresponding FFmpeg format available)
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC4:
        return AV_PIX_FMT_YUV420P;
    case QVideoFrameFormat::Format_Jpeg:
        return AV_PIX_FMT_BGRA;
    case QVideoFrameFormat::Format_ARGB8888:
        return AV_PIX_FMT_ARGB;
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
        return AV_PIX_FMT_0RGB;
    case QVideoFrameFormat::Format_BGRA8888:
        return AV_PIX_FMT_BGRA;
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
        return AV_PIX_FMT_BGR0;
    case QVideoFrameFormat::Format_ABGR8888:
        return AV_PIX_FMT_ABGR;
    case QVideoFrameFormat::Format_XBGR8888:
        return AV_PIX_FMT_0BGR;
    case QVideoFrameFormat::Format_RGBA8888:
        return AV_PIX_FMT_RGBA;
    case QVideoFrameFormat::Format_RGBX8888:
        return AV_PIX_FMT_RGB0;
    case QVideoFrameFormat::Format_YUV422P:
        return AV_PIX_FMT_YUV422P;
    case QVideoFrameFormat::Format_YUV420P:
        return AV_PIX_FMT_YUV420P;
    case QVideoFrameFormat::Format_YUV420P10:
        return AV_PIX_FMT_YUV420P10;
    case QVideoFrameFormat::Format_UYVY:
        return AV_PIX_FMT_UYVY422;
    case QVideoFrameFormat::Format_YUYV:
        return AV_PIX_FMT_YUYV422;
    case QVideoFrameFormat::Format_NV12:
        return AV_PIX_FMT_NV12;
    case QVideoFrameFormat::Format_NV21:
        return AV_PIX_FMT_NV21;
    case QVideoFrameFormat::Format_Y8:
        return AV_PIX_FMT_GRAY8;
    case QVideoFrameFormat::Format_Y16:
        return AV_PIX_FMT_GRAY16;
    case QVideoFrameFormat::Format_P010:
        return AV_PIX_FMT_P010;
    case QVideoFrameFormat::Format_P016:
        return AV_PIX_FMT_P016;
    case QVideoFrameFormat::Format_SamplerExternalOES:
        return AV_PIX_FMT_MEDIACODEC;
    }
}

struct SwsFrameData
{
    static constexpr int arraySize = 4; // Array size required by sws_scale
    std::array<uchar *, arraySize> bits;
    std::array<int, arraySize> stride;
};

SwsFrameData getSwsData(QVideoFrame &dst)
{
    switch (dst.pixelFormat()) {
    case QVideoFrameFormat::Format_YV12:
    case QVideoFrameFormat::Format_IMC1:
        return { { dst.bits(0), dst.bits(2), dst.bits(1), nullptr },
                 { dst.bytesPerLine(0), dst.bytesPerLine(2), dst.bytesPerLine(1), 0 } };

    case QVideoFrameFormat::Format_IMC2:
        return { { dst.bits(0), dst.bits(1) + dst.bytesPerLine(1) / 2, dst.bits(1), nullptr },
                 { dst.bytesPerLine(0), dst.bytesPerLine(1), dst.bytesPerLine(1), 0 } };

    case QVideoFrameFormat::Format_IMC4:
        return { { dst.bits(0), dst.bits(1), dst.bits(1) + dst.bytesPerLine(1) / 2, nullptr },
                 { dst.bytesPerLine(0), dst.bytesPerLine(1), dst.bytesPerLine(1), 0 } };
    default:
        return { { dst.bits(0), dst.bits(1), dst.bits(2), nullptr },
                 { dst.bytesPerLine(0), dst.bytesPerLine(1), dst.bytesPerLine(2), 0 } };
    }
}

struct SwsColorSpace
{
    int colorSpace;
    int colorRange; // 0 - mpeg/video, 1 - jpeg/full
};

// Qt heuristics for determining color space requires checking
// both frame color space and range. This function mimics logic
// used elsewhere in Qt Multimedia.
SwsColorSpace toSwsColorSpace(QVideoFrameFormat::ColorRange colorRange,
                              QVideoFrameFormat::ColorSpace colorSpace)
{
    const int avRange = colorRange == QVideoFrameFormat::ColorRange_Video ? 0 : 1;

    switch (colorSpace) {
    case QVideoFrameFormat::ColorSpace_BT601:
        if (colorRange == QVideoFrameFormat::ColorRange_Full)
            return { SWS_CS_ITU709, 1 }; // TODO: FIXME - Not exact match
        return { SWS_CS_ITU601, 0 };
    case QVideoFrameFormat::ColorSpace_BT709:
        return { SWS_CS_ITU709, avRange };
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        return { SWS_CS_ITU601, 1 }; // TODO: Why do ITU601 and Adobe RGB match well?
    case QVideoFrameFormat::ColorSpace_BT2020:
        return { SWS_CS_BT2020, avRange };
    case QVideoFrameFormat::ColorSpace_Undefined:
    default:
        return { SWS_CS_DEFAULT, avRange };
    }
}

using PixelFormat = QVideoFrameFormat::PixelFormat;

// clang-format off

QFFmpeg::SwsContextUPtr createConverter(const QSize &srcSize, PixelFormat srcPixFmt,
                               const QSize &dstSize, PixelFormat dstPixFmt)
{
    return QFFmpeg::createSwsContext(srcSize, toAVPixelFormat(srcPixFmt), dstSize, toAVPixelFormat(dstPixFmt), SWS_BILINEAR);
}

bool setColorSpaceDetails(SwsContext *context,
                          const QVideoFrameFormat &srcFormat,
                          const QVideoFrameFormat &dstFormat)
{
    const SwsColorSpace src = toSwsColorSpace(srcFormat.colorRange(), srcFormat.colorSpace());
    const SwsColorSpace dst = toSwsColorSpace(dstFormat.colorRange(), dstFormat.colorSpace());

    constexpr int brightness = 0;
    constexpr int contrast = 0;
    constexpr int saturation = 0;
    const int status = sws_setColorspaceDetails(context,
        sws_getCoefficients(src.colorSpace), src.colorRange,
        sws_getCoefficients(dst.colorSpace), dst.colorRange,
        brightness, contrast, saturation);

    return status == 0;
}

bool convert(SwsContext *context, QVideoFrame &src, int srcHeight, QVideoFrame &dst)
{
    if (!src.map(QVideoFrame::ReadOnly))
        return false;

    QScopeGuard unmapSrc{[&] {
        src.unmap();
    }};

    if (!dst.map(QVideoFrame::WriteOnly))
        return false;

    QScopeGuard unmapDst{[&] {
        dst.unmap();
    }};

    const SwsFrameData srcData = getSwsData(src);
    const SwsFrameData dstData = getSwsData(dst);

    constexpr int firstSrcSliceRow = 0;
    const int scaledHeight = sws_scale(context,
        srcData.bits.data(), srcData.stride.data(),
        firstSrcSliceRow, srcHeight,
        dstData.bits.data(), dstData.stride.data());

    if (scaledHeight != srcHeight)
        return false;

    return true;
}

// Ensure even size if using planar format with chroma subsampling
QSize adjustSize(const QSize& size, PixelFormat srcFmt, PixelFormat dstFmt)
{
    const auto* srcDesc = QVideoTextureHelper::textureDescription(srcFmt);
    const auto* dstDesc = QVideoTextureHelper::textureDescription(dstFmt);

    QSize output = size;
    for (const auto desc : { srcDesc, dstDesc }) {
        for (int i = 0; i < desc->nplanes; ++i) {
            // TODO: Assumes that max subsampling is 2
            if (desc->sizeScale[i].x != 1)
                output.setWidth(output.width() & ~1); // Make even

            if (desc->sizeScale[i].y != 1)
                output.setHeight(output.height() & ~1); // Make even
        }
    }

    return output;
}

} // namespace

// Converts a video frame to the dstFormat video frame format.
QVideoFrame convertFrame(QVideoFrame &src, const QVideoFrameFormat &dstFormat)
{
    if (src.size() != dstFormat.frameSize()) {
        qCCritical(lc) << "Resizing is not supported";
        return {};
    }

    // Adjust size to even width/height if we have chroma subsampling
    const QSize size = adjustSize(src.size(), src.pixelFormat(), dstFormat.pixelFormat());
    if (size != src.size())
        qCWarning(lc) << "Input truncated to even width/height";

    const QFFmpeg::SwsContextUPtr conv = createConverter(
        size, src.pixelFormat(), size, dstFormat.pixelFormat());

    if (!conv) {
        qCCritical(lc) << "Failed to create SW converter";
        return {};
    }

    if (!setColorSpaceDetails(conv.get(), src.surfaceFormat(), dstFormat)) {
        qCCritical(lc) << "Failed to set color space details";
        return {};
    }

    QVideoFrame dst{ dstFormat };

    if (!convert(conv.get(), src, size.height(), dst)) {
        qCCritical(lc) << "Frame conversion failed";
        return {};
    }

    return dst;
}

// clang-format on

QT_END_NAMESPACE
