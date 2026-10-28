// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4L2FORMATS_P_H
#define QV4L2FORMATS_P_H

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

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <linux/videodev2.h>

#include <cstdint>
#include <iterator>
#include <optional>

QT_BEGIN_NAMESPACE

namespace QtV4L2 {

struct V4L2FormatMapEntry
{
    QVideoFrameFormat::PixelFormat format;
    uint32_t v4l2Format;
};

inline constexpr V4L2FormatMapEntry formatMap[] = {
    { QVideoFrameFormat::Format_YUV420P, V4L2_PIX_FMT_YUV420 },
    { QVideoFrameFormat::Format_YUV422P, V4L2_PIX_FMT_YUV422P },
    { QVideoFrameFormat::Format_YUYV, V4L2_PIX_FMT_YUYV },
    { QVideoFrameFormat::Format_UYVY, V4L2_PIX_FMT_UYVY },
    { QVideoFrameFormat::Format_XBGR8888, V4L2_PIX_FMT_XBGR32 },
    { QVideoFrameFormat::Format_XRGB8888, V4L2_PIX_FMT_XRGB32 },
    { QVideoFrameFormat::Format_ABGR8888, V4L2_PIX_FMT_ABGR32 },
    { QVideoFrameFormat::Format_ARGB8888, V4L2_PIX_FMT_ARGB32 },
    { QVideoFrameFormat::Format_BGRX8888, V4L2_PIX_FMT_BGR32 },
    { QVideoFrameFormat::Format_RGBX8888, V4L2_PIX_FMT_RGB32 },
    { QVideoFrameFormat::Format_BGRA8888, V4L2_PIX_FMT_BGRA32 },
    { QVideoFrameFormat::Format_RGBA8888, V4L2_PIX_FMT_RGBA32 },
    { QVideoFrameFormat::Format_Y8, V4L2_PIX_FMT_GREY },
    { QVideoFrameFormat::Format_Y16, V4L2_PIX_FMT_Y16 },
    { QVideoFrameFormat::Format_NV12, V4L2_PIX_FMT_NV12 },
    { QVideoFrameFormat::Format_NV21, V4L2_PIX_FMT_NV21 },

    // FIXME: V4L2_PIX_FMT_MJPEG and V4L2_PIX_FMT_JPEG both map to QVideoFrameFormat::Format_Jpeg
    { QVideoFrameFormat::Format_Jpeg, V4L2_PIX_FMT_MJPEG },
    { QVideoFrameFormat::Format_Jpeg, V4L2_PIX_FMT_JPEG },
};

inline QVideoFrameFormat::PixelFormat formatForV4L2Format(uint32_t v4l2Format)
{
    namespace ranges = QtMultimediaPrivate::ranges;
    const auto found = ranges::find_if(formatMap, [&](const V4L2FormatMapEntry &entry) {
        return entry.v4l2Format == v4l2Format;
    });
    if (found == std::end(formatMap))
        return QVideoFrameFormat::Format_Invalid;
    return found->format;
}

inline std::optional<uint32_t> v4l2FormatForPixelFormat(QVideoFrameFormat::PixelFormat format)
{
    namespace ranges = QtMultimediaPrivate::ranges;
    const auto found = ranges::find_if(formatMap, [&](const V4L2FormatMapEntry &entry) {
        return entry.format == format;
    });
    if (found == std::end(formatMap))
        return std::nullopt;
    return found->v4l2Format;
}

inline QVideoFrameFormat::ColorSpace colorSpaceForV4L2Colorspace(v4l2_colorspace colorspace)
{
    switch (colorspace) {
    case V4L2_COLORSPACE_REC709:
        return QVideoFrameFormat::ColorSpace_BT709;
    case V4L2_COLORSPACE_JPEG:
        return QVideoFrameFormat::ColorSpace_AdobeRgb;
    case V4L2_COLORSPACE_SRGB:
        // ##### is this correct???
        return QVideoFrameFormat::ColorSpace_BT601;
    case V4L2_COLORSPACE_BT2020:
        return QVideoFrameFormat::ColorSpace_BT2020;
    default:
    case V4L2_COLORSPACE_DCI_P3:
        return QVideoFrameFormat::ColorSpace_Undefined;
    }
}

inline std::optional<v4l2_colorspace>
v4l2ColorspaceForColorSpace(QVideoFrameFormat::ColorSpace colorSpace)
{
    switch (colorSpace) {
    case QVideoFrameFormat::ColorSpace_BT601:
        return V4L2_COLORSPACE_SMPTE170M;
    case QVideoFrameFormat::ColorSpace_BT709:
        return V4L2_COLORSPACE_REC709;
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
        return V4L2_COLORSPACE_OPRGB;
    case QVideoFrameFormat::ColorSpace_BT2020:
        return V4L2_COLORSPACE_BT2020;
    case QVideoFrameFormat::ColorSpace_Undefined:
        return std::nullopt;
    }
    return std::nullopt;
}

inline QSize sizeForV4L2Format(const v4l2_pix_format &pixFormat)
{
    return QSize{
        int(pixFormat.width),
        int(pixFormat.height),
    };
}

} // namespace QtV4L2

QT_END_NAMESPACE

#endif // QV4L2FORMATS_P_H
