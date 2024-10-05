// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIAGENERATOR_H
#define MEDIAGENERATOR_H

#include "settings.h"

#include <QAudioFormat>
#include <QSize>
#include <chrono>
#include <vector>
#include <optional>

QT_BEGIN_NAMESPACE
class QVideoFrame;
class QAudioBuffer;
QT_END_NAMESPACE

class AudioGenerator
{
public:
    using Settings = AudioGeneratorSettings;

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
    using Settings = VideoGeneratorSettings;

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

#endif // MEDIAGENERATOR_H
