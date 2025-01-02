// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframeconversionhelper_p.h"
#include "qrgb.h"

#include <mutex>

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
    height &= ~1;
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

        y += yStride; // stride * 2
        u += uStride;
        v += vStride;
        rgb0 += width;
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
            // Copy 4 pixels onto the stack in one go. This significantly increases performance
            // in the case where the mapped memory is uncached (because it's a framebuffer)
            Pixel p[4];
            memcpy(p, data, 4*sizeof(Pixel));
            *argb++ = qPremultiply(p[0].convert());
            *argb++ = qPremultiply(p[1].convert());
            *argb++ = qPremultiply(p[2].convert());
            *argb++ = qPremultiply(p[3].convert());
            data += 4;
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
            // Copy 4 pixels onto the stack in one go. This significantly increases performance
            // in the case where the mapped memory is uncached (because it's a framebuffer)
            Pixel p[4];
            memcpy(p, data, 4*sizeof(Pixel));
            *argb++ = p[0].convert();
            *argb++ = p[1].convert();
            *argb++ = p[2].convert();
            *argb++ = p[3].convert();
            data += 4;
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
    height &= ~1;
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

template<typename Pixel>
static void QT_FASTCALL qt_copy_pixels_with_mask(Pixel *dst, const Pixel *src, size_t size,
                                                 Pixel mask)
{
    for (size_t x = 0; x < size; ++x)
        dst[x] = src[x] | mask;
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

static PixelsCopyFunc qPixelsCopyFunc = qt_copy_pixels_with_mask<uint32_t>;

static std::once_flag InitFuncsAsmFlag;

static void qInitFuncsAsm()
{
#ifdef QT_COMPILER_SUPPORTS_SSE2
    extern void QT_FASTCALL  qt_convert_ARGB8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_ABGR8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_RGBA8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_BGRA8888_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_copy_pixels_with_mask_sse2(uint32_t * dst, const uint32_t *src, size_t size, uint32_t mask);

    if (qCpuHasFeature(SSE2)){
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888] = qt_convert_ARGB8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888_Premultiplied] = qt_convert_ARGB8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_XRGB8888] = qt_convert_ARGB8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888_Premultiplied] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRX8888] = qt_convert_BGRA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_ABGR8888] = qt_convert_ABGR8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_ABGR8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_RGBA8888] = qt_convert_RGBA8888_to_ARGB32_sse2;
        qConvertFuncs[QVideoFrameFormat::Format_RGBX8888] = qt_convert_RGBA8888_to_ARGB32_sse2;

        qPixelsCopyFunc = qt_copy_pixels_with_mask_sse2;
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
        qConvertFuncs[QVideoFrameFormat::Format_BGRX8888] = qt_convert_BGRA8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_ABGR8888] = qt_convert_ABGR8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_ABGR8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_RGBA8888] = qt_convert_RGBA8888_to_ARGB32_ssse3;
        qConvertFuncs[QVideoFrameFormat::Format_RGBX8888] = qt_convert_RGBA8888_to_ARGB32_ssse3;
    }
#endif
#ifdef QT_COMPILER_SUPPORTS_AVX2
    extern void QT_FASTCALL  qt_convert_ARGB8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_ABGR8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_RGBA8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_convert_BGRA8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output);
    extern void QT_FASTCALL  qt_copy_pixels_with_mask_avx2(uint32_t * dst, const uint32_t *src, size_t size, uint32_t mask);
    if (qCpuHasFeature(AVX2)){
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888] = qt_convert_ARGB8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_ARGB8888_Premultiplied] = qt_convert_ARGB8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_XRGB8888] = qt_convert_ARGB8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRA8888_Premultiplied] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_BGRX8888] = qt_convert_BGRA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_ABGR8888] = qt_convert_ABGR8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_XBGR8888] = qt_convert_ABGR8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_RGBA8888] = qt_convert_RGBA8888_to_ARGB32_avx2;
        qConvertFuncs[QVideoFrameFormat::Format_RGBX8888] = qt_convert_RGBA8888_to_ARGB32_avx2;

        qPixelsCopyFunc = qt_copy_pixels_with_mask_avx2;
    }
#endif
}

VideoFrameConvertFunc qConverterForFormat(QVideoFrameFormat::PixelFormat format)
{
    std::call_once(InitFuncsAsmFlag, &qInitFuncsAsm);

    VideoFrameConvertFunc convert = qConvertFuncs[format];
    return convert;
}

void Q_MULTIMEDIA_EXPORT qCopyPixelsWithAlphaMask(uint32_t *dst,
                                                  const uint32_t *src,
                                                  size_t pixCount,
                                                  QVideoFrameFormat::PixelFormat format,
                                                  bool srcAlphaVaries)
{
    if (pixCount == 0)
        return;

    const auto mask = qAlphaMask(format);

    if (srcAlphaVaries || (src[0] & mask) != mask)
        qCopyPixelsWithMask(dst, src, pixCount, mask);
    else
        memcpy(dst, src, pixCount * 4);
}

void qCopyPixelsWithMask(uint32_t *dst, const uint32_t *src, size_t size, uint32_t mask)
{
    std::call_once(InitFuncsAsmFlag, &qInitFuncsAsm);

    qPixelsCopyFunc(dst, src, size, mask);
}

uint32_t qAlphaMask(QVideoFrameFormat::PixelFormat format)
{
    switch (format) {
    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return 0xff;
#else
        return 0xff000000;
#endif
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        return 0xff000000;
#else
        return 0xff;
#endif
    default:
        return 0;
    }
}

QT_END_NAMESPACE
