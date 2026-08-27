// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpipewire_videoformat_support_p.h"

#include <algorithm>
#include <cmath>

QT_BEGIN_NAMESPACE

namespace QtPipeWire {

FrameRate rateFromFps(qreal fps)
{
    // NTSC rates get special handling if it seems like the user wants them
    const std::initializer_list<FrameRate> ntscRates = {
        { 23.976, SPA_FRACTION(24'000, 1'001) },
        { 29.97, SPA_FRACTION(30'000, 1'001) },
        { 59.94, SPA_FRACTION(60'000, 1'001) },
    };
    // NOTE: One could assume that a requested 60.f mapped to 60000/1001.
    // TODO: There's also other rates (59.97, 49.99, etc..)

    for (const FrameRate &rate : ntscRates) {
        constexpr float precision = 0.01f;
        if (std::abs(fps - rate.fps) < precision)
            return rate;
    }

    // By default, round to a whole number fps. Rounding down is safest,
    // but avoid 0/1, which isn't accepted as a maximum rate
    quint32 roundedFps = std::max(std::floor(fps), 1.);
    return FrameRate{
        .fps = qreal(roundedFps),
        .frac = SPA_FRACTION(roundedFps, 1),
    };
}

QVideoFrameFormat::PixelFormat toQtPixelFormat(spa_video_format spaVideoFormat)
{
    switch (spaVideoFormat) {
    default:
        break;
    case SPA_VIDEO_FORMAT_I420:
        return QVideoFrameFormat::Format_YUV420P;
    case SPA_VIDEO_FORMAT_Y42B:
        return QVideoFrameFormat::Format_YUV422P;
    case SPA_VIDEO_FORMAT_YV12:
        return QVideoFrameFormat::Format_YV12;
    case SPA_VIDEO_FORMAT_UYVY:
        return QVideoFrameFormat::Format_UYVY;
    case SPA_VIDEO_FORMAT_YUY2:
        return QVideoFrameFormat::Format_YUYV;
    case SPA_VIDEO_FORMAT_NV12:
        return QVideoFrameFormat::Format_NV12;
    case SPA_VIDEO_FORMAT_NV21:
        return QVideoFrameFormat::Format_NV21;
    case SPA_VIDEO_FORMAT_AYUV:
        return QVideoFrameFormat::Format_AYUV;
    case SPA_VIDEO_FORMAT_GRAY8:
        return QVideoFrameFormat::Format_Y8;
    case SPA_VIDEO_FORMAT_xRGB:
        return QVideoFrameFormat::Format_XRGB8888;
    case SPA_VIDEO_FORMAT_xBGR:
        return QVideoFrameFormat::Format_XBGR8888;
    case SPA_VIDEO_FORMAT_RGBx:
        return QVideoFrameFormat::Format_RGBX8888;
    case SPA_VIDEO_FORMAT_BGRx:
        return QVideoFrameFormat::Format_BGRX8888;
    case SPA_VIDEO_FORMAT_ARGB:
        return QVideoFrameFormat::Format_ARGB8888;
    case SPA_VIDEO_FORMAT_ABGR:
        return QVideoFrameFormat::Format_ABGR8888;
    case SPA_VIDEO_FORMAT_RGBA:
        return QVideoFrameFormat::Format_RGBA8888;
    case SPA_VIDEO_FORMAT_BGRA:
        return QVideoFrameFormat::Format_BGRA8888;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    case SPA_VIDEO_FORMAT_GRAY16_LE:
        return QVideoFrameFormat::Format_Y16;
    case SPA_VIDEO_FORMAT_P010_10LE:
        return QVideoFrameFormat::Format_P010;
#else
    case SPA_VIDEO_FORMAT_GRAY16_BE:
        return QVideoFrameFormat::Format_Y16;
    case SPA_VIDEO_FORMAT_P010_10BE:
        return QVideoFrameFormat::Format_P010;
#endif
    }

    return QVideoFrameFormat::Format_Invalid;
}

spa_video_format toSpaVideoFormat(QVideoFrameFormat::PixelFormat pixelFormat)
{
    switch (pixelFormat) {
    default:
        break;
    case QVideoFrameFormat::Format_YUV420P:
        return SPA_VIDEO_FORMAT_I420;
    case QVideoFrameFormat::Format_YUV422P:
        return SPA_VIDEO_FORMAT_Y42B;
    case QVideoFrameFormat::Format_YV12:
        return SPA_VIDEO_FORMAT_YV12;
    case QVideoFrameFormat::Format_UYVY:
        return SPA_VIDEO_FORMAT_UYVY;
    case QVideoFrameFormat::Format_YUYV:
        return SPA_VIDEO_FORMAT_YUY2;
    case QVideoFrameFormat::Format_NV12:
        return SPA_VIDEO_FORMAT_NV12;
    case QVideoFrameFormat::Format_NV21:
        return SPA_VIDEO_FORMAT_NV21;
    case QVideoFrameFormat::Format_AYUV:
        return SPA_VIDEO_FORMAT_AYUV;
    case QVideoFrameFormat::Format_Y8:
        return SPA_VIDEO_FORMAT_GRAY8;
    case QVideoFrameFormat::Format_XRGB8888:
        return SPA_VIDEO_FORMAT_xRGB;
    case QVideoFrameFormat::Format_XBGR8888:
        return SPA_VIDEO_FORMAT_xBGR;
    case QVideoFrameFormat::Format_RGBX8888:
        return SPA_VIDEO_FORMAT_RGBx;
    case QVideoFrameFormat::Format_BGRX8888:
        return SPA_VIDEO_FORMAT_BGRx;
    case QVideoFrameFormat::Format_ARGB8888:
        return SPA_VIDEO_FORMAT_ARGB;
    case QVideoFrameFormat::Format_ABGR8888:
        return SPA_VIDEO_FORMAT_ABGR;
    case QVideoFrameFormat::Format_RGBA8888:
        return SPA_VIDEO_FORMAT_RGBA;
    case QVideoFrameFormat::Format_BGRA8888:
        return SPA_VIDEO_FORMAT_BGRA;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    case QVideoFrameFormat::Format_Y16:
        return SPA_VIDEO_FORMAT_GRAY16_LE;
    case QVideoFrameFormat::Format_P010:
        return SPA_VIDEO_FORMAT_P010_10LE;
#else
    case QVideoFrameFormat::Format_Y16:
        return SPA_VIDEO_FORMAT_GRAY16_BE;
    case QVideoFrameFormat::Format_P010:
        return SPA_VIDEO_FORMAT_P010_10BE;
#endif
    }

    return SPA_VIDEO_FORMAT_UNKNOWN;
}

} // namespace QtPipeWire

QT_END_NAMESPACE
