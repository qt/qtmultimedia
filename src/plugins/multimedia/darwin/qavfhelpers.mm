// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include <qavfhelpers_p.h>
#include <CoreMedia/CMFormatDescription.h>
#include <CoreVideo/CoreVideo.h>
#include <qdebug.h>

#import <CoreVideo/CoreVideo.h>

namespace {

using PixelFormat = QVideoFrameFormat::PixelFormat;
using ColorRange = QVideoFrameFormat::ColorRange;

// clang-format off
constexpr std::tuple<CvPixelFormat, PixelFormat, ColorRange> PixelFormatMap[] = {
    { kCVPixelFormatType_32ARGB, PixelFormat::Format_ARGB8888, ColorRange::ColorRange_Unknown },
    { kCVPixelFormatType_32BGRA, PixelFormat::Format_BGRA8888, ColorRange::ColorRange_Unknown },
    { kCVPixelFormatType_420YpCbCr8Planar, PixelFormat::Format_YUV420P, ColorRange::ColorRange_Unknown },
    { kCVPixelFormatType_420YpCbCr8PlanarFullRange, PixelFormat::Format_YUV420P, ColorRange::ColorRange_Full },
    { kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange, PixelFormat::Format_NV12, ColorRange::ColorRange_Video },
    { kCVPixelFormatType_420YpCbCr8BiPlanarFullRange, PixelFormat::Format_NV12, ColorRange::ColorRange_Full },
    { kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange, PixelFormat::Format_P010, ColorRange::ColorRange_Video },
    { kCVPixelFormatType_420YpCbCr10BiPlanarFullRange, PixelFormat::Format_P010, ColorRange::ColorRange_Full },
    { kCVPixelFormatType_422YpCbCr8, PixelFormat::Format_UYVY, ColorRange::ColorRange_Video },
    { kCVPixelFormatType_422YpCbCr8_yuvs, PixelFormat::Format_YUYV, ColorRange::ColorRange_Video },
    { kCVPixelFormatType_OneComponent8, PixelFormat::Format_Y8, ColorRange::ColorRange_Unknown },
    { kCVPixelFormatType_OneComponent16, PixelFormat::Format_Y16, ColorRange::ColorRange_Unknown },

    // The cases with kCMVideoCodecType_JPEG/kCMVideoCodecType_JPEG_OpenDML as cv pixel format should be investigated.
    // Matching kCMVideoCodecType_JPEG_OpenDML to ColorRange_Full is a little hack to distinguish between
    // kCMVideoCodecType_JPEG and kCMVideoCodecType_JPEG_OpenDML.
    { kCMVideoCodecType_JPEG, PixelFormat::Format_Jpeg, ColorRange::ColorRange_Unknown },
    { kCMVideoCodecType_JPEG_OpenDML, PixelFormat::Format_Jpeg, ColorRange::ColorRange_Full }
};
// clang-format on

template<typename Type, typename... Args>
Type findInPixelFormatMap(Type defaultValue, Args... args)
{
    auto checkElement = [&](const auto &element) {
        return ((args == std::get<Args>(element)) && ...);
    };

    auto found = std::find_if(std::begin(PixelFormatMap), std::end(PixelFormatMap), checkElement);
    return found == std::end(PixelFormatMap) ? defaultValue : std::get<Type>(*found);
}

}

ColorRange QAVFHelpers::colorRangeForCVPixelFormat(CvPixelFormat cvPixelFormat)
{
    return findInPixelFormatMap(ColorRange::ColorRange_Unknown, cvPixelFormat);
}

PixelFormat QAVFHelpers::fromCVPixelFormat(CvPixelFormat cvPixelFormat)
{
    return findInPixelFormatMap(PixelFormat::Format_Invalid, cvPixelFormat);
}

CvPixelFormat QAVFHelpers::toCVPixelFormat(PixelFormat pixelFmt, ColorRange colorRange)
{
    return findInPixelFormatMap(CvPixelFormatInvalid, pixelFmt, colorRange);
}

QVideoFrameFormat QAVFHelpers::videoFormatForImageBuffer(CVImageBufferRef buffer, bool openGL)
{
    auto cvPixelFormat = CVPixelBufferGetPixelFormatType(buffer);
    auto pixelFormat = fromCVPixelFormat(cvPixelFormat);
    if (openGL) {
        if (cvPixelFormat == kCVPixelFormatType_32BGRA)
            pixelFormat = QVideoFrameFormat::Format_SamplerRect;
        else
            qWarning() << "Accelerated macOS OpenGL video supports BGRA only, got CV pixel format"
                       << cvPixelFormat;
    }

    size_t width = CVPixelBufferGetWidth(buffer);
    size_t height = CVPixelBufferGetHeight(buffer);

    QVideoFrameFormat format(QSize(width, height), pixelFormat);

    auto colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    auto colorTransfer = QVideoFrameFormat::ColorTransfer_Unknown;

    if (CFStringRef cSpace = reinterpret_cast<CFStringRef>(
                CVBufferGetAttachment(buffer, kCVImageBufferYCbCrMatrixKey, nullptr))) {
        if (CFEqual(cSpace, kCVImageBufferYCbCrMatrix_ITU_R_709_2)) {
            colorSpace = QVideoFrameFormat::ColorSpace_BT709;
        } else if (CFEqual(cSpace, kCVImageBufferYCbCrMatrix_ITU_R_601_4)
                   || CFEqual(cSpace, kCVImageBufferYCbCrMatrix_SMPTE_240M_1995)) {
            colorSpace = QVideoFrameFormat::ColorSpace_BT601;
        } else if (@available(macOS 10.11, iOS 9.0, *)) {
            if (CFEqual(cSpace, kCVImageBufferYCbCrMatrix_ITU_R_2020)) {
                colorSpace = QVideoFrameFormat::ColorSpace_BT2020;
            }
        }
    }

    if (CFStringRef cTransfer = reinterpret_cast<CFStringRef>(
        CVBufferGetAttachment(buffer, kCVImageBufferTransferFunctionKey, nullptr))) {

        if (CFEqual(cTransfer, kCVImageBufferTransferFunction_ITU_R_709_2)) {
            colorTransfer = QVideoFrameFormat::ColorTransfer_BT709;
        } else if (CFEqual(cTransfer, kCVImageBufferTransferFunction_SMPTE_240M_1995)) {
            colorTransfer = QVideoFrameFormat::ColorTransfer_BT601;
        } else if (CFEqual(cTransfer, kCVImageBufferTransferFunction_sRGB)) {
            colorTransfer = QVideoFrameFormat::ColorTransfer_Gamma22;
        } else if (CFEqual(cTransfer, kCVImageBufferTransferFunction_UseGamma)) {
            auto gamma = reinterpret_cast<CFNumberRef>(
                        CVBufferGetAttachment(buffer, kCVImageBufferGammaLevelKey, nullptr));
            double g;
            CFNumberGetValue(gamma, kCFNumberFloat32Type, &g);
            // These are best fit values given what we have in our enum
            if (g < 0.8)
                ; // unknown
            else if (g < 1.5)
                colorTransfer = QVideoFrameFormat::ColorTransfer_Linear;
            else if (g < 2.1)
                colorTransfer = QVideoFrameFormat::ColorTransfer_BT709;
            else if (g < 2.5)
                colorTransfer = QVideoFrameFormat::ColorTransfer_Gamma22;
            else if (g < 3.2)
                colorTransfer = QVideoFrameFormat::ColorTransfer_Gamma28;
        }
        if (@available(macOS 10.12, iOS 11.0, *)) {
            if (CFEqual(cTransfer, kCVImageBufferTransferFunction_ITU_R_2020))
                colorTransfer = QVideoFrameFormat::ColorTransfer_BT709;
        }
        if (@available(macOS 10.12, iOS 11.0, *)) {
            if (CFEqual(cTransfer, kCVImageBufferTransferFunction_ITU_R_2100_HLG)) {
                colorTransfer = QVideoFrameFormat::ColorTransfer_STD_B67;
            } else if (CFEqual(cTransfer, kCVImageBufferTransferFunction_SMPTE_ST_2084_PQ)) {
                colorTransfer = QVideoFrameFormat::ColorTransfer_ST2084;
            }
        }
    }

    format.setColorRange(colorRangeForCVPixelFormat(cvPixelFormat));
    format.setColorSpace(colorSpace);
    format.setColorTransfer(colorTransfer);
    return format;
}
