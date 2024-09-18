// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIAGENERATOR_H
#define MEDIAGENERATOR_H

#include <QAudioFormat>
#include <QSize>
#include <chrono>
#include <vector>

QT_BEGIN_NAMESPACE
class QVideoFrame;
class QAudioBuffer;
QT_END_NAMESPACE

class AudioGenerator
{
public:
    struct Settings
    {
        std::chrono::milliseconds duration = std::chrono::seconds(5);
        std::vector<uint32_t> channelFrequencies = { 555 };
        std::chrono::milliseconds bufferDuration = std::chrono::milliseconds(100);
        QAudioFormat::SampleFormat sampleFormat = QAudioFormat::Int16;
        uint32_t sampleRate = 40000;
        QAudioFormat::ChannelConfig channelConfig = QAudioFormat::ChannelConfigStereo;
    };

    AudioGenerator(const Settings &settings);

    QAudioBuffer generate();

private:
    Settings m_settings;
    uint32_t m_index = 0;
    uint32_t m_sampleIndex = 0;
};

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
    };

    VideoGenerator(const Settings &settings);

    QVideoFrame generate();

private:
    Settings m_settings;
    uint32_t m_index = 0;
};

#endif // MEDIAGENERATOR_H
