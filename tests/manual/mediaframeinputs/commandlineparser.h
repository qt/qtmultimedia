// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef COMMANDLINEPARSER_H
#define COMMANDLINEPARSER_H

#include <QCommandLineParser>

#include "mediagenerator.h"
#include "recordingrunner.h"

class CommandLineParser
{
public:
    struct Result
    {
        AudioGeneratorSettingsOpt audioGenerationSettings;
        VideoGeneratorSettingsOpt videoGenerationSettings;
        PushModeSettingsOpt pushModeSettings;
        QUrl outputLocation;
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
        QString streams;
        QString mode;
        QString outputLocation;
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
    };

    Options createOptions();

private:
    const QString m_videoStream = QStringLiteral("video");
    const QString m_audioStream = QStringLiteral("audio");
    const QString m_pullMode = QStringLiteral("pull");
    const QString m_pushMode = QStringLiteral("push");

    QStringList m_streams{ m_audioStream, m_videoStream };
    const QStringList m_modes{ m_pullMode, m_pushMode };

    const QStringList m_sampleFormatsStr = { QStringLiteral("UInt8"), QStringLiteral("Int16"),
                                             QStringLiteral("Int32"), QStringLiteral("Float") };

    const QList<QAudioFormat::ChannelConfig> m_channelConfigs = {
        QAudioFormat::ChannelConfigMono,          QAudioFormat::ChannelConfigStereo,
        QAudioFormat::ChannelConfig2Dot1,         QAudioFormat::ChannelConfig3Dot0,
        QAudioFormat::ChannelConfig3Dot1,         QAudioFormat::ChannelConfigSurround5Dot0,
        QAudioFormat::ChannelConfigSurround5Dot1, QAudioFormat::ChannelConfigSurround7Dot1,
    };

    const QStringList m_channelConfigsStr = {
        QStringLiteral("ChannelConfigMono"),          QStringLiteral("ChannelConfigStereo"),
        QStringLiteral("ChannelConfig2Dot1"),         QStringLiteral("ChannelConfig3Dot0"),
        QStringLiteral("ChannelConfig3Dot1"),         QStringLiteral("ChannelConfigSurround5Dot0"),
        QStringLiteral("ChannelConfigSurround5Dot1"), QStringLiteral("ChannelConfigSurround7Dot1")
    };

    AudioGenerator::Settings m_audioGenerationSettings;
    VideoGenerator::Settings m_videoGenerationSettings;
    PushModeSettings m_pushModeSettings;
    QUrl m_outputLocation;
    QString m_mode = m_pullMode;

    QCommandLineParser m_parser;
    Options m_options = createOptions(); // should be the last member
};

#endif // COMMANDLINEPARSER_H
