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

#include <QtMultimedia/qvideoframeformat.h>
#include <QtCore/qspan.h>

#include <cstdint>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

static constexpr uint64_t DmaBufFormatModifierInvalid = ((1ULL << 56) - 1);

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
    YUV411       = fourcc_code('Y', 'U', '1', '1'), /* 4x1 subsampled Cb (1) and Cr (2) planes */
    YUV420       = fourcc_code('Y', 'U', '1', '2'), /* 2x2 subsampled Cb (1) and Cr (2) planes */
    YVU420       = fourcc_code('Y', 'V', '1', '2'), /* 2x2 subsampled Cr (1) and Cb (2) planes */
    YUV422       = fourcc_code('Y', 'U', '1', '6'), /* 2x1 subsampled Cb (1) and Cr (2) planes */
    YUV444       = fourcc_code('Y', 'U', '2', '4'), /* non-subsampled Cb (1) and Cr (2) planes */
};
// clang-format on

inline QSpan<const DRMFormat>
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
    case QVideoFrameFormat::Format_Jpeg:
    case QVideoFrameFormat::Format_SamplerRect:
        return {};

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

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif
