// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include "settings.h"

#include <QCommandLineParser>
#include <unordered_map>

class CommandLineParser
{
public:
    struct Result
    {
        AudioGeneratorSettingsOpt audioGenerationSettings;
        VideoGeneratorSettingsOpt videoGenerationSettings;
        PushModeSettingsOpt pushModeSettings;
        RecorderSettings recorderSettings;
    };

    CommandLineParser();

    Result process();

private:
    Result takeResult();

    template <typename ContextChecker, typename... ResultGetter>
    auto parsedValue(const QString &option, ContextChecker &&contextChecker,
                     ResultGetter &&...resultGetter)
            -> std::optional<decltype(std::invoke(resultGetter..., QString(),
                                                  std::declval<bool &>()))>;

    QString addOption(QCommandLineOption option);

    struct Options
    {
        // common
        QString streams;
        QString mode;

        // generator
        QString duration;
        QString channelFrequencies;
        QString audioBufferDuration;
        QString sampleFormat;
        QString channelConfig;
        QString frameRate;
        QString resolution;
        QString framePatternWidth;
        QString framePatternSpeed;
        QString pushModeProducingRate;
        QString pushModeMaxQueueSize;

        // recorder
        QString outputLocation;
        QString recorderFrameRate;
        QString recorderQuality;
        QString recorderResolution;
    };

    Options createOptions();

private:
    const QString m_videoStream = QStringLiteral("video");
    const QString m_audioStream = QStringLiteral("audio");
    const QString m_pullMode = QStringLiteral("pull");
    const QString m_pushMode = QStringLiteral("push");

    QStringList m_streams{ m_audioStream, m_videoStream };
    const QStringList m_modes{ m_pullMode, m_pushMode };

    template <typename Enum>
    using EnumMap = std::unordered_map<Enum, QStringView>;

    const EnumMap<QAudioFormat::SampleFormat> m_sampleFormats = {
        { QAudioFormat::UInt8, u"uint8" },
        { QAudioFormat::Int16, u"int16" },
        { QAudioFormat::Int32, u"int32" },
        { QAudioFormat::Float, u"float" },
    };

    const EnumMap<QAudioFormat::ChannelConfig> m_channelConfigs = {
        { QAudioFormat::ChannelConfigMono, u"mono" },
        { QAudioFormat::ChannelConfigStereo, u"stereo" },
        { QAudioFormat::ChannelConfig2Dot1, u"2Dot1" },
        { QAudioFormat::ChannelConfig3Dot0, u"3Dot0" },
        { QAudioFormat::ChannelConfig3Dot1, u"3Dot1" },
        { QAudioFormat::ChannelConfigSurround5Dot0, u"surround5Dot0" },
        { QAudioFormat::ChannelConfigSurround5Dot1, u"surround5Dot1" },
        { QAudioFormat::ChannelConfigSurround7Dot1, u"surround7Dot1" }
    };

    const EnumMap<QMediaRecorder::Quality> m_recorderQualities = {
        { QMediaRecorder::VeryLowQuality, u"veryLow" },   { QMediaRecorder::LowQuality, u"low" },
        { QMediaRecorder::NormalQuality, u"normal" },     { QMediaRecorder::HighQuality, u"high" },
        { QMediaRecorder::VeryHighQuality, u"veryHigh" },
    };

    AudioGeneratorSettings m_audioGenerationSettings;
    VideoGeneratorSettings m_videoGenerationSettings;
    PushModeSettings m_pushModeSettings;
    RecorderSettings m_recorderSettings;
    QString m_mode = m_pullMode;

    QCommandLineParser m_parser;
    Options m_options = createOptions(); // should be the last member
};

#endif // COMMANDLINEPARSER_H
