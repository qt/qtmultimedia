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

static void QT_FASTCALL qt_convert_AYUV444_to_ARGB32(const QVideoFrame &frame, uchar *output)
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

static void QT_FASTCALL qt_convert_AYUV444_Premultiplied_to_ARGB32(const QVideoFrame &frame, uchar *output)
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


static void QT_FASTCALL qt_convert_BGRA32_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const quint32 *bgra = reinterpret_cast<const quint32*>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = qPremultiply(qConvertBGRA32ToARGB32(*bgra++));
            *argb++ = qPremultiply(qConvertBGRA32ToARGB32(*bgra++));
            *argb++ = qPremultiply(qConvertBGRA32ToARGB32(*bgra++));
            *argb++ = qPremultiply(qConvertBGRA32ToARGB32(*bgra++));
        }

        // leftovers
        for (; x < width; ++x)
            *argb++ = qPremultiply(qConvertBGRA32ToARGB32(*bgra++));

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_BGRA32_Premultiplied_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const quint32 *bgra = reinterpret_cast<const quint32*>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = qConvertBGRA32ToARGB32(*bgra++);
            *argb++ = qConvertBGRA32ToARGB32(*bgra++);
            *argb++ = qConvertBGRA32ToARGB32(*bgra++);
            *argb++ = qConvertBGRA32ToARGB32(*bgra++);
        }

        // leftovers
        for (; x < width; ++x)
            *argb++ = qConvertBGRA32ToARGB32(*bgra++);

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_ABGR32_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const quint32 *abgr = reinterpret_cast<const quint32*>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = qConvertABGR32ToARGB32(*abgr++);
            *argb++ = qConvertABGR32ToARGB32(*abgr++);
            *argb++ = qConvertABGR32ToARGB32(*abgr++);
            *argb++ = qConvertABGR32ToARGB32(*abgr++);
        }

        // leftovers
        for (; x < width; ++x)
            *argb++ = qConvertABGR32ToARGB32(*abgr++);

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_BGR565_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 2)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const quint16 *bgr = reinterpret_cast<const quint16*>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = qConvertBGR565ToARGB32(*bgr++);
            *argb++ = qConvertBGR565ToARGB32(*bgr++);
            *argb++ = qConvertBGR565ToARGB32(*bgr++);
            *argb++ = qConvertBGR565ToARGB32(*bgr++);
        }

        // leftovers
        for (; x < width; ++x)
            *argb++ = qConvertBGR565ToARGB32(*bgr++);

        src += stride;
    }
}

static void QT_FASTCALL qt_convert_BGR555_to_ARGB32(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 2)

    quint32 *argb = reinterpret_cast<quint32*>(output);

    for (int y = 0; y < height; ++y) {
        const quint16 *bgr = reinterpret_cast<const quint16*>(src);

        int x = 0;
        for (; x < width - 3; x += 4) {
            *argb++ = qConvertBGR555ToARGB32(*bgr++);
            *argb++ = qConvertBGR555ToARGB32(*bgr++);
            *argb++ = qConvertBGR555ToARGB32(*bgr++);
            *argb++ = qConvertBGR555ToARGB32(*bgr++);
        }

        // leftovers
        for (; x < width; ++x)
            *argb++ = qConvertBGR555ToARGB32(*bgr++);

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

static VideoFrameConvertFunc qConvertFuncs[QVideoSurfaceFormat::NPixelFormats] = {
    /* Format_Invalid */                nullptr, // Not needed
    /* Format_ARGB32 */                 nullptr, // Not needed
    /* Format_ARGB32_Premultiplied */   nullptr, // Not needed
    /* Format_RGB32 */                  nullptr, // Not needed
    /* Format_RGB565 */                 nullptr, // Not needed
    /* Format_RGB555 */                 nullptr, // Not needed
    /* Format_BGRA32 */                 qt_convert_BGRA32_to_ARGB32,
    /* Format_BGRA32_Premultiplied */   qt_convert_BGRA32_Premultiplied_to_ARGB32,
    /* Format_ABGR32 */                 qt_convert_ABGR32_to_ARGB32,
    /* Format_BGR32 */                  qt_convert_BGRA32_Premultiplied_to_ARGB32,
    /* Format_BGR565 */                 qt_convert_BGR565_to_ARGB32,
    /* Format_BGR555 */                 qt_convert_BGR555_to_ARGB32,
    /* Format_AYUV444 */                qt_convert_AYUV444_to_ARGB32,
    /* Format_AYUV444_Premultiplied */  qt_convert_AYUV444_Premultiplied_to_ARGB32,
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
    /* Format_Y8 */                     nullptr, // Not needed
    /* Format_Y16 */                    nullptr, // Not needed
    /* Format_P010 */                   qt_convert_P016_to_ARGB32,
    /* Format_P016 */                   qt_convert_P016_to_ARGB32,
    /* Format_Jpeg */                   nullptr, // Not needed
};

static void qInitConvertFuncsAsm()
{
#ifdef QT_COMPILER_SUPPORTS_SSE2
    extern void QT_FASTCALL qt_convert_BGRA32_to_ARGB32_sse2(const QVideoFrame&, uchar*);
    if (qCpuHasFeature(SSE2)){
        qConvertFuncs[QVideoSurfaceFormat::Format_BGRA32] = qt_convert_BGRA32_to_ARGB32_sse2;
        qConvertFuncs[QVideoSurfaceFormat::Format_BGRA32_Premultiplied] = qt_convert_BGRA32_to_ARGB32_sse2;
        qConvertFuncs[QVideoSurfaceFormat::Format_BGR32] = qt_convert_BGRA32_to_ARGB32_sse2;
    }
#endif
#ifdef QT_COMPILER_SUPPORTS_SSSE3
    extern void QT_FASTCALL qt_convert_BGRA32_to_ARGB32_ssse3(const QVideoFrame&, uchar*);
    if (qCpuHasFeature(SSSE3)){
        qConvertFuncs[QVideoSurfaceFormat::Format_BGRA32] = qt_convert_BGRA32_to_ARGB32_ssse3;
        qConvertFuncs[QVideoSurfaceFormat::Format_BGRA32_Premultiplied] = qt_convert_BGRA32_to_ARGB32_ssse3;
        qConvertFuncs[QVideoSurfaceFormat::Format_BGR32] = qt_convert_BGRA32_to_ARGB32_ssse3;
    }
#endif
#ifdef QT_COMPILER_SUPPORTS_AVX2
    extern void QT_FASTCALL qt_convert_BGRA32_to_ARGB32_avx2(const QVideoFrame&, uchar*);
    if (qCpuHasFeature(AVX2)){
        qConvertFuncs[QVideoSurfaceFormat::Format_BGRA32] = qt_convert_BGRA32_to_ARGB32_avx2;
        qConvertFuncs[QVideoSurfaceFormat::Format_BGRA32_Premultiplied] = qt_convert_BGRA32_to_ARGB32_avx2;
        qConvertFuncs[QVideoSurfaceFormat::Format_BGR32] = qt_convert_BGRA32_to_ARGB32_avx2;
    }
#endif
}

VideoFrameConvertFunc qConverterForFormat(QVideoSurfaceFormat::PixelFormat format)
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
