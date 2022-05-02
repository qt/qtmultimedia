/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef QVIDEOFRAMECONVERSIONHELPER_P_H
#define QVIDEOFRAMECONVERSIONHELPER_P_H

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

#include <qvideoframe.h>
#include <private/qsimd_p.h>

QT_BEGIN_NAMESPACE

// Converts to RGB32 or ARGB32_Premultiplied
typedef void (QT_FASTCALL *VideoFrameConvertFunc)(const QVideoFrame &frame, uchar *output);

VideoFrameConvertFunc qConverterForFormat(QVideoFrameFormat::PixelFormat format);

template<int a, int r, int g, int b>
struct RgbPixel
{
    uchar data[4];
    inline quint32 convert() const
    {
        return (a >= 0 ? (uint(data[a]) << 24) : 0xff000000)
               | (uint(data[r]) << 16)
               | (uint(data[g]) << 8)
               | (uint(data[b]));
    }

};

template<typename Y>
struct YPixel
{
    Y data;
    static constexpr uint shift = (sizeof(Y) - 1)*8;
    inline quint32 convert() const
    {
        uint y = (data >> shift) & 0xff;
        return (0xff000000)
               | (y << 16)
               | (y << 8)
               | (y);
    }

};


using ARGB8888 = RgbPixel<0, 1, 2, 3>;
using ABGR8888 = RgbPixel<0, 3, 2, 1>;
using RGBA8888 = RgbPixel<3, 0, 1, 2>;
using BGRA8888 = RgbPixel<3, 2, 1, 0>;
using XRGB8888 = RgbPixel<-1, 1, 2, 3>;
using XBGR8888 = RgbPixel<-1, 3, 2, 1>;
using RGBX8888 = RgbPixel<-1, 0, 1, 2>;
using BGRX8888 = RgbPixel<-1, 2, 1, 0>;

#define FETCH_INFO_PACKED(frame) \
    const uchar *src = frame.bits(0); \
    int stride = frame.bytesPerLine(0); \
    int width = frame.width(); \
    int height = frame.height();

#define FETCH_INFO_BIPLANAR(frame) \
    const uchar *plane1 = frame.bits(0); \
    const uchar *plane2 = frame.bits(1); \
    int plane1Stride = frame.bytesPerLine(0); \
    int plane2Stride = frame.bytesPerLine(1); \
    int width = frame.width(); \
    int height = frame.height();

#define FETCH_INFO_TRIPLANAR(frame) \
    const uchar *plane1 = frame.bits(0); \
    const uchar *plane2 = frame.bits(1); \
    const uchar *plane3 = frame.bits(2); \
    int plane1Stride = frame.bytesPerLine(0); \
    int plane2Stride = frame.bytesPerLine(1); \
    int plane3Stride = frame.bytesPerLine(2); \
    int width = frame.width(); \
    int height = frame.height(); \

#define MERGE_LOOPS(width, height, stride, bpp) \
    if (stride == width * bpp) { \
        width *= height; \
        height = 1; \
        stride = 0; \
    }

#define ALIGN(boundary, ptr, x, length) \
    for (; ((reinterpret_cast<qintptr>(ptr) & (boundary - 1)) != 0) && x < length; ++x)

QT_END_NAMESPACE

#endif // QVIDEOFRAMECONVERSIONHELPER_P_H

