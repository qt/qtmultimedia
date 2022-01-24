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
