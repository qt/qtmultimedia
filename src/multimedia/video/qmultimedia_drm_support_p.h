// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_DRM_SUPPORT_P_H
#define QMULTIMEDIA_DRM_SUPPORT_P_H

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

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtMultimedia/qvideoframeformat.h>
#include <QtCore/qspan.h>
#include <QtCore/qstring.h>

#include <cstdint>
#include <optional>

QT_BEGIN_NAMESPACE

class QDebug;

namespace QtMultimediaPrivate {

// see DRM_FORMAT_MOD_VENDOR_* in drm_fourcc.h
enum class DRMModifier : std::uint64_t { };

static constexpr DRMModifier DmaBufFormatModifierInvalid = DRMModifier{ (1ULL << 56) - 1 };
static constexpr DRMModifier DrmFormatModifierLinear = DRMModifier{ 0 };

Q_MULTIMEDIA_EXPORT QString toString(DRMModifier modifier);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug debug, DRMModifier modifier);

constexpr uint32_t fourcc_code(char a, char b, char c, char d)
{
    return ((uint32_t)(a) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24));
}

// clang-format off
enum class DRMFormat : std::uint32_t {
    RGBA8888     = fourcc_code('R', 'A', '2', '4'), /* [31:0] R:G:B:A 8:8:8:8 little endian */
    RGB888       = fourcc_code('R', 'G', '2', '4'), /* [23:0] R:G:B little endian */
    RG88         = fourcc_code('R', 'G', '8', '8'), /* [15:0] R:G 8:8 little endian */
    ARGB8888     = fourcc_code('A', 'R', '2', '4'), /* [31:0] A:R:G:B 8:8:8:8 little endian */
    ABGR8888     = fourcc_code('A', 'B', '2', '4'), /* [31:0] A:B:G:R 8:8:8:8 little endian */
    XRGB8888     = fourcc_code('X', 'R', '2', '4'), /* [31:0] x:R:G:B 8:8:8:8 little endian */
    XBGR8888     = fourcc_code('X', 'B', '2', '4'), /* [31:0] x:B:G:R 8:8:8:8 little endian */
    BGRA8888     = fourcc_code('B', 'A', '2', '4'), /* [31:0] B:G:R:A 8:8:8:8 little endian */
    BGR888       = fourcc_code('B', 'G', '2', '4'), /* [23:0] B:G:R little endian */
    GR88         = fourcc_code('G', 'R', '8', '8'), /* [15:0] G:R 8:8 little endian */
    R8           = fourcc_code('R', '8', ' ', ' '), /* [7:0] R */
    R16          = fourcc_code('R', '1', '6', ' '), /* [15:0] R little endian */
    RGB565       = fourcc_code('R', 'G', '1', '6'), /* [15:0] R:G:B 5:6:5 little endian */
    RG1616       = fourcc_code('R', 'G', '3', '2'), /* [31:0] R:G 16:16 little endian */
    GR1616       = fourcc_code('G', 'R', '3', '2'), /* [31:0] G:R 16:16 little endian */
    BGRA1010102  = fourcc_code('B', 'A', '3', '0'), /* [31:0] B:G:R:A 10:10:10:2 little endian */
    YUYV         = fourcc_code('Y', 'U', 'Y', 'V'), /* [31:0] Cr0:Y1:Cb0:Y0 8:8:8:8 little endian */
    UYVY         = fourcc_code('U', 'Y', 'V', 'Y'), /* [31:0] Y1:Cr0:Y0:Cb0 8:8:8:8 little endian */
    AYUV         = fourcc_code('A', 'Y', 'U', 'V'), /* [31:0] A:Y:Cb:Cr 8:8:8:8 little endian */
    NV12         = fourcc_code('N', 'V', '1', '2'), /* 2x2 subsampled Cr:Cb plane */
    NV21         = fourcc_code('N', 'V', '2', '1'), /* 2x2 subsampled Cb:Cr plane */
    P010         = fourcc_code('P', '0', '1', '0'), /* 2x2 subsampled Cr:Cb plane, 10 bits per channel */
    P016         = fourcc_code('P', '0', '1', '6'), /* 2x2 subsampled Cr:Cb plane, 16 bits per channel */
    YUV411       = fourcc_code('Y', 'U', '1', '1'), /* 4x1 subsampled Cb (1) and Cr (2) planes */
    YUV420       = fourcc_code('Y', 'U', '1', '2'), /* 2x2 subsampled Cb (1) and Cr (2) planes */
    YVU420       = fourcc_code('Y', 'V', '1', '2'), /* 2x2 subsampled Cr (1) and Cb (2) planes */
    YUV422       = fourcc_code('Y', 'U', '1', '6'), /* 2x1 subsampled Cb (1) and Cr (2) planes */
    YUV444       = fourcc_code('Y', 'U', '2', '4'), /* non-subsampled Cb (1) and Cr (2) planes */
    MJPEG        = fourcc_code('M', 'J', 'P', 'G'), /* Motion-JPEG */
};
// clang-format on

Q_MULTIMEDIA_EXPORT QString toString(DRMFormat format);
Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug debug, DRMFormat format);

#if __cplusplus >= 202302L
#  define constexpr_cxx23 constexpr
#else
#  define constexpr_cxx23 inline
#endif

constexpr_cxx23 QSpan<const DRMFormat>
dmaBufFourccFromPixelFormat(const QVideoFrameFormat::PixelFormat format)
{
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    constexpr DRMFormat rgba_fourcc = DRMFormat::ABGR8888;
    constexpr DRMFormat rg_fourcc = DRMFormat::GR88;
    constexpr DRMFormat rg16_fourcc = DRMFormat::GR1616;
#else
    constexpr DRMFormat rgba_fourcc = DRMFormat::RGBA8888;
    constexpr DRMFormat rg_fourcc = DRMFormat::RG88;
    constexpr DRMFormat rg16_fourcc = DRMFormat::RG1616;
#endif

    switch (format) {
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
    case QVideoFrameFormat::Format_SamplerExternalOES:
    case QVideoFrameFormat::Format_SamplerRect:
        return {};

    case QVideoFrameFormat::Format_Jpeg: {
        static constexpr DRMFormat format[] = { DRMFormat::MJPEG };
        return format;
    }

    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV: {
        static constexpr DRMFormat format[] = { rgba_fourcc };
        return format;
    }

    case QVideoFrameFormat::Format_Y8: {
        static constexpr DRMFormat format[] = { DRMFormat::R8 };
        return format;
    }
    case QVideoFrameFormat::Format_Y16: {
        static constexpr DRMFormat format[] = { DRMFormat::R16 };
        return format;
    }

    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_YV12: {
        static constexpr DRMFormat format[] = { DRMFormat::R8, DRMFormat::R8, DRMFormat::R8 };
        return format;
    }
    case QVideoFrameFormat::Format_YUV420P10: {
        static constexpr DRMFormat format[] = { DRMFormat::R16, DRMFormat::R16, DRMFormat::R16 };
        return format;
    }

    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21: {
        static constexpr DRMFormat format[] = { DRMFormat::R8, rg_fourcc };
        return format;
    }

    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016: {
        static constexpr DRMFormat format[] = { DRMFormat::R16, rg16_fourcc };
        return format;
    }
    }
    return {};
}

#undef constexpr_cxx23

constexpr std::optional<QVideoFrameFormat::PixelFormat> pixelFormatFromDrmFourcc(DRMFormat format)
{
    switch (format) {
    case DRMFormat::NV12:
        return QVideoFrameFormat::Format_NV12;
    case DRMFormat::NV21:
        return QVideoFrameFormat::Format_NV21;
    case DRMFormat::YUV420:
        return QVideoFrameFormat::Format_YUV420P;
    case DRMFormat::YVU420:
        return QVideoFrameFormat::Format_YV12;
    case DRMFormat::YUV422:
        return QVideoFrameFormat::Format_YUV422P;
    case DRMFormat::YUYV:
        return QVideoFrameFormat::Format_YUYV;
    case DRMFormat::UYVY:
        return QVideoFrameFormat::Format_UYVY;
    case DRMFormat::XRGB8888:
        return QVideoFrameFormat::Format_XRGB8888;
    case DRMFormat::XBGR8888:
        return QVideoFrameFormat::Format_XBGR8888;
    case DRMFormat::ARGB8888:
        return QVideoFrameFormat::Format_ARGB8888;
    case DRMFormat::ABGR8888:
        return QVideoFrameFormat::Format_ABGR8888;
    case DRMFormat::R8:
        return QVideoFrameFormat::Format_Y8;
    case DRMFormat::R16:
        return QVideoFrameFormat::Format_Y16;
    case DRMFormat::P010:
        return QVideoFrameFormat::Format_P010;
    case DRMFormat::P016:
        return QVideoFrameFormat::Format_P016;
    case DRMFormat::MJPEG:
        return QVideoFrameFormat::Format_Jpeg;

    // these formats are ambiguous, because they can be premultiplied:
    case DRMFormat::RGBA8888:
        return QVideoFrameFormat::Format_RGBA8888;
    case DRMFormat::BGRA8888:
        return QVideoFrameFormat::Format_BGRA8888;
    case DRMFormat::AYUV:
        return QVideoFrameFormat::Format_AYUV;

    // No QVideoFrameFormat equivalent at all (e.g. 24bpp RGB/BGR with no alpha channel).
    case DRMFormat::RGB888:
    case DRMFormat::RG88:
    case DRMFormat::BGR888:
    case DRMFormat::GR88:
    case DRMFormat::RGB565:
    case DRMFormat::RG1616:
    case DRMFormat::GR1616:
    case DRMFormat::BGRA1010102:
    case DRMFormat::YUV411:
    case DRMFormat::YUV444:
        return std::nullopt;
    }
    return std::nullopt;
}

struct DmaBufPlane
{
    int fd = -1;
    uint32_t offset = 0;
    uint32_t pitch = 0;
    DRMFormat drmFormat = DRMFormat::RGBA8888;
    DRMModifier modifier = DmaBufFormatModifierInvalid;
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif
