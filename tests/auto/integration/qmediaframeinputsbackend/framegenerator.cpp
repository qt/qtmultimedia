// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "framegenerator.h"
#include "../shared/audiogenerationutils.h"
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

void VideoGenerator::setPattern(ImagePattern pattern)
{
    m_pattern = pattern;
}

void VideoGenerator::setFrameCount(int count)
{
    m_maxFrameCount = count;
}

void VideoGenerator::setSize(QSize size)
{
    m_size = size;
}

void VideoGenerator::setFrameRate(double rate)
{
    m_frameRate = rate;
}

void VideoGenerator::setPeriod(milliseconds period)
{
    m_period = period;
}

void VideoGenerator::emitEmptyFrameOnStop()
{
    m_emitEmptyFrameOnStop = true;
}

static void fillColoredSquares(QImage& image)
{
    QList<QColor> colors = { Qt::red, Qt::green, Qt::blue, Qt::yellow };
    const int width = image.width();
    const int height = image.height();

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            const int colorX = i < width / 2 ? 0 : 1;
            const int colorY = j < height / 2 ? 0 : 1;
            const int colorIndex = colorX + 2 * colorY;
            image.setPixel(i, j, colors[colorIndex].rgb());
        }
    }
}

QVideoFrame VideoGenerator::createFrame()
{
    QImage image(m_size, QImage::Format_ARGB32);
    switch (m_pattern) {
    case ImagePattern::SingleColor:
        image.fill(colors[m_frameIndex % colors.size()]);
        break;
    case ImagePattern::ColoredSquares:
        fillColoredSquares(image);
        break;
    }

    QVideoFrame frame(image);

    if (m_frameRate)
        frame.setStreamFrameRate(*m_frameRate);

    if (m_period) {
        frame.setStartTime(duration_cast<microseconds>(*m_period).count() * m_frameIndex);
        frame.setEndTime(duration_cast<microseconds>(*m_period).count() * (m_frameIndex + 1));
    }

    return frame;
}

void VideoGenerator::nextFrame()
{
    if (m_frameIndex == m_maxFrameCount) {
        emit done();
        if (m_emitEmptyFrameOnStop)
            emit frameCreated({});
        return;
    }

    const QVideoFrame frame = createFrame();
    emit frameCreated(frame);
    ++m_frameIndex;
}

AudioGenerator::AudioGenerator()
{
    m_format.setSampleFormat(QAudioFormat::UInt8);
    m_format.setSampleRate(8000);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigMono);
}

void AudioGenerator::setFormat(const QAudioFormat &format)
{
    m_format = format;
}

void AudioGenerator::setBufferCount(int count)
{
    m_maxBufferCount = std::max(count, 1);
}

void AudioGenerator::setDuration(microseconds duration)
{
    m_duration = duration;
}

void AudioGenerator::setFrequency(qreal frequency)
{
    m_frequency = frequency;
}

void AudioGenerator::emitEmptyBufferOnStop()
{
    m_emitEmptyBufferOnStop = true;
}

QAudioBuffer AudioGenerator::createAudioBuffer()
{
    const microseconds bufferDuration = m_duration * (m_bufferIndex + 1) / m_maxBufferCount
            - m_duration * m_bufferIndex / m_maxBufferCount;
    QByteArray data = createSineWaveData(m_format, bufferDuration, m_sampleIndex, m_frequency);
    Q_ASSERT(m_format.bytesPerSample());
    m_sampleIndex += data.size() / m_format.bytesPerSample();
    return QAudioBuffer(data, m_format);
}

void AudioGenerator::nextBuffer()
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

QT_END_NAMESPACE

#include "moc_framegenerator.cpp"
