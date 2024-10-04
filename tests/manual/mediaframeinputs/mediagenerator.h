// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIAGENERATOR_H
#define MEDIAGENERATOR_H

#include <QAudioFormat>
#include <QSize>
#include <chrono>
#include <vector>
#include <optional>

QT_BEGIN_NAMESPACE
class QVideoFrame;
class QAudioBuffer;
QT_END_NAMESPACE

enum class MediaTimeGenerationMode {
    None,
    FrameRate = 0x1,
    TimeStamps = 0x2,
    FrameRateAndTimeStamps = FrameRate | TimeStamps
};

class AudioGenerator
{
public:
    struct Settings
    {
        std::chrono::milliseconds duration = std::chrono::seconds(5);
        std::vector<uint32_t> channelFrequencies = { 555 };
        std::chrono::microseconds bufferDuration = std::chrono::milliseconds(100);
        QAudioFormat::SampleFormat sampleFormat = QAudioFormat::Int16;
        uint32_t sampleRate = 40000;
        QAudioFormat::ChannelConfig channelConfig = QAudioFormat::ChannelConfigStereo;
        MediaTimeGenerationMode timeGenerationMode =
                MediaTimeGenerationMode::FrameRateAndTimeStamps;
    };

    AudioGenerator(const Settings &settings);

    QAudioBuffer generate();

    std::chrono::microseconds interval() const { return m_settings.bufferDuration; }

private:
    Settings m_settings;
    QAudioFormat m_format;
    uint32_t m_index = 0;
    uint32_t m_sampleIndex = 0;
};

using AudioGeneratorSettingsOpt = std::optional<AudioGenerator::Settings>;

class VideoGenerator
{
public:
    struct Settings
    {
        std::chrono::milliseconds duration = std::chrono::seconds(5);
        uint32_t frameRate = 30;
        QSize resolution = { 1024, 700 };
        uint32_t patternWidth = 20;
        float patternSpeed = 1.f;
        MediaTimeGenerationMode timeGenerationMode = MediaTimeGenerationMode::TimeStamps;
    };

    VideoGenerator(const Settings &settings);

    QVideoFrame generate();

    std::chrono::microseconds interval() const
    {
        return std::chrono::microseconds(std::chrono::seconds(1)) / m_settings.frameRate;
    }

private:
    Settings m_settings;
    uint32_t m_index = 0;
};

using VideoGeneratorSettingsOpt = std::optional<VideoGenerator::Settings>;

#endif // MEDIAGENERATOR_H
