// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QAudioFormat>
#include <QMediaRecorder>
#include <QMediaFormat>
#include <QUrl>

#include <optional>
#include <vector>
#include <chrono>

enum class MediaTimeGenerationMode {
    None,
    FrameRate = 1,
    TimeStamps = 2,
    FrameRateAndTimeStamps = FrameRate | TimeStamps
};

struct AudioGeneratorSettings
{
    std::chrono::milliseconds duration = std::chrono::seconds(5);
    std::vector<uint32_t> channelFrequencies = { 555 };
    std::chrono::microseconds bufferDuration = std::chrono::milliseconds(100);
    QAudioFormat::SampleFormat sampleFormat = QAudioFormat::Int16;
    uint32_t sampleRate = 40000;
    QAudioFormat::ChannelConfig channelConfig = QAudioFormat::ChannelConfigStereo;
    MediaTimeGenerationMode timeGenerationMode = MediaTimeGenerationMode::FrameRateAndTimeStamps;
};

using AudioGeneratorSettingsOpt = std::optional<AudioGeneratorSettings>;

struct VideoGeneratorSettings
{
    std::chrono::milliseconds duration = std::chrono::seconds(5);
    uint32_t frameRate = 30;
    QSize resolution = { 1024, 700 };
    uint32_t patternWidth = 20;
    float patternSpeed = 1.f;
    MediaTimeGenerationMode timeGenerationMode = MediaTimeGenerationMode::TimeStamps;
};

using VideoGeneratorSettingsOpt = std::optional<VideoGeneratorSettings>;

struct RecorderSettings
{
    uint32_t frameRate = 0;
    QSize resolution;
    std::optional<QMediaRecorder::Quality> quality;
    QUrl outputLocation;
    std::optional<QMediaFormat::AudioCodec> audioCodec;
    std::optional<QMediaFormat::VideoCodec> videoCodec;
    std::optional<QMediaFormat::FileFormat> fileFormat;
};

struct PushModeSettings
{
    qreal producingRate = 1.;
    uint32_t maxQueueSize = 5;
};

using PushModeSettingsOpt = std::optional<PushModeSettings>;

#endif // SETTINGS_H
