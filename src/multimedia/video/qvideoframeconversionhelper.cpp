/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qvideoframeconversionhelper_p.h"
#include "qrgb.h"

QT_BEGIN_NAMESPACE

#define CLAMP(n) (n > 255 ? 255 : (n < 0 ? 0 : n))

#define EXPAND_UV(u, v) \
    int uu = u - 128; \
    int vv = v - 128; \
    int rv = 409 * vv + 128; \
    int guv = 100 * uu + 208 * vv + 128; \
    int bu = 516 * uu + 128; \

static inline quint32 qYUVToARGB32(int y, int rv, int guv, int bu, int a = 0xff)
{
    int yy = (y - 16) * 298;
    return (a << 24)
            | CLAMP((yy + rv) >> 8) << 16
            | CLAMP((yy - guv) >> 8) << 8
            | CLAMP((yy + bu) >> 8);
}

static inline void planarYUV420_to_ARGB32(const uchar *y, int yStride,
                                          const uchar *u, int uStride,
                                          const uchar *v, int vStride,
                                          int uvPixelStride,
                                          quint32 *rgb,
                                          int width, int height)
{
    quint32 *rgb0 = rgb;
    quint32 *rgb1 = rgb + width;

    for (int j = 0; j < height; j += 2) {
        const uchar *lineY0 = y;
        const uchar *lineY1 = y + yStride;
        const uchar *lineU = u;
        const uchar *lineV = v;

        for (int i = 0; i < width; i += 2) {
            EXPAND_UV(*lineU, *lineV);
            lineU += uvPixelStride;
            lineV += uvPixelStride;

            *rgb0++ = qYUVToARGB32(*lineY0++, rv, guv, bu);
            *rgb0++ = qYUVToARGB32(*lineY0++, rv, guv, bu);
            *rgb1++ = qYUVToARGB32(*lineY1++, rv, guv, bu);
            *rgb1++ = qYUVToARGB32(*lineY1++, rv, guv, bu);
        }

        y += yStride << 1; // stride * 2
        u += uStride;
        v += vStride;
        rgb0 += width;
        rgb1 += width;
    }
}

static inline void planarYUV422_to_ARGB32(const uchar *y, int yStride,
                                          const uchar *u, int uStride,
                                          const uchar *v, int vStride,
                                          int uvPixelStride,
                                          quint32 *rgb,
                                          int width, int height)
{
    quint32 *rgb0 = rgb;
    quint32 *rgb1 = rgb + width;

    for (int j = 0; j < height; ++j) {
        const uchar *lineY0 = y;
        const uchar *lineU = u;
        const uchar *lineV = v;

        for (int i = 0; i < width; i += 2) {
            EXPAND_UV(*lineU, *lineV);
            lineU += uvPixelStride;
            lineV += uvPixelStride;

            *rgb0++ = qYUVToARGB32(*lineY0++, rv, guv, bu);
            *rgb0++ = qYUVToARGB32(*lineY0++, rv, guv, bu);
        }

        y += yStride << 1; // stride * 2
        u += uStride;
        v += vStride;
        rgb0 += width;
        rgb1 += width;
    }
}



static void QT_FASTCALL qt_convert_YUV420P_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_TRIPLANAR(frame)
    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane2, plane2Stride,
                           plane3, plane3Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_YUV422P_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_TRIPLANAR(frame)
    planarYUV422_to_ARGB32(plane1, plane1Stride,
                           plane2, plane2Stride,
                           plane3, plane3Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}


static void QT_FASTCALL qt_convert_YV12_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_TRIPLANAR(frame)
    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane3, plane3Stride,
                           plane2, plane2Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_AYUV_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *rgb = reinterpret_cast<quint32*>(output);

    for (int i = 0; i < height; ++i) {
        const uchar *lineSrc = src;

        for (int j = 0; j < width; ++j) {
            int a = *lineSrc++;
            int y = *lineSrc++;
            int u = *lineSrc++;
            int v = *lineSrc++;

            EXPAND_UV(u, v);

            *rgb++ = qPremultiply(qYUVToARGB32(y, rv, guv, bu, a));
        }

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_AYUV_Premultiplied_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *rgb = reinterpret_cast<quint32*>(output);

    for (int i = 0; i < height; ++i) {
        const uchar *lineSrc = src;

        for (int j = 0; j < width; ++j) {
            int a = *lineSrc++;
            int y = *lineSrc++;
            int u = *lineSrc++;
            int v = *lineSrc++;

            EXPAND_UV(u, v);

            *rgb++ = qYUVToARGB32(y, rv, guv, bu, a);
        }

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_UYVY_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 2)

    quint32 *rgb = reinterpret_cast<quint32*>(output);

    for (int i = 0; i < height; ++i) {
        const uchar *lineSrc = src;

        for (int j = 0; j < width; j += 2) {
            int u = *lineSrc++;
            int y0 = *lineSrc++;
            int v = *lineSrc++;
            int y1 = *lineSrc++;

            EXPAND_UV(u, v);

            *rgb++ = qYUVToARGB32(y0, rv, guv, bu);
            *rgb++ = qYUVToARGB32(y1, rv, guv, bu);
        }

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_YUYV_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 2)

    quint32 *rgb = reinterpret_cast<quint32*>(output);

    for (int i = 0; i < height; ++i) {
        const uchar *lineSrc = src;

        for (int j = 0; j < width; j += 2) {
            int y0 = *lineSrc++;
            int u = *lineSrc++;
            int y1 = *lineSrc++;
            int v = *lineSrc++;

            EXPAND_UV(u, v);

            *rgb++ = qYUVToARGB32(y0, rv, guv, bu);
            *rgb++ = qYUVToARGB32(y1, rv, guv, bu);
        }

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_NV12_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_BIPLANAR(frame)
    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane2, plane2Stride,
                           plane2 + 1, plane2Stride,
                           2,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_NV21_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_BIPLANAR(frame)
    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane2 + 1, plane2Stride,
                           plane2, plane2Stride,
                           2,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_IMC1_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_TRIPLANAR(frame)
    Q_ASSERT(plane1Stride == plane2Stride);
    Q_ASSERT(plane1Stride == plane3Stride);

    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane3, plane3Stride,
                           plane2, plane2Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_IMC2_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_BIPLANAR(frame)
    Q_ASSERT(plane1Stride == plane2Stride);

    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane2 + (plane1Stride >> 1), plane1Stride,
                           plane2, plane1Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_IMC3_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_TRIPLANAR(frame)
    Q_ASSERT(plane1Stride == plane2Stride);
    Q_ASSERT(plane1Stride == plane3Stride);

    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane2, plane2Stride,
                           plane3, plane3Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}

static void QT_FASTCALL qt_convert_IMC4_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_BIPLANAR(frame)
    Q_ASSERT(plane1Stride == plane2Stride);

    planarYUV420_to_ARGB32(plane1, plane1Stride,
                           plane2, plane1Stride,
                           plane2 + (plane1Stride >> 1), plane1Stride,
                           1,
                           reinterpret_cast<quint32*>(output),
                           width, height);
}


template<typename Pixel>
static void QT_FASTCALL qt_convert_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const Pixel *data = reinterpret_cast<const Pixel *>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = qPremultiply(data->convert());
            ++data;
            *argb++ = qPremultiply(data->convert());
            ++data;
            *argb++ = qPremultiply(data->convert());
            ++data;
            *argb++ = qPremultiply(data->convert());
            ++data;
        }

        // leftovers
        for (; x < width; ++x) {
            *argb++ = qPremultiply(data->convert());
            ++data;
        }

        src += stride;
    }
}

template<typename Pixel>
static void QT_FASTCALL qt_convert_premultiplied_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const Pixel *data = reinterpret_cast<const Pixel *>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = data->convert();
            ++data;
            *argb++ = data->convert();
            ++data;
            *argb++ = data->convert();
            ++data;
            *argb++ = data->convert();
            ++data;
        }

        // leftovers
        for (; x < width; ++x) {
            *argb++ = data->convert();
            ++data;
        }

        src += stride;
    }
}

static inline void planarYUV420_16bit_to_ARGB32(const uchar *y, int yStride,
                                                  const uchar *u, int uStride,
                                                  const uchar *v, int vStride,
                                                  int uvPixelStride,
                                          quint32 *rgb,
                                          int width, int height)
{
    quint32 *rgb0 = rgb;
    quint32 *rgb1 = rgb + width;

    for (int j = 0; j < height; j += 2) {
        const uchar *lineY0 = y;
        const uchar *lineY1 = y + yStride;
        const uchar *lineU = u;
        const uchar *lineV = v;

        for (int i = 0; i < width; i += 2) {
            EXPAND_UV(*lineU, *lineV);
            lineU += uvPixelStride;
            lineV += uvPixelStride;

            *rgb0++ = qYUVToARGB32(*lineY0, rv, guv, bu);
            lineY0 += 2;
            *rgb0++ = qYUVToARGB32(*lineY0, rv, guv, bu);
            lineY0 += 2;
            *rgb1++ = qYUVToARGB32(*lineY1, rv, guv, bu);
            lineY1 += 2;
            *rgb1++ = qYUVToARGB32(*lineY1, rv, guv, bu);
            lineY1 += 2;
        }

        y += yStride << 1; // stride * 2
        u += uStride;
        v += vStride;
        rgb0 += width;
        rgb1 += width;
    }
}

static void QT_FASTCALL qt_convert_P016_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_BIPLANAR(frame)
    planarYUV420_16bit_to_ARGB32(plane1 + 1, plane1Stride,
                           plane2 + 1, plane2Stride,
                           plane2 + 3, plane2Stride,
                           4,
                           reinterpret_cast<quint32*>(output),
                           width, height);

}

template <typename Y>
static void QT_FASTCALL qt_convert_Y_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, (int)sizeof(Y))
    quint32 *argb = reinterpret_cast<quint32*>(output);

    using Pixel = YPixel<Y>;

    for (int y = 0; y < height; ++y) {
        const Pixel *pixel = reinterpret_cast<const Pixel *>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = pixel->convert();
            ++pixel;
            *argb++ = pixel->convert();
            ++pixel;
            *argb++ = pixel->convert();
            ++pixel;
            *argb++ = pixel->convert();
            ++pixel;
        }

        // leftovers
        for (; x < width; ++x) {
            *argb++ = pixel->convert();
            ++pixel;
        }

        src += stride;
    }
    MERGE_LOOPS(width, height, stride, 1)
}

static VideoFrameConvertFunc qConvertFuncs[QVideoFrameFormat::NPixelFormats] = {
    /* Format_Invalid */                nullptr, // Not needed
    /* Format_ARGB8888 */                 qt_convert_to_ARGB32<ARGB8888>,
    /* Format_ARGB8888_Premultiplied */   qt_convert_premultiplied_to_ARGB32<ARGB8888>,
    /* Format_XRGB8888 */                 qt_convert_premultiplied_to_ARGB32<XRGB8888>,
    /* Format_BGRA8888 */                 qt_convert_to_ARGB32<BGRA8888>,
    /* Format_BGRA8888_Premultiplied */   qt_convert_premultiplied_to_ARGB32<BGRA8888>,
    /* Format_BGRX8888 */                 qt_convert_premultiplied_to_ARGB32<BGRX8888>,
    /* Format_ABGR8888 */                 qt_convert_to_ARGB32<ABGR8888>,
    /* Format_XBGR8888 */                 qt_convert_premultiplied_to_ARGB32<XBGR8888>,
    /* Format_RGBA8888 */                 qt_convert_to_ARGB32<RGBA8888>,
    /* Format_RGBX8888 */                 qt_convert_premultiplied_to_ARGB32<RGBX8888>,
    /* Format_AYUV */                     qt_convert_AYUV_to_ARGB32,
    /* Format_AYUV_Premultiplied */       qt_convert_AYUV_Premultiplied_to_ARGB32,
    /* Format_YUV420P */                qt_convert_YUV420P_to_ARGB32,
    /* Format_YUV422P */                qt_convert_YUV422P_to_ARGB32,
    /* Format_YV12 */                   qt_convert_YV12_to_ARGB32,
    /* Format_UYVY */                   qt_convert_UYVY_to_ARGB32,
    /* Format_YUYV */                   qt_convert_YUYV_to_ARGB32,
    /* Format_NV12 */                   qt_convert_NV12_to_ARGB32,
    /* Format_NV21 */                   qt_convert_NV21_to_ARGB32,
    /* Format_IMC1 */                   qt_convert_IMC1_to_ARGB32,
    /* Format_IMC2 */                   qt_convert_IMC2_to_ARGB32,
    /* Format_IMC3 */                   qt_convert_IMC3_to_ARGB32,
    /* Format_IMC4 */                   qt_convert_IMC4_to_ARGB32,
    /* Format_Y8 */                     qt_convert_Y_to_ARGB32<uchar>,
    /* Format_Y16 */                    qt_convert_Y_to_ARGB32<ushort>,
    /* Format_P010 */                   qt_convert_P016_to_ARGB32,
    /* Format_P016 */                   qt_convert_P016_to_ARGB32,
    /* Format_Jpeg */                   nullptr, // Not needed
};

static void qInitConvertFuncsAsm()
{
#ifdef QT_COMPILER_SUPPORTS_SSE2
    extern void QT_FASTCALL  qt_convert_ARGB8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_ABGR8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_RGBA8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_BGRA8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    if (qCpuHasFeature(SSE2)){
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888] = qt_convert_ARGB8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888_Premultiplied] = qt_convert_ARGB8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_XRGB8888] = qt_convert_ARGB8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888_Premultiplied] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_ABGR8888] = qt_convert_ABGR8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_ABGR8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_sse2;
    }
#endif
#ifdef QT_COMPILER_SUPPORTS_SSSE3
    extern void QT_FASTCALL  qt_convert_ARGB8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_ABGR8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_RGBA8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_BGRA8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output);
    if (qCpuHasFeature(SSSE3)){
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888] = qt_convert_ARGB8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888_Premultiplied] = qt_convert_ARGB8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_XRGB8888] = qt_convert_ARGB8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888_Premultiplied] = qt_convert_BGRA8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_BGRA8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_ABGR8888] = qt_convert_ABGR8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_ABGR8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_ssse3;
    }
#endif
#ifdef QT_COMPILER_SUPPORTS_AVX2
    extern void QT_FASTCALL  qt_convert_ARGB8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_ABGR8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_RGBA8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_BGRA8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    if (qCpuHasFeature(AVX2)){
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888] = qt_convert_ARGB8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888_Premultiplied] = qt_convert_ARGB8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_XRGB8888] = qt_convert_ARGB8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888_Premultiplied] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_ABGR8888] = qt_convert_ABGR8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_ABGR8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_avx2;
    }
#endif
}

VideoFrameConvertFunc qConverterForFormat(QVideoFrameFormat::PixelFormat format)
{
    static bool initAsmFuncsDone = false;
    if (!initAsmFuncsDone) {
        qInitConvertFuncsAsm();
        initAsmFuncsDone = true;
    }
    VideoFrameConvertFunc convert = qConvertFuncs[format];
    return convert;
}


QT_END_NAMESPACE
