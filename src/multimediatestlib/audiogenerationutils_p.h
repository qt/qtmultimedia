// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef AUDIOGENERATIONUTILS_H
#define AUDIOGENERATIONUTILS_H


//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QAudioFormat>
#include <QAudioBuffer>
#include <QtCore/qmath.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>

QT_BEGIN_NAMESPACE

class SineWaveGenerator
{
public:
    static constexpr double tau = 2 * M_PI;

    SineWaveGenerator(std::uint32_t phase, std::uint32_t increment)
        : m_phase(phase), m_increment(increment)
    {
    }

    double getSample() const
    {
        constexpr double scale = static_cast<double>(UINT32_MAX);
        return std::sin(tau * (static_cast<double>(m_phase) / scale));
    }

    void advance()
    {
        m_phase += m_increment; // wraps naturally
    }

    double operator()()
    {
        double result = getSample();
        advance();
        return result;
    }

private:
    std::uint32_t m_phase;
    std::uint32_t m_increment;
};

class SineWaveSignal
{
    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = double;
        using difference_type = std::ptrdiff_t;
        using pointer = void;
        using reference = double;

        iterator(std::uint32_t phase, std::uint32_t increment) : m_generator(phase, increment) { }

        double operator*() const { return m_generator.getSample(); }

        iterator &operator++()
        {
            m_generator.advance();
            return *this;
        }

        bool operator!=(std::nullptr_t) const
        {
            return true; // infinite range
        }

    private:
        SineWaveGenerator m_generator;
    };

public:
    SineWaveSignal(double frequency, double sample_rate, double phase = 0.0)
    {
        constexpr auto scale = double(std::numeric_limits<std::uint32_t>::max());
        double normalizedPhase = std::fmod(phase, SineWaveGenerator::tau);
        if (normalizedPhase < 0.0)
            normalizedPhase += SineWaveGenerator::tau;
        m_phase = std::uint32_t((normalizedPhase / SineWaveGenerator::tau) * scale);
        m_increment = std::uint32_t((frequency / sample_rate) * scale);
    }

    iterator begin() const
    {
        return iterator{
            m_phase,
            m_increment,
        };
    }
    std::nullptr_t end() const { return nullptr; }

private:
    std::uint32_t m_phase;
    std::uint32_t m_increment;
};

inline QByteArray createSineWaveData(const QAudioFormat &format, std::chrono::microseconds duration,
                                     qint32 sampleIndex = 0, qreal frequency = 500,
                                     qreal volume = 0.8)
{
    if (!format.isValid())
        return {};

    const qint32 length = format.bytesForDuration(duration.count());

    QByteArray data(format.bytesForDuration(duration.count()), Qt::Uninitialized);
    unsigned char *ptr = reinterpret_cast<unsigned char *>(data.data());
    const auto end = ptr + length;

    auto writeNextFrame = [&](auto value) {
        Q_ASSERT(sizeof(value) == format.bytesPerSample());
        *reinterpret_cast<decltype(value) *>(ptr) = value;
        ptr += sizeof(value);
    };

    const double initialPhase = 2.0 * M_PI * frequency * sampleIndex / format.sampleRate();
    SineWaveSignal generator(frequency, format.sampleRate(), initialPhase);
    for (double rawSample : generator) {
        if (ptr >= end)
            break;
        const qreal x = rawSample * volume;
        for (int ch = 0; ch < format.channelCount(); ++ch) {
            switch (format.sampleFormat()) {
            case QAudioFormat::UInt8:
                writeNextFrame(static_cast<quint8>(std::round((1.0 + x) / 2 * 255)));
                break;
            case QAudioFormat::Int16:
                writeNextFrame(
                        static_cast<qint16>(std::round(x * std::numeric_limits<qint16>::max())));
                break;
            case QAudioFormat::Int32:
                writeNextFrame(
                        static_cast<qint32>(std::round(x * std::numeric_limits<qint32>::max())));
                break;
            case QAudioFormat::Float:
                writeNextFrame(static_cast<float>(x));
                break;
            case QAudioFormat::Unknown:
            case QAudioFormat::NSampleFormats:
                break;
            }
        }
    }

    Q_ASSERT(ptr == end);

    return data;
}

class AudioGenerator : public QObject
{
    Q_OBJECT
public:
    AudioGenerator()
    {
        m_format.setSampleFormat(QAudioFormat::UInt8);
        m_format.setSampleRate(8000);
        m_format.setChannelConfig(QAudioFormat::ChannelConfigMono);
    }

    void setFormat(const QAudioFormat &format)
    { //
        m_format = format;
    }

    void setBufferCount(int count)
    { //
        m_maxBufferCount = std::max(count, 1);
    }

    void setDuration(std::chrono::microseconds duration)
    { //
        m_duration = duration;
    }

    void setFrequency(qreal frequency)
    { //
        m_frequency = frequency;
    }

    void emitEmptyBufferOnStop()
    { //
        m_emitEmptyBufferOnStop = true;
    }

    QAudioBuffer createAudioBuffer()
    {
        const std::chrono::microseconds bufferDuration = m_duration * (m_bufferIndex + 1) / m_maxBufferCount
                - m_duration * m_bufferIndex / m_maxBufferCount;
        QByteArray data = createSineWaveData(m_format, bufferDuration, m_sampleIndex, m_frequency);
        Q_ASSERT(m_format.bytesPerSample());
        m_sampleIndex += data.size() / m_format.bytesPerSample();
        return QAudioBuffer(data, m_format);
    }

signals:
    void done();
    void audioBufferCreated(const QAudioBuffer &buffer);

public slots:
    void nextBuffer()
{
        if (m_bufferIndex == m_maxBufferCount) {
            emit done();
            if (m_emitEmptyBufferOnStop)
                emit audioBufferCreated({});
            return;
        }

        const QAudioBuffer buffer = createAudioBuffer();

        emit audioBufferCreated(buffer);
        ++m_bufferIndex;
    }

private:
    int m_maxBufferCount = 1;
    std::chrono::microseconds m_duration{ std::chrono::seconds{ 1 } };
    int m_bufferIndex = 0;
    QAudioFormat m_format;
    bool m_emitEmptyBufferOnStop = false;
    qreal m_frequency = 500.;
    qint32 m_sampleIndex = 0;
};

QT_END_NAMESPACE

#endif // AUDIOGENERATIONUTILS_H
