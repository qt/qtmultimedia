/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qvideoframeconversionhelper_p.h"

#ifdef QT_COMPILER_SUPPORTS_SSE2

QT_BEGIN_NAMESPACE

void QT_FASTCALL qt_convert_BGRA32_to_ARGB32_sse2(const QVideoFrame &frame, uchar *output)
{
    FETCH_INFO_PACKED(frame)
    MERGE_LOOPS(width, height, stride, 4)
    quint32 *argb = reinterpret_cast<quint32*>(output);

    const __m128i zero = _mm_setzero_si128();

    for (int y = 0; y < height; ++y) {
        const quint32 *bgra = reinterpret_cast<const quint32*>(src);

        int x = 0;
        ALIGN(16, argb, x, width) {
            *argb = qConvertBGRA32ToARGB32(*bgra);
            ++bgra;
            ++argb;
        }

        for (; x < width - 3; x += 4) {
            __m128i pixelData = _mm_loadu_si128(reinterpret_cast<const __m128i*>(bgra));
            bgra += 4;
            __m128i gaComponents = _mm_unpacklo_epi8(pixelData, zero);
            __m128i brComponents = _mm_unpackhi_epi8(pixelData, zero);
            gaComponents = _mm_shufflelo_epi16(_mm_shufflehi_epi16(gaComponents, _MM_SHUFFLE(0, 1, 2, 3)), _MM_SHUFFLE(0, 1, 2, 3)); // swap GA
            brComponents = _mm_shufflelo_epi16(_mm_shufflehi_epi16(brComponents, _MM_SHUFFLE(0, 1, 2, 3)), _MM_SHUFFLE(0, 1, 2, 3)); // swap BR
            pixelData = _mm_packus_epi16(gaComponents, brComponents);
            _mm_store_si128(reinterpret_cast<__m128i*>(argb), pixelData);
            argb += 4;
        }

        // leftovers
        for (; x < width; ++x) {
            *argb = qConvertBGRA32ToARGB32(*bgra);
            ++bgra;
            ++argb;
        }

        src += stride;
    }
}

QT_END_NAMESPACE

#endif
