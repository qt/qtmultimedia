// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "mediagenerator.h"

#include <QVideoFrame>
#include <QAudioBuffer>
#include <QDebug>

namespace {

uint32_t channelFrequency(int channelIndex, const std::vector<uint32_t> &channelFrequencies)
{
    return channelFrequencies[channelIndex % channelFrequencies.size()];
}

double normalizedSineSampleValue(uint32_t frequency, uint32_t sampleIndex, uint32_t sampleRate)
{
    return sin(2 * M_PI * frequency * sampleIndex / sampleRate);
}

template <typename ResultHandler>
void toSampleValue(double normalizedValue, QAudioFormat::SampleFormat sampleFormat,
                   ResultHandler &&resultHandler)
{
    switch (sampleFormat)
    case QAudioFormat::UInt8: {
        resultHandler(static_cast<quint8>(qRound((1.0 + normalizedValue) / 2 * 255)));
        break;
    case QAudioFormat::Int16:
        resultHandler(
                static_cast<qint16>(qRound(normalizedValue * std::numeric_limits<qint16>::max())));
        break;
    case QAudioFormat::Int32: {
        resultHandler(
                static_cast<quint32>(qRound(normalizedValue * std::numeric_limits<qint32>::max())));
        break;
    }
    case QAudioFormat::Float:
        resultHandler(static_cast<float>(normalizedValue));
        break;
    case QAudioFormat::Unknown:
    case QAudioFormat::NSampleFormats:
        Q_ASSERT(!"Unknown audio sample format");
        break;
    }
}

QByteArray createSineWaveData(const QAudioFormat &format, uint32_t &sampleIndex,
                              std::chrono::microseconds duration,
                              const std::vector<uint32_t> &channelFrequencies)
{
    const int bytesPerSample = format.bytesPerSample();

    qint32 remainingBytes = format.bytesForDuration(duration.count());
    Q_ASSERT(bytesPerSample);
    Q_ASSERT((remainingBytes % bytesPerSample) == 0);
    Q_ASSERT(!channelFrequencies.empty());

    QByteArray result(remainingBytes, 0);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(result.data());

    auto writeNextSampleValue = [&](auto sampleValue) {
        Q_ASSERT(sizeof(sampleValue) == bytesPerSample);
        *reinterpret_cast<decltype(sampleValue) *>(ptr) = sampleValue;
        ptr += sizeof(sampleValue);
        remainingBytes -= sizeof(sampleValue);
    };

    while (remainingBytes) {
        for (int channelIndex = 0; channelIndex < format.channelCount(); ++channelIndex) {
            const uint32_t frequency = channelFrequency(channelIndex, channelFrequencies);
            const double normalizedSampleValue =
                    normalizedSineSampleValue(frequency, sampleIndex, format.sampleRate());

            toSampleValue(normalizedSampleValue, format.sampleFormat(), writeNextSampleValue);
        }
        ++sampleIndex;
    }

    return result;
}

QRgb imagePatternValue(int xIndex, int yIndex, double patternSpeed, uint32_t patternWidth,
                       uint32_t imageIndex)
{
    static const uint32_t availableColors[] = { qRgba(255, 0, 0, 0), qRgba(0, 255, 0, 0),
                                                qRgba(0, 0, 255, 0) };
    constexpr double angle = M_PI / 3;

    // inverse sin and cos as angle, otherise the angle will represent the orthogonal line
    static const double xFactor = sin(angle);
    static const double yFactor = cos(angle);

    const double value =
            (xIndex * xFactor + yIndex * yFactor + imageIndex * patternSpeed) / patternWidth;
    int colorIndex = qRound(value) % std::size(availableColors);
    if (colorIndex < 0)
        colorIndex += std::size(availableColors);
    return availableColors[colorIndex];
}

QImage createPatternImage(QSize size, uint32_t patternWidth, float patternSpeed,
                          uint32_t imageIndex)
{
    QImage image(size, QImage::Format_RGBA8888);

    uchar *imageData = image.bits();
    for (int yIndex = 0; yIndex < size.height(); ++yIndex) {
        auto colors = reinterpret_cast<uint32_t *>(imageData + yIndex * image.bytesPerLine());
        for (int xIndex = 0; xIndex < size.width(); ++xIndex)
            colors[xIndex] =
                    imagePatternValue(xIndex, yIndex, patternSpeed, patternWidth, imageIndex);
    }

    return image;
}

std::chrono::microseconds videoFrameTime(uint32_t frameIndex, uint32_t frameRate)
{
    using namespace std::chrono;
    return microseconds(seconds(1)) * frameIndex / frameRate;
}

} // namespace

AudioGenerator::AudioGenerator(const Settings &settings) : m_settings(settings)
{
    m_format.setSampleFormat(m_settings.sampleFormat);
    m_format.setSampleRate(m_settings.sampleRate);
    m_format.setChannelConfig(m_settings.channelConfig);

    // align duration
    m_settings.bufferDuration = std::chrono::microseconds(m_format.durationForBytes(
            m_format.bytesForDuration(m_settings.bufferDuration.count())));
}

QAudioBuffer AudioGenerator::generate()
{
    if (m_index == m_settings.duration / m_settings.bufferDuration)
        return {};

    const QByteArray sineData = createSineWaveData(
            m_format, m_sampleIndex, m_settings.bufferDuration, m_settings.channelFrequencies);

    qint64 startTime = -1;
    if (qToUnderlying(m_settings.timeGenerationMode)
        & qToUnderlying(MediaTimeGenerationMode::TimeStamps))
        startTime = std::chrono::microseconds(m_settings.bufferDuration * m_index).count();

    ++m_index;

    return QAudioBuffer(sineData, m_format, startTime);
}

VideoGenerator::VideoGenerator(const Settings &settings) : m_settings(settings) { }

QVideoFrame VideoGenerator::generate()
{
    if (m_index == m_settings.frameRate * m_settings.duration.count() / 1000)
        return {};

    QVideoFrame frame(createPatternImage(m_settings.resolution, m_settings.patternWidth,
                                         m_settings.patternSpeed, m_index));
    if (qToUnderlying(m_settings.timeGenerationMode)
        & qToUnderlying(MediaTimeGenerationMode::FrameRate))
        frame.setStreamFrameRate(m_settings.frameRate);

    if (qToUnderlying(m_settings.timeGenerationMode)
        & qToUnderlying(MediaTimeGenerationMode::TimeStamps)) {
        frame.setStartTime(videoFrameTime(m_index, m_settings.frameRate).count());
        frame.setEndTime(videoFrameTime(m_index + 1, m_settings.frameRate).count());
    }

    ++m_index;

    return frame;
}
