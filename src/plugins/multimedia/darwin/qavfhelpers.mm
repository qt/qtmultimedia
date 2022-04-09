/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qavfhelpers_p.h>
#include <CoreMedia/CMFormatDescription.h>
#include <CoreVideo/CoreVideo.h>
#include <qdebug.h>

#import <CoreVideo/CoreVideo.h>

QVideoFrameFormat::PixelFormat QAVFHelpers::fromCVPixelFormat(unsigned avPixelFormat)
{
    switch (avPixelFormat) {
    case kCVPixelFormatType_32ARGB:
        return QVideoFrameFormat::Format_ARGB8888;
    case kCVPixelFormatType_32BGRA:
        return QVideoFrameFormat::Format_BGRA8888;
    case kCVPixelFormatType_420YpCbCr8Planar:
    case kCVPixelFormatType_420YpCbCr8PlanarFullRange:
        return QVideoFrameFormat::Format_YUV420P;
    case kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr8BiPlanarFullRange:
        return QVideoFrameFormat::Format_NV12;
    case kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange:
    case kCVPixelFormatType_420YpCbCr10BiPlanarFullRange:
        return QVideoFrameFormat::Format_P010;
    case kCVPixelFormatType_422YpCbCr8:
        return QVideoFrameFormat::Format_UYVY;
    case kCVPixelFormatType_422YpCbCr8_yuvs:
        return QVideoFrameFormat::Format_YUYV;
    case kCVPixelFormatType_OneComponent8:
        return QVideoFrameFormat::Format_Y8;
    case q_kCVPixelFormatType_OneComponent16:
        return QVideoFrameFormat::Format_Y16;

    case kCMVideoCodecType_JPEG:
    case kCMVideoCodecType_JPEG_OpenDML:
        return QVideoFrameFormat::Format_Jpeg;
    default:
        return QVideoFrameFormat::Format_Invalid;
    }
}

bool QAVFHelpers::toCVPixelFormat(QVideoFrameFormat::PixelFormat qtFormat, unsigned &conv)
{
    switch (qtFormat) {
    case QVideoFrameFormat::Format_ARGB8888:
        conv = kCVPixelFormatType_32ARGB;
        break;
    case QVideoFrameFormat::Format_BGRA8888:
        conv = kCVPixelFormatType_32BGRA;
        break;
    case QVideoFrameFormat::Format_YUV420P:
        conv = kCVPixelFormatType_420YpCbCr8PlanarFullRange;
        break;
    case QVideoFrameFormat::Format_NV12:
        conv = kCVPixelFormatType_420YpCbCr8BiPlanarFullRange;
        break;
    case QVideoFrameFormat::Format_P010:
        conv = kCVPixelFormatType_420YpCbCr10BiPlanarFullRange;
        break;
    case QVideoFrameFormat::Format_UYVY:
        conv = kCVPixelFormatType_422YpCbCr8;
        break;
    case QVideoFrameFormat::Format_YUYV:
        conv = kCVPixelFormatType_422YpCbCr8_yuvs;
        break;
    case QVideoFrameFormat::Format_Y8:
        conv = kCVPixelFormatType_OneComponent8;
        break;
    case QVideoFrameFormat::Format_Y16:
        conv = q_kCVPixelFormatType_OneComponent16;
        break;
    default:
        return false;
    }

    return true;
}

QVideoFrameFormat QAVFHelpers::videoFormatForImageBuffer(CVImageBufferRef buffer, bool openGL)
{
    auto avPixelFormat = CVPixelBufferGetPixelFormatType(buffer);
    if (openGL) {
        if (avPixelFormat == kCVPixelFormatType_32BGRA)
            avPixelFormat = QVideoFrameFormat::Format_SamplerRect;
        else
            qWarning() << "Accelerated macOS OpenGL video supports BGRA only, got CV pixel format" << avPixelFormat;
    }
    auto pixelFormat = fromCVPixelFormat(avPixelFormat);

    size_t width = CVPixelBufferGetWidth(buffer);
    size_t height = CVPixelBufferGetHeight(buffer);

    QVideoFrameFormat format(QSize(width, height), pixelFormat);

    auto colorSpace = QVideoFrameFormat::ColorSpace_Undefined;
    auto colorTransfer = QVideoFrameFormat::ColorTransfer_Unknown;

    CFStringRef cSpace = reinterpret_cast<CFStringRef>(
                CVBufferGetAttachment(buffer, kCVImageBufferYCbCrMatrixKey, nullptr));
    if (CFEqual(cSpace, kCVImageBufferYCbCrMatrix_ITU_R_709_2)) {
        colorSpace = QVideoFrameFormat::ColorSpace_BT709;
    } else if (CFEqual(cSpace, kCVImageBufferYCbCrMatrix_ITU_R_601_4) ||
               CFEqual(cSpace, kCVImageBufferYCbCrMatrix_SMPTE_240M_1995)) {
        colorSpace = QVideoFrameFormat::ColorSpace_BT601;
    } else if (@available(macOS 10.11, iOS 9.0, *)) {
        if (CFEqual(cSpace, kCVImageBufferYCbCrMatrix_ITU_R_2020)) {
            colorSpace = QVideoFrameFormat::ColorSpace_BT2020;
        }
    }

    CFStringRef cTransfer = reinterpret_cast<CFStringRef>(
                CVBufferGetAttachment(buffer, kCVImageBufferTransferFunctionKey, nullptr));

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

    format.setColorSpace(colorSpace);
    format.setColorTransfer(colorTransfer);
    return format;
}
