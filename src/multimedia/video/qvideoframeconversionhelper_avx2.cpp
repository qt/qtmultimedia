// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframeconversionhelper_p.h"

#ifdef QT_COMPILER_SUPPORTS_AVX2

QT_BEGIN_NAMESPACE

namespace  {

template<int a, int r, int g, int b>
void convert_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)
    quint32 *argb = reinterpret_cast<quint32*>(output);

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    __m256i shuffleMask = _mm256_set_epi8(12 + a, 12 + r, 12 + g, 12 + b,
                                          8 + a, 8 + r, 8 + g, 8 + b,
                                          4 + a, 4 + r, 4 + g, 4 + b,
                                          0 + a, 0 + r, 0 + g, 0 + b,
                                          12 + a, 12 + r, 12 + g, 12 + b,
                                          8 + a, 8 + r, 8 + g, 8 + b,
                                          4 + a, 4 + r, 4 + g, 4 + b,
                                          0 + a, 0 + r, 0 + g, 0 + b);
#else
    __m256i shuffleMask = _mm256_set_epi8(15 - a, 15 - r, 15 - g, 15 - b,
                                          11 - a, 11 - r, 11 - g, 11 - b,
                                          7 - a, 7 - r, 7 - g, 7 - b,
                                          3 - a, 3 - r, 3 - g, 3 - b,
                                          15 - a, 15 - r, 15 - g, 15 - b,
                                          11 - a, 11 - r, 11 - g, 11 - b,
                                          7 - a, 7 - r, 7 - g, 7 - b,
                                          3 - a, 3 - r, 3 - g, 3 - b);
#endif

    using Pixel = const ArgbPixel<a, r, g, b>;

    for (int y = 0; y < height; ++y) {
        auto *pixel = reinterpret_cast<const Pixel *>(src);

        int x = 0;
        ALIGN(32, argb, x, width) {
            *argb = pixel->convert();
            ++pixel;
            ++argb;
        }

        for (; x < width - 15; x += 16) {
            __m256i pixelData = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pixel));
            __m256i pixelData2 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(pixel + 8));
            pixel += 16;
            pixelData = _mm256_shuffle_epi8(pixelData, shuffleMask);
            pixelData2 = _mm256_shuffle_epi8(pixelData2, shuffleMask);
            _mm256_store_si256(reinterpret_cast<__m256i*>(argb), pixelData);
            _mm256_store_si256(reinterpret_cast<__m256i*>(argb + 8), pixelData2);
            argb += 16;
        }

        // leftovers
        for (; x < width; ++x) {
            *argb = pixel->convert();
            ++pixel;
            ++argb;
        }

        src += stride;
    }
}

}


void QT_FASTCALL qt_convert_ARGB8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_avx2<0, 1, 2, 3>(frame, output);
}

void QT_FASTCALL qt_convert_ABGR8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_avx2<0, 3, 2, 1>(frame, output);
}

void QT_FASTCALL qt_convert_RGBA8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_avx2<3, 0, 1, 2>(frame, output);
}

void QT_FASTCALL qt_convert_BGRA8888_to_ARGB32_avx2(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_avx2<3, 2, 1, 0>(frame, output);
}

QT_END_NAMESPACE

#endif
