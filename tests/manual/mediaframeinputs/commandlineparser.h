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
        AudioGeneratorSettingsOpt audioGeneratorSettings;
        VideoGeneratorSettingsOpt videoGeneratorSettings;
        PushModeSettingsOpt pushModeSettings;
        RecorderSettings recorderSettings;
    };

    CommandLineParser();

    Result process();

private:
    template <typename ContextChecker, typename... ResultGetter>
    auto parsedValue(const QString &option, ContextChecker &&contextChecker,
                     ResultGetter &&...resultGetter)
            -> std::optional<decltype(std::invoke(resultGetter..., QString(),
                                                  std::declval<bool &>()))>;

    QString addOption(QCommandLineOption option);

    void parseCommonSettings();

    void parseAudioGeneratorSettings();

    void parseVideoGeneratorSettings();

    void parsePushModeSettings();

    void parseRecorderSettings();


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
        QString recorderAudioCodec;
        QString recorderVideoCodec;
        QString recorderFileFormat;
    };

    Options createOptions();

private:
    const QString m_videoStream = QStringLiteral("video");
    const QString m_audioStream = QStringLiteral("audio");
    const QString m_pullMode = QStringLiteral("pull");
    const QString m_pushMode = QStringLiteral("push");

    const QStringList m_streams{ m_audioStream, m_videoStream };
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

    const EnumMap<QMediaFormat::AudioCodec> m_audioCodecs = {
        { QMediaFormat::AudioCodec::MP3, u"mp3" },
        { QMediaFormat::AudioCodec::AAC, u"aac" },
        { QMediaFormat::AudioCodec::AC3, u"ac3" },
        { QMediaFormat::AudioCodec::EAC3, u"eac3" },
        { QMediaFormat::AudioCodec::FLAC, u"flac" },
        { QMediaFormat::AudioCodec::DolbyTrueHD, u"dolbyTrueHD" },
        { QMediaFormat::AudioCodec::Opus, u"opus" },
        { QMediaFormat::AudioCodec::Vorbis, u"vorbis" },
        { QMediaFormat::AudioCodec::Wave, u"wave" },
        { QMediaFormat::AudioCodec::WMA, u"wma" },
        { QMediaFormat::AudioCodec::ALAC, u"alac" },
    };

    const EnumMap<QMediaFormat::VideoCodec> m_videoCodecs = {
        { QMediaFormat::VideoCodec::MPEG1, u"mpeg1" },
        { QMediaFormat::VideoCodec::MPEG2, u"mpeg2" },
        { QMediaFormat::VideoCodec::MPEG4, u"mpeg4" },
        { QMediaFormat::VideoCodec::H264, u"h264" },
        { QMediaFormat::VideoCodec::H265, u"h265" },
        { QMediaFormat::VideoCodec::VP8, u"vp8" },
        { QMediaFormat::VideoCodec::VP9, u"vp9" },
        { QMediaFormat::VideoCodec::AV1, u"av1" },
        { QMediaFormat::VideoCodec::Theora, u"theora" },
        { QMediaFormat::VideoCodec::WMV, u"wmv" },
        { QMediaFormat::VideoCodec::MotionJPEG, u"motionJPEG" },
    };

    const EnumMap<QMediaFormat::FileFormat> m_fileFormats = {
        { QMediaFormat::FileFormat::WMV, u"wmv" },
        { QMediaFormat::FileFormat::AVI, u"avi" },
        { QMediaFormat::FileFormat::Matroska, u"matroska" },
        { QMediaFormat::FileFormat::MPEG4, u"mpeg4" },
        { QMediaFormat::FileFormat::Ogg, u"ogg" },
        { QMediaFormat::FileFormat::QuickTime, u"quickTime" },
        { QMediaFormat::FileFormat::WebM, u"webm" },
        { QMediaFormat::FileFormat::Mpeg4Audio, u"mpeg4Audio" },
        { QMediaFormat::FileFormat::AAC, u"aac" },
        { QMediaFormat::FileFormat::WMA, u"wma" },
        { QMediaFormat::FileFormat::MP3, u"mp3" },
        { QMediaFormat::FileFormat::FLAC, u"flac" },
    };

    QString m_mode = m_pullMode;
    Result m_result;

    QCommandLineParser m_parser;
    Options m_options = createOptions(); // should be the last member
};

#endif // COMMANDLINEPARSER_H
