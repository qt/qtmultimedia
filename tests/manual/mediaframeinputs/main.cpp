// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QApplication>
#include <QCommandLineParser>

#include "previewrunner.h"
#include "recordingrunner.h"

#include <optional>

template <typename F>
auto cmdParserValue(const QCommandLineParser &parser, QString key, F &&f)
        -> std::optional<std::invoke_result_t<F, QString, bool &>>
{
    if (!parser.isSet(key))
        return {};

    const QString value = parser.value(key);
    if (value.isEmpty())
        return {};

    bool ok = true;
    const auto result = f(value, ok);
    if (!ok)
        qWarning() << "Invalid value" << key << ":" << value;

    return result;
}

struct CommandLineParseResult
{
    std::optional<AudioGenerator::Settings> audioGenerationSettings;
    std::optional<VideoGenerator::Settings> videoGenerationSettings;
    QUrl outputLocation;
};

auto toUInt(const QString &value, bool &ok)
{
    return value.toUInt(&ok);
}

auto toChannelFrequencies(const QString &value, bool &ok)
{
    std::vector<uint32_t> result;
    const QStringList split = value.split(',');
    ok = !split.isEmpty();

    for (auto it = split.cbegin(); ok && it != split.cend(); ++it)
        result.push_back(it->toUInt(&ok));

    return result;
}

auto toSize(const QString &value, bool &ok)
{
    const QStringList xy = value.split('x');
    ok = xy.size() == 2;
    QSize result;
    if (ok)
        result.setWidth(xy.front().toUInt(&ok));
    if (ok)
        result.setHeight(xy.back().toUInt(&ok));

    return result;
}

auto toFloat(const QString &value, bool &ok)
{
    return value.toFloat(&ok);
}

auto toStreams(const QString &value, bool &ok)
{
    const QStringList result = value.split(',');
    ok = !result.empty() && std::all_of(result.cbegin(), result.cend(), [](const QString &s) {
        return s.compare(QStringView(u"audio"), Qt::CaseInsensitive) == 0
                || s.compare(QStringView(u"video"), Qt::CaseInsensitive) == 0;
    });
    return result;
}

auto toUrl(const QString &value, bool &ok)
{
    return QUrl::fromLocalFile(value);
}

static CommandLineParseResult parseCommandLine()
{
    QCommandLineParser cmdParser;

    AudioGenerator::Settings audioGenerationSettings;
    VideoGenerator::Settings videoGenerationSettings;
    QUrl outputLocation;
    QStringList streams{ QStringLiteral("audio"), QStringLiteral("video") };

    cmdParser.setApplicationDescription(QStringLiteral("Media Frame Inputs Test"));

    cmdParser.addOption(
            { QStringLiteral("streams"),
              QStringLiteral("Types of generated streams (%1).").arg(streams.join(u", ")),
              QStringLiteral("list of enum; defaults to %1").arg(streams.join(',')) });

    cmdParser.addOption({ QStringLiteral("outputLocation"),
                          QStringLiteral("Output location for the media."),
                          QStringLiteral("File or directory") });

    cmdParser.addOption(
            { QStringLiteral("duration"), QStringLiteral("Media duration."),
              QStringLiteral("ms; defaults to %1").arg(audioGenerationSettings.duration.count()) });

    QString defaultChannelFrequenciesStr;
    for (size_t i = 0; i < audioGenerationSettings.channelFrequencies.size(); ++i) {
        if (i != 0)
            defaultChannelFrequenciesStr += u",";
        defaultChannelFrequenciesStr +=
                QString::number(audioGenerationSettings.channelFrequencies[i]);
    }
    QString::number(audioGenerationSettings.channelFrequencies.front());

    cmdParser.addOption(
            { QStringLiteral("frequencies"),
              QStringLiteral("Generated audio frequencies for each channel. It's "
                             "possible to set one or several ones: x,y,z."),
              QStringLiteral("list of hz; defaults to %1").arg(defaultChannelFrequenciesStr) });

    cmdParser.addOption({ QStringLiteral("audioBufferDuration"),
                          QStringLiteral("Duration of single audio buffer."),
                          QStringLiteral("ms; defaults to %1")
                                  .arg(audioGenerationSettings.bufferDuration.count()) });

    const QStringList sampleFormatsStr = { QStringLiteral("UInt8"), QStringLiteral("Int16"),
                                           QStringLiteral("Int32"), QStringLiteral("Float") };

    cmdParser.addOption({
            QStringLiteral("sampleFormat"),
            QStringLiteral("Generated audio sample format (%1).").arg(sampleFormatsStr.join(u", ")),
            QStringLiteral("enum; defaults to %1")
                    .arg(sampleFormatsStr.at(audioGenerationSettings.sampleFormat - 1)),
    });

    const QList<QAudioFormat::ChannelConfig> channelConfigs = {
        QAudioFormat::ChannelConfigMono,          QAudioFormat::ChannelConfigStereo,
        QAudioFormat::ChannelConfig2Dot1,         QAudioFormat::ChannelConfig3Dot0,
        QAudioFormat::ChannelConfig3Dot1,         QAudioFormat::ChannelConfigSurround5Dot0,
        QAudioFormat::ChannelConfigSurround5Dot1, QAudioFormat::ChannelConfigSurround7Dot1,
    };

    const QStringList channelConfigsStr = {
        QStringLiteral("ChannelConfigMono"),          QStringLiteral("ChannelConfigStereo"),
        QStringLiteral("ChannelConfig2Dot1"),         QStringLiteral("ChannelConfig3Dot0"),
        QStringLiteral("ChannelConfig3Dot1"),         QStringLiteral("ChannelConfigSurround5Dot0"),
        QStringLiteral("ChannelConfigSurround5Dot1"), QStringLiteral("ChannelConfigSurround7Dot1")
    };

    cmdParser.addOption({
            QStringLiteral("channelConfig"),
            QStringLiteral("Generated audio channel config (%1).")
                    .arg(channelConfigsStr.join(u", ")),
            QStringLiteral("enum; defaults to %1")
                    .arg(channelConfigsStr.at(
                            channelConfigs.indexOf(audioGenerationSettings.channelConfig))),
    });

    cmdParser.addOption(
            { QStringLiteral("frameRate"), QStringLiteral("Generated video frame rate."),
              QStringLiteral("uint, defaults to %1").arg(videoGenerationSettings.frameRate) });

    cmdParser.addOption(
            { QStringLiteral("resolution"), QStringLiteral("Generated video frame resolution."),
              QStringLiteral("WxH; defaults to %1x%2")
                      .arg(QString::number(videoGenerationSettings.resolution.width()),
                           QString::number(videoGenerationSettings.resolution.height())) });

    cmdParser.addOption(
            { QStringLiteral("framePatternWidth"),
              QStringLiteral("Generated video frame patern pixel width."),
              QStringLiteral("uint, defaults to %1").arg(videoGenerationSettings.patternWidth) });

    cmdParser.addOption(
            { QStringLiteral("framePatternSpeed"),
              QStringLiteral("Relative speed of moving the frame pattern."),
              QStringLiteral("float, defaults to %1").arg(videoGenerationSettings.patternSpeed) });

    cmdParser.addOption({
            QStringLiteral("help"),
            QStringLiteral("Print help."),
    });

    cmdParser.process(*QApplication::instance());

    if (cmdParser.isSet(QStringLiteral("help")))
        cmdParser.showHelp(); // exits

    if (auto duration = cmdParserValue(cmdParser, QStringLiteral("duration"), toUInt)) {
        audioGenerationSettings.duration = std::chrono::milliseconds(*duration);
        videoGenerationSettings.duration = std::chrono::milliseconds(*duration);
    }

    if (auto frequencies =
                cmdParserValue(cmdParser, QStringLiteral("frequencies"), toChannelFrequencies))
        audioGenerationSettings.channelFrequencies = std::move(*frequencies);

    if (auto bufferDuration =
                cmdParserValue(cmdParser, QStringLiteral("audioBufferDuration"), toUInt))
        audioGenerationSettings.bufferDuration = std::chrono::milliseconds(*bufferDuration);

    auto toSampleFormat = [&](const QString &value, bool &ok) {
        const qsizetype index = sampleFormatsStr.indexOf(value, 0, Qt::CaseInsensitive);
        ok = index >= 0;
        return QAudioFormat::SampleFormat(index + 1);
    };

    if (auto format = cmdParserValue(cmdParser, QStringLiteral("sampleFormat"), toSampleFormat))
        audioGenerationSettings.sampleFormat = *format;

    auto toChannelConfig = [&](const QString &value, bool &ok) {
        const qsizetype index = channelConfigsStr.indexOf(value, 0, Qt::CaseInsensitive);
        ok = index >= 0;
        return ok ? channelConfigs.at(index) : QAudioFormat::ChannelConfigUnknown;
    };

    if (auto config = cmdParserValue(cmdParser, QStringLiteral("channelConfig"), toChannelConfig))
        audioGenerationSettings.channelConfig = *config;

    if (auto frameRate = cmdParserValue(cmdParser, QStringLiteral("frameRate"), toUInt))
        videoGenerationSettings.frameRate = *frameRate;

    if (auto resolution = cmdParserValue(cmdParser, QStringLiteral("resolution"), toSize))
        videoGenerationSettings.resolution = *resolution;

    if (auto patternWidth = cmdParserValue(cmdParser, QStringLiteral("framePatternWidth"), toUInt))
        videoGenerationSettings.patternWidth = *patternWidth;

    if (auto patternSpeed = cmdParserValue(cmdParser, QStringLiteral("framePatternSpeed"), toUInt))
        videoGenerationSettings.patternSpeed = *patternSpeed;

    if (auto location = cmdParserValue(cmdParser, QStringLiteral("outputLocation"), toUrl))
        outputLocation = *location;

    if (auto parsedStreams = cmdParserValue(cmdParser, QStringLiteral("streams"), toStreams))
        streams = *parsedStreams;

    CommandLineParseResult result;

    if (streams.contains(QStringView(u"audio"), Qt::CaseInsensitive))
        result.audioGenerationSettings = std::move(audioGenerationSettings);

    if (streams.contains(QStringView(u"video"), Qt::CaseInsensitive))
        result.videoGenerationSettings = std::move(videoGenerationSettings);

    result.outputLocation = std::move(outputLocation);

    return result;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    auto [audioGenerationSettings, videoGenerationSettings, outputLocation] = parseCommandLine();

    RecordingRunner recordingRunner(audioGenerationSettings, videoGenerationSettings);
    PreviewRunner previewRunner;

    QObject::connect(&recordingRunner, &RecordingRunner::finished, &app, [&]() {
        if (recordingRunner.recorder().error() == QMediaRecorder::NoError)
            previewRunner.run(recordingRunner.recorder().actualLocation());
        else
            QMetaObject::invokeMethod(&app, &QApplication::quit, Qt::QueuedConnection);
    });

    QObject::connect(&previewRunner, &PreviewRunner::finished, &app, &QApplication::quit,
                     Qt::QueuedConnection);

    recordingRunner.run(outputLocation);

    return app.exec();
}
