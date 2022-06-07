// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideoframeconversionhelper_p.h"

#ifdef QT_COMPILER_SUPPORTS_SSSE3

QT_BEGIN_NAMESPACE

namespace  {

template<int a, int r, int g, int b>
void convert_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)
    quint32 *argb = reinterpret_cast<quint32*>(output);

#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    __m128i shuffleMask = _mm_set_epi8(12 + a, 12 + r, 12 + g, 12 + b,
                                       8 + a, 8 + r, 8 + g, 8 + b,
                                       4 + a, 4 + r, 4 + g, 4 + b,
                                       0 + a, 0 + r, 0 + g, 0 + b);
#else
    __m128i shuffleMask = _mm_set_epi8(15 - a, 15 - r, 15 - g, 15 - b,
                                       11 - a, 11 - r, 11 - g, 11 - b,
                                       7 - a, 7 - r, 7 - g, 7 - b,
                                       3 - a, 3 - r, 3 - g, 3 - b);
#endif

    using Pixel = const ArgbPixel<a, r, g, b>;

    for (int y = 0; y < height; ++y) {
        const auto *pixel = reinterpret_cast<const Pixel *>(src);

        int x = 0;
        ALIGN(16, argb, x, width) {
            *argb = pixel->convert();
            ++pixel;
            ++argb;
        }

        for (; x < width - 7; x += 8) {
            __m128i pixelData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pixel));
            __m128i pixelData2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pixel + 4));
            pixel += 8;
            pixelData = _mm_shuffle_epi8(pixelData, shuffleMask);
            pixelData2 = _mm_shuffle_epi8(pixelData2, shuffleMask);
            _mm_store_si128(reinterpret_cast<__m128i*>(argb), pixelData);
            _mm_store_si128(reinterpret_cast<__m128i*>(argb + 4), pixelData2);
            argb += 8;
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

void QT_FASTCALL qt_convert_ARGB8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_ssse3<0, 1, 2, 3>(frame, output);
}

void QT_FASTCALL qt_convert_ABGR8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_ssse3<0, 3, 2, 1>(frame, output);
}

void QT_FASTCALL qt_convert_RGBA8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_ssse3<3, 0, 1, 2>(frame, output);
}

void QT_FASTCALL qt_convert_BGRA8888_to_ARGB32_ssse3(const QVideoFrame &frame, uchar *output)
{
    convert_to_ARGB32_ssse3<3, 2, 1, 0>(frame, output);
}

QT_END_NAMESPACE

#endif
