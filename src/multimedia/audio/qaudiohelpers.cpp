// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiohelpers_p.h"

#include <limits>

QT_BEGIN_NAMESPACE

namespace QAudioHelperInternal
{

#if defined(Q_CC_GNU)
#  define QT_MM_RESTRICT __restrict__
#elif defined(Q_CC_MSVC)
#  define QT_MM_RESTRICT __restrict
#else
#  define QT_MM_RESTRICT /*__restrict__*/
#endif

namespace {

template<class T>
inline T applyVolumeOnSample(T sample, float factor)
{
    if constexpr (std::is_signed_v<T>) {
        return sample * factor;
    } else {
        using SignedT = std::make_signed_t<T>;
        // Unsigned samples are biased around 0x80/0x8000
        constexpr T offset = SignedT(1) << (std::numeric_limits<T>::digits - 1);
        return offset + (SignedT(sample - offset) * factor);
    }
}

template<class T>
void adjustSamples(float factor,
                   const void *QT_MM_RESTRICT src,
                   void *QT_MM_RESTRICT dst,
                   int samples)
{
    const T *pSrc = (const T *)src;
    T *pDst = (T *)dst;

    for (int i = 0; i < samples; i++)
        pDst[i] = applyVolumeOnSample(pSrc[i], factor);
}

} // namespace

void qMultiplySamples(float factor,
                      const QAudioFormat &format,
                      const void *src,
                      void *dest,
                      int len) QT_MM_NONBLOCKING
{
    const int samplesCount = len / qMax(1, format.bytesPerSample());

    auto clamp = [](float arg) {
        float realVolume = std::clamp<float>(arg, 0.f, 1.f);
        return realVolume;
    };

    switch (format.sampleFormat()) {
    case QAudioFormat::UInt8:
        return QAudioHelperInternal::adjustSamples<quint8>(clamp(factor), src, dest, samplesCount);
    case QAudioFormat::Int16:
        return QAudioHelperInternal::adjustSamples<qint16>(clamp(factor), src, dest, samplesCount);
    case QAudioFormat::Int32:
        return QAudioHelperInternal::adjustSamples<qint32>(clamp(factor), src, dest, samplesCount);
    case QAudioFormat::Float:
        return QAudioHelperInternal::adjustSamples<float>(factor, src, dest, samplesCount);
    default:
        Q_UNREACHABLE_RETURN();
    }
}

void applyVolume(float volume,
                 const QAudioFormat &format,
                 QSpan<const std::byte> source,
                 QSpan<std::byte> destination) QT_MM_NONBLOCKING
{
    Q_ASSERT(source.size() == destination.size());

    if (Q_LIKELY(volume == 1.f)) {
        std::copy(source.begin(), source.end(), destination.begin());
    } else if (volume == 0) {
        std::byte zero =
                format.sampleFormat() == QAudioFormat::UInt8 ? std::byte{ 0x80 } : std::byte{ 0 };

        std::fill(destination.begin(), destination.begin() + source.size(), zero);
    } else {
        QAudioHelperInternal::qMultiplySamples(volume, format, source.data(), destination.data(),
                                               source.size());
    }
}

} // namespace QAudioHelperInternal

#undef QT_MM_RESTRICT

QT_END_NAMESPACE
