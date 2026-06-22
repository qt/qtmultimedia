// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaudiohelpers_p.h"

#include <QtMultimedia/private/qaudio_qspan_support_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qmultimedia_enum_to_string_converter_p.h>
#include <QtCore/qdebug.h>

#include <algorithm>
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
                      int len) noexcept Q_DECL_NONBLOCKING_FUNCTION
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
                 QSpan<std::byte> destination) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    Q_ASSERT(source.size() == destination.size());

    if (Q_LIKELY(volume == 1.f)) {
        if (source.data() != destination.data())
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

namespace {

template <NativeSampleFormat Format>
Q_ALWAYS_INLINE constexpr int32_t toInt32(QSpan<const std::byte> source) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    switch (Format) {
    case NativeSampleFormat::uint8_t: {
        constexpr int32_t offset = (1 << 7);
        uint8_t smp = uint8_t(source.front());
        int32_t val = (int32_t(smp) - offset) << 24;
        return val;
    }
    case NativeSampleFormat::int16_t: {
        union CastUnion {
            std::int16_t i16;
            std::byte b[2];
        } castUnion = {};

        std::copy_n(source.data(), 2, castUnion.b);

        int32_t val = castUnion.i16 << 16;
        return val;
    }
    case NativeSampleFormat::int24_t_3b: {
        union CastUnion {
            std::int32_t i32;
            std::byte b[4];
        } castUnion = {};

        std::copy_n(source.data(), 3, castUnion.b);

        int32_t val = castUnion.i32 << 8;
        return val;
    }
    case NativeSampleFormat::int24_t_4b_low: {
        union CastUnion {
            std::int32_t i32;
            std::byte b[4];
        } castUnion = {};

        std::copy_n(source.data(), 4, castUnion.b);
        int32_t val = castUnion.i32 << 8;
        return val;
    }
    case NativeSampleFormat::float32_t: {
        union CastUnion {
            float f32;
            std::byte b[4];
        } castUnion = {};

        std::copy_n(source.data(), 4, castUnion.b);
        constexpr double range = std::numeric_limits<int32_t>::max();
        int32_t val = (castUnion.f32 * range);
        return val;
    }
    case NativeSampleFormat::int32_t: {
        union CastUnion {
            std::int32_t i32;
            std::byte b[4];
        } castUnion = {};

        std::copy_n(source.data(), 4, castUnion.b);
        return castUnion.i32;
    }

    default:
        Q_UNREACHABLE_RETURN({});
    }
}

template <NativeSampleFormat Format>
Q_ALWAYS_INLINE constexpr void storeSampleWithFormat(QSpan<std::byte> destination,
                                                     int32_t value) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    switch (Format) {
    case NativeSampleFormat::uint8_t: {
        value = value >> 24;
        constexpr int32_t offset = 1 << 7;
        uint8_t sampleValue = value + offset;
        std::copy_n(reinterpret_cast<const std::byte *>(&sampleValue), 1, destination.data());
        return;
    }
    case NativeSampleFormat::int16_t: {
        value = value >> 16;
        int16_t sampleValue = value;
        std::copy_n(reinterpret_cast<const std::byte *>(&sampleValue), 2, destination.data());
        return;
    }

    case NativeSampleFormat::int24_t_3b: {
        int32_t sampleValue = value >> 8;
        std::copy_n(reinterpret_cast<const std::byte *>(&sampleValue), 3, destination.data());
        return;
    }

    case NativeSampleFormat::int24_t_4b_low: {
        int32_t sampleValue = (value >> 8) & 0x00ffffff;
        std::copy_n(reinterpret_cast<const std::byte *>(&sampleValue), 4, destination.data());
        return;
    }
    case NativeSampleFormat::int32_t: {
        std::copy_n(reinterpret_cast<const std::byte *>(&value), 4, destination.data());
        return;
    }
    case NativeSampleFormat::float32_t: {
        constexpr double normalizationFactor = 1.0 / std::numeric_limits<int32_t>::max();
        float sampleValue = float(double(value) * normalizationFactor);
        std::copy_n(reinterpret_cast<const std::byte *>(&sampleValue), 4, destination.data());
        return;
    }
    default:
        Q_UNREACHABLE_RETURN();
    }
}

template <NativeSampleFormat sourceFormat, NativeSampleFormat destinationFormat>
struct WordConverter
{
    void operator()(QSpan<const std::byte> source, QSpan<std::byte> destination)
    {
        if constexpr (sourceFormat == destinationFormat) {
            std::copy(source.begin(), source.end(), destination.begin());
        } else {
            constexpr size_t bytesPerSampleSource = bytesPerSample(sourceFormat);
            constexpr size_t bytesPerSampleDest = bytesPerSample(destinationFormat);

            using namespace QtMultimediaPrivate;
            while (!source.isEmpty()) {
                if (size_t(source.size()) >= 4 * bytesPerSampleSource) {
                    // unroll by 4.
                    // should help the compiler to vectorize, or at least avoid pipeline stalls.

                    auto sourceSampleRange1 = take(source, bytesPerSampleSource);
                    source = drop(source, bytesPerSampleSource);
                    auto sourceSampleRange2 = take(source, bytesPerSampleSource);
                    source = drop(source, bytesPerSampleSource);
                    auto sourceSampleRange3 = take(source, bytesPerSampleSource);
                    source = drop(source, bytesPerSampleSource);
                    auto sourceSampleRange4 = take(source, bytesPerSampleSource);
                    source = drop(source, bytesPerSampleSource);

                    int32_t value1 = toInt32<sourceFormat>(sourceSampleRange1);
                    int32_t value2 = toInt32<sourceFormat>(sourceSampleRange2);
                    int32_t value3 = toInt32<sourceFormat>(sourceSampleRange3);
                    int32_t value4 = toInt32<sourceFormat>(sourceSampleRange4);

                    auto destSampleRange1 = take(destination, bytesPerSampleDest);
                    destination = drop(destination, bytesPerSampleDest);
                    auto destSampleRange2 = take(destination, bytesPerSampleDest);
                    destination = drop(destination, bytesPerSampleDest);
                    auto destSampleRange3 = take(destination, bytesPerSampleDest);
                    destination = drop(destination, bytesPerSampleDest);
                    auto destSampleRange4 = take(destination, bytesPerSampleDest);
                    destination = drop(destination, bytesPerSampleDest);

                    storeSampleWithFormat<destinationFormat>(destSampleRange1, value1);
                    storeSampleWithFormat<destinationFormat>(destSampleRange2, value2);
                    storeSampleWithFormat<destinationFormat>(destSampleRange3, value3);
                    storeSampleWithFormat<destinationFormat>(destSampleRange4, value4);
                } else {
                    auto sourceSampleRange = take(source, bytesPerSampleSource);
                    int32_t value = toInt32<sourceFormat>(sourceSampleRange);

                    auto destSampleRange = take(destination, bytesPerSampleDest);
                    storeSampleWithFormat<destinationFormat>(destSampleRange, value);

                    source = drop(source, bytesPerSampleSource);
                    destination = drop(destination, bytesPerSampleDest);
                }
            }
        }
    }
};

template <NativeSampleFormat destinationFormat>
void convertSampleFormatWithDestinationFormat(QSpan<const std::byte> source,
                                              NativeSampleFormat sourceFormat,
                                              QSpan<std::byte> destination) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    switch (sourceFormat) {
    case NativeSampleFormat::uint8_t:
        return WordConverter<NativeSampleFormat::uint8_t, destinationFormat>()(source, destination);
    case NativeSampleFormat::int16_t:
        return WordConverter<NativeSampleFormat::int16_t, destinationFormat>()(source, destination);
    case NativeSampleFormat::int32_t:
        return WordConverter<NativeSampleFormat::int32_t, destinationFormat>()(source, destination);
    case NativeSampleFormat::int24_t_3b:
        return WordConverter<NativeSampleFormat::int24_t_3b, destinationFormat>()(source,
                                                                                  destination);
    case NativeSampleFormat::int24_t_4b_low:
        return WordConverter<NativeSampleFormat::int24_t_4b_low, destinationFormat>()(source,
                                                                                      destination);
    case NativeSampleFormat::float32_t:
        return WordConverter<NativeSampleFormat::float32_t, destinationFormat>()(source,
                                                                                 destination);
    default:
        Q_UNREACHABLE_RETURN();
    }
}

} // namespace

void convertSampleFormat(QSpan<const std::byte> source, NativeSampleFormat sourceFormat,
                         QSpan<std::byte> destination,
                         NativeSampleFormat destinationFormat) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    using namespace QtMultimediaPrivate;

    size_t bytesPerSampleSource = bytesPerSample(sourceFormat);
    size_t bytesPerSampleDest = bytesPerSample(destinationFormat);

    Q_ASSERT(source.size() * bytesPerSampleDest == destination.size() * bytesPerSampleSource);

    switch (destinationFormat) {
    case NativeSampleFormat::uint8_t:
        return convertSampleFormatWithDestinationFormat<NativeSampleFormat::uint8_t>(
                source, sourceFormat, destination);
    case NativeSampleFormat::int16_t:
        return convertSampleFormatWithDestinationFormat<NativeSampleFormat::int16_t>(
                source, sourceFormat, destination);
    case NativeSampleFormat::int32_t:
        return convertSampleFormatWithDestinationFormat<NativeSampleFormat::int32_t>(
                source, sourceFormat, destination);
    case NativeSampleFormat::int24_t_3b:
        return convertSampleFormatWithDestinationFormat<NativeSampleFormat::int24_t_3b>(
                source, sourceFormat, destination);
    case NativeSampleFormat::int24_t_4b_low:
        return convertSampleFormatWithDestinationFormat<NativeSampleFormat::int24_t_4b_low>(
                source, sourceFormat, destination);
    case NativeSampleFormat::float32_t:
        return convertSampleFormatWithDestinationFormat<NativeSampleFormat::float32_t>(
                source, sourceFormat, destination);
    default:
        Q_UNREACHABLE_RETURN();
    }
}

NativeSampleFormat bestNativeSampleFormat(const QAudioFormat &fmt,
                                          QSpan<const NativeSampleFormat> supportedNativeFormats)
{
    Q_ASSERT(!supportedNativeFormats.empty());

    auto resolveCandidate = [&](QSpan<const NativeSampleFormat> candidates) {
        auto it = std::find_first_of(candidates.begin(), candidates.end(),
                                     supportedNativeFormats.begin(), supportedNativeFormats.end());

        if (it != candidates.end())
            return *it;

        qFatal() << "No candidate for conversion found. That should not happen";
        Q_UNREACHABLE_RETURN(NativeSampleFormat::float32_t);
    };

    // heuristics:
    // select the best format that does not involve precision
    // if that does not yield a format, find a format with the least loss of precision.
    switch (fmt.sampleFormat()) {
    case QAudioFormat::SampleFormat::UInt8: {
        constexpr auto candidates = std::array{
            NativeSampleFormat::uint8_t,    NativeSampleFormat::int16_t,
            NativeSampleFormat::int24_t_3b, NativeSampleFormat::int24_t_4b_low,
            NativeSampleFormat::int32_t,    NativeSampleFormat::float32_t,
        };

        return resolveCandidate(candidates);
    }
    case QAudioFormat::SampleFormat::Int16: {
        constexpr auto candidates = std::array{
            NativeSampleFormat::int16_t,        NativeSampleFormat::int24_t_3b,
            NativeSampleFormat::int24_t_4b_low, NativeSampleFormat::int32_t,
            NativeSampleFormat::float32_t,      NativeSampleFormat::uint8_t,
        };

        return resolveCandidate(candidates);
    }
    case QAudioFormat::SampleFormat::Int32: {
        constexpr auto candidates = std::array{
            NativeSampleFormat::int32_t,    NativeSampleFormat::float32_t,
            NativeSampleFormat::int24_t_3b, NativeSampleFormat::int24_t_4b_low,
            NativeSampleFormat::int16_t,    NativeSampleFormat::uint8_t,

        };

        return resolveCandidate(candidates);
    }
    case QAudioFormat::SampleFormat::Float: {
        constexpr auto candidates = std::array{
            NativeSampleFormat::float32_t,      NativeSampleFormat::int24_t_3b,
            NativeSampleFormat::int24_t_4b_low, NativeSampleFormat::int32_t,
            NativeSampleFormat::int16_t,        NativeSampleFormat::uint8_t,
        };

        return resolveCandidate(candidates);
    }
    case QAudioFormat::SampleFormat::Unknown:
    case QAudioFormat::SampleFormat::NSampleFormats: {
        qFatal() << "bestNativeSampleFormat called with invalid argument" << fmt.sampleFormat();
        Q_UNREACHABLE_RETURN(NativeSampleFormat::int32_t);
    }

    default: {
        Q_UNREACHABLE_RETURN(NativeSampleFormat::int32_t);
    }
    }
}

NativeSampleFormat toNativeSampleFormat(QAudioFormat::SampleFormat fmt)
{
    switch (fmt) {
    case QAudioFormat::SampleFormat::Float:
        return NativeSampleFormat::float32_t;
    case QAudioFormat::SampleFormat::UInt8:
        return NativeSampleFormat::uint8_t;
    case QAudioFormat::SampleFormat::Int16:
        return NativeSampleFormat::int16_t;
    case QAudioFormat::SampleFormat::Int32:
        return NativeSampleFormat::int32_t;
    default:
        Q_UNREACHABLE_RETURN({});
    }
}

QAudioFormat::SampleFormat bestSampleFormat(const NativeSampleFormat fmt)
{
    switch (fmt) {
    case NativeSampleFormat::uint8_t:
        return QAudioFormat::SampleFormat::UInt8;
    case NativeSampleFormat::int16_t:
        return QAudioFormat::SampleFormat::Int16;
    case NativeSampleFormat::int32_t:
    case NativeSampleFormat::int24_t_3b:
    case NativeSampleFormat::int24_t_4b_low:
        return QAudioFormat::SampleFormat::Int32;
    case NativeSampleFormat::float32_t:
        return QAudioFormat::SampleFormat::Float;
    default:
        Q_UNREACHABLE_RETURN({});
    }
}

std::optional<float> sanitizeVolume(float volume, float lastValue)
{
    constexpr float epsilon = 1.f / (1 << 22); // good enough for 22bit resolution

    // multiplication is optimized for 0 and 1
    // values which are sufficiently close to 0 and 1 can be considered as 0 or 1
    if (volume < epsilon)
        volume = 0;
    else if (volume > (1.f - epsilon))
        volume = 1.f;

    // similar to qFuzzyCompare, but with better heuristics for volume
    if (std::abs(volume - lastValue) < epsilon)
        return std::nullopt;

    return volume;
}

void fillSilence(QSpan<std::byte> buffer, NativeSampleFormat sampleFormat) noexcept
{
    namespace ranges = QtMultimediaPrivate::ranges;
    ranges::fill(buffer,
                 sampleFormat == NativeSampleFormat::uint8_t ? std::byte{ 0x80 } : std::byte{ 0 });
}

void fillSilence(QSpan<std::byte> buffer, QAudioFormat fmt) noexcept
{
    fillSilence(buffer, toNativeSampleFormat(fmt.sampleFormat()));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {

struct CatmullRomBasis
{
    explicit CatmullRomBasis(float t)
    {
        Q_ASSERT(t >= 0.0f && t < 1.0f);
        const float t2 = t * t;
        const float t3 = t2 * t;
        c0 = -0.5f * t3 + t2 - 0.5f * t;
        c1 = 1.5f * t3 - 2.5f * t2 + 1.0f;
        c2 = -1.5f * t3 + 2.0f * t2 + 0.5f * t;
        c3 = 0.5f * t3 - 0.5f * t2;
    }

    float interpolate(float pm1, float p0, float p1, float p2) const
    {
        return (c0 * pm1 + c1 * p0) + (c2 * p1 + c3 * p2);
    }

private:
    float c0, c1, c2, c3;
};

} // namespace

QByteArray resampleAudioCatmullRom(QSpan<const float> input, int nChannels, int inputRate,
                                   int outputRate)
{
    Q_ASSERT(nChannels >= 1);
    Q_ASSERT(inputRate > 0);
    Q_ASSERT(outputRate > 0);

    if (input.isEmpty() || nChannels <= 0)
        return {};

    if (inputRate == outputRate) {
        return QByteArray(reinterpret_cast<const char *>(input.data()),
                          qsizetype(input.size()) * qsizetype(sizeof(float)));
    }

    const qsizetype inputFrames = input.size() / nChannels;
    if (inputFrames == 0)
        return {};

    // Number of output frames
    const qsizetype outputFrames =
            qsizetype(double(inputFrames) * double(outputRate) / double(inputRate) + 0.5);
    if (outputFrames <= 0)
        return {};

    QByteArray out;
    out.resizeForOverwrite(outputFrames * nChannels * qsizetype(sizeof(float)));
    QSpan<float> dst(reinterpret_cast<float *>(out.data()), outputFrames * nChannels);

    using namespace QtMultimediaPrivate;

    // Catmull-Rom: needs p0,p1,p2,p3 where output samples between p1 and p2
    // p[i] = clamp(i, 0, inputFrames-1)
    const float increment = float(inputRate) / float(outputRate);

    qsizetype iPos = 0;
    float fPos = 0.0f;

    for (qsizetype outFrame = 0; outFrame < outputFrames; ++outFrame) {
        const qsizetype i1 = iPos;
        const float t = fPos;

        // Clamp frame indices
        const qsizetype i0 = std::max(qsizetype(0), i1 - 1);
        const qsizetype i2 = std::min(i1 + 1, inputFrames - 1);
        const qsizetype i3 = std::min(i1 + 2, inputFrames - 1);

        const CatmullRomBasis basis(t);

        auto frameOut = take(dst, nChannels);
        for (int ch = 0; ch < nChannels; ++ch) {
            const float pm1 = input[i0 * nChannels + ch];
            const float p0 = input[i1 * nChannels + ch];
            const float p1 = input[i2 * nChannels + ch];
            const float p2 = input[i3 * nChannels + ch];
            frameOut[ch] = basis.interpolate(pm1, p0, p1, p2);
        }
        dst = drop(dst, nChannels);

        fPos += increment;
        while (fPos >= 1.0f) {
            fPos -= 1.0f;
            iPos += 1;
        }
    }

    return out;
}

CatmullRomInterpolator::CatmullRomInterpolator(int nChannels, int inputRate, int outputRate)
    : m_nChannels(nChannels), m_ratio(float(inputRate) / float(outputRate))
{
    Q_ASSERT(nChannels >= 1);
    Q_ASSERT(inputRate > 0);
    Q_ASSERT(outputRate > 0);

    m_history.assign(3 * nChannels, 0.0f);
}

void CatmullRomInterpolator::reset()
{
    m_iPos = 0;
    m_fPos = 0.0f;
    m_inputFramesConsumed = 0;
    namespace ranges = QtMultimediaPrivate::ranges;
    ranges::fill(m_history, 0.0f);
}

void CatmullRomInterpolator::advancePosition()
{
    m_fPos += m_ratio;
    while (m_fPos >= 1.0f) {
        m_fPos -= 1.0f;
        m_iPos += 1;
    }
}

auto CatmullRomInterpolator::process(QSpan<const float> input,
                                     QSpan<float> output) noexcept Q_DECL_NONBLOCKING_FUNCTION
        -> ResampleResult
{
    using namespace QtMultimediaPrivate;

    Q_PRESUME(m_nChannels >= 1);
    const qsizetype nChannels = m_nChannels;
    const qsizetype inputFrames = input.size() / nChannels;
    const qsizetype maxOutputFrames = output.size() / nChannels;

    // Flush mode: empty input signals end-of-stream
    if (inputFrames == 0) {
        qsizetype outputFramePos = 0;
        const qsizetype lastAvailableFrame = m_inputFramesConsumed - 1;

        while (outputFramePos < maxOutputFrames) {
            if (m_iPos > lastAvailableFrame)
                break;

            const qsizetype pm1FrameIdx = std::max(qsizetype(0), m_iPos - 1);
            const qsizetype p1FrameIdx = std::min(m_iPos + 1, lastAvailableFrame);
            const qsizetype p2FrameIdx = std::min(m_iPos + 2, lastAvailableFrame);

            const CatmullRomBasis basis(m_fPos);

            for (qsizetype ch = 0; ch < nChannels; ++ch) {
                auto fetch = [&](qsizetype absoluteFrameIdx) -> float {
                    if (absoluteFrameIdx < m_inputFramesConsumed) {
                        const qsizetype historySampleIdx =
                                absoluteFrameIdx - (m_inputFramesConsumed - 3);
                        if (historySampleIdx >= 0)
                            return m_history[historySampleIdx * nChannels + ch];
                        return m_history[ch]; // clamp to oldest history
                    }
                    return m_history[ch]; // beyond available data, clamp
                };

                const float pm1 = fetch(pm1FrameIdx);
                const float p0 = fetch(m_iPos);
                const float p1 = fetch(p1FrameIdx);
                const float p2 = fetch(p2FrameIdx);

                output[outputFramePos * nChannels + ch] =
                        basis.interpolate(pm1, p0, p1, p2);
            }

            advancePosition();
            ++outputFramePos;
        }

        return ResampleResult{
            input,
            take(output, outputFramePos * nChannels),
        };
    }

    qsizetype outputFramePos = 0;
    qsizetype maxConsumedFrames = 0;

    while (outputFramePos < maxOutputFrames) {
        if (m_iPos + 2 >= m_inputFramesConsumed + inputFrames)
            break;

        // Clamp frame indices relative to available data
        const qsizetype pm1FrameIdx = std::max(qsizetype(0), m_iPos - 1);
        const qsizetype p1FrameIdx =
                std::min(m_iPos + 1, m_inputFramesConsumed + inputFrames - 1);
        const qsizetype p2FrameIdx =
                std::min(m_iPos + 2, m_inputFramesConsumed + inputFrames - 1);

        const CatmullRomBasis basis(m_fPos);

        for (qsizetype ch = 0; ch < nChannels; ++ch) {
            auto fetch = [&](qsizetype absoluteFrameIdx) -> float {
                if (absoluteFrameIdx >= m_inputFramesConsumed)
                    return input[(absoluteFrameIdx - m_inputFramesConsumed) * nChannels + ch];
                const qsizetype historySampleIdx =
                        absoluteFrameIdx - (m_inputFramesConsumed - 3);
                if (historySampleIdx >= 0)
                    return m_history[historySampleIdx * nChannels + ch];
                return m_history[ch]; // clamp to oldest history (stream start)
            };

            const float pm1 = fetch(pm1FrameIdx);
            const float p0 = fetch(m_iPos);
            const float p1 = fetch(p1FrameIdx);
            const float p2 = fetch(p2FrameIdx);

            output[outputFramePos * nChannels + ch] =
                    basis.interpolate(pm1, p0, p1, p2);
        }

        maxConsumedFrames =
                std::max(maxConsumedFrames,
                         m_iPos + 3 - m_inputFramesConsumed);
        advancePosition();
        ++outputFramePos;
    }

    // Update history with last 3 frames of consumed portion
    qsizetype framesToCache = std::min(maxConsumedFrames, inputFrames);
    // If the main loop produced no output but there is input (m_iPos lags
    // behind m_inputFramesConsumed at a chunk boundary), consume all input into history so
    // the caller makes progress. The flush path interpolates using history.
    if (framesToCache == 0 && inputFrames > 0)
        framesToCache = inputFrames;
    namespace ranges = QtMultimediaPrivate::ranges;
    if (framesToCache >= 3) {
        const qsizetype last3FramesSampleOffset = (framesToCache - 3) * nChannels;
        ranges::copy(take(drop(input, last3FramesSampleOffset), 3 * nChannels),
                     m_history.data());
    } else if (framesToCache > 0) {
        const qsizetype newCacheSampleCount = framesToCache * nChannels;
        const qsizetype retainedSampleCount = (3 - framesToCache) * nChannels;
        auto history = QSpan(m_history);
        ranges::copy(take(drop(history, newCacheSampleCount), retainedSampleCount),
                     history.begin());
        ranges::copy(take(input, newCacheSampleCount),
                     drop(history, retainedSampleCount).begin());
    }

    m_inputFramesConsumed += framesToCache;

    const qsizetype unconsumedInputFrames = inputFrames - framesToCache;
    return ResampleResult{
        take(drop(input, framesToCache * nChannels), unconsumedInputFrames * nChannels),
        take(output, outputFramePos * nChannels),
    };
}

void upmixMonoToStereo(QSpan<float> out, QSpan<const float> in,
                       UpmixScaling scaling) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    constexpr float kEqualPowerGain(M_SQRT1_2); // 1/sqrt(2)
    Q_ASSERT(out.size() == in.size() * 2);

    const qsizetype frames = in.size();
    const float gain = (scaling == UpmixScaling::EqualPower) ? kEqualPowerGain : 1.0f;

    for (qsizetype i = 0; i < frames; ++i) {
        const float s = in[i] * gain;
        out[i * 2 + 0] = s;
        out[i * 2 + 1] = s;
    }
}

QByteArray upmixMonoToStereo(QSpan<const float> input, UpmixScaling scaling)
{
    if (input.empty())
        return {};

    const qsizetype frames = input.size();
    QByteArray out{
        2 * frames * qsizetype(sizeof(float)),
        Qt::Initialization::Uninitialized,
    };

    upmixMonoToStereo(QSpan<float>(reinterpret_cast<float *>(out.data()), 2 * frames),
                      input, scaling);
    return out;
}

void downmixStereoToMono(QSpan<float> out, QSpan<const float> in,
                         DownmixScaling scaling) noexcept Q_DECL_NONBLOCKING_FUNCTION
{
    constexpr float kEqualPowerGain(M_SQRT1_2); // 1/sqrt(2)

    Q_ASSERT(in.size() % 2 == 0);
    Q_ASSERT(out.size() == in.size() / 2);

    const qsizetype frames = in.size() / 2;
    const float gain = (scaling == DownmixScaling::KeepPower) ? kEqualPowerGain : 0.5f;

    for (qsizetype i = 0; i < frames; ++i)
        out[i] = (in[i * 2 + 0] + in[i * 2 + 1]) * gain;
}

QByteArray downmixStereoToMono(QSpan<const float> input, DownmixScaling scaling)
{
    if (input.empty())
        return {};

    Q_ASSERT(input.size() % 2 == 0);

    const qsizetype frames = input.size() / 2;
    QByteArray out{
        frames * qsizetype(sizeof(float)),
        Qt::Initialization::Uninitialized,
    };

    downmixStereoToMono(QSpan<float>(reinterpret_cast<float *>(out.data()), frames),
                        input, scaling);
    return out;
}

} // namespace QAudioHelperInternal

#undef QT_MM_RESTRICT

// clang-format off
QT_MM_MAKE_STRING_RESOLVER( QAudioHelperInternal::NativeSampleFormat, QtMultimediaPrivate::EnumName,
                           (QAudioHelperInternal::NativeSampleFormat::uint8_t, "uint8_t")
                           (QAudioHelperInternal::NativeSampleFormat::int16_t, "int16_t")
                           (QAudioHelperInternal::NativeSampleFormat::int32_t, "int32_t")
                           (QAudioHelperInternal::NativeSampleFormat::int24_t_3b, "int24_t_3b")
                           (QAudioHelperInternal::NativeSampleFormat::int24_t_4b_low, "int24_t_4b_low")
                           (QAudioHelperInternal::NativeSampleFormat::float32_t, "float32_t")
                           );
// clang-format on

QT_MM_DEFINE_QDEBUG_ENUM(QAudioHelperInternal::NativeSampleFormat);

QT_END_NAMESPACE
