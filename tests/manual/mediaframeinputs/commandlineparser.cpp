// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "commandlineparser.h"

#include <QApplication>

namespace {
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

auto toPositiveFloat(const QString &value, bool &ok)
{
    const float result = value.toFloat(&ok);
    ok = ok && result > 0;
    return result;
}

auto toUrl(const QString &value, bool &ok)
{
    return QUrl::fromLocalFile(value);
}

auto toStreams(const QStringList &streams, const QString &value, bool &ok)
{
    const QStringList result = value.split(',');
    ok = !result.empty() && std::all_of(result.cbegin(), result.cend(), [&](const QString &s) {
        return streams.contains(s, Qt::CaseInsensitive);
    });
    return result;
}

auto toMode(const QStringList &modes, const QString &value, bool &ok)
{
    ok = modes.contains(value, Qt::CaseInsensitive);
    return value;
}

auto toChannelConfig(const QList<QAudioFormat::ChannelConfig> &channelConfigs,
                     const QStringList &channelConfigsStr, const QString &value, bool &ok)
{
    const qsizetype index = channelConfigsStr.indexOf(value, 0, Qt::CaseInsensitive);
    ok = index >= 0;
    return ok ? channelConfigs.at(index) : QAudioFormat::ChannelConfigUnknown;
}

auto toSampleFormat(const QStringList &sampleFormatsStr, const QString &value, bool &ok)
{
    const qsizetype index = sampleFormatsStr.indexOf(value, 0, Qt::CaseInsensitive);
    ok = index >= 0;
    return QAudioFormat::SampleFormat(index + 1);
}

bool contains(const QStringList &list, const QString str)
{
    return list.contains(str, Qt::CaseInsensitive);
}

bool equals(const QString &a, const QString b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

template <typename Numbers, typename Separator>
QString joinNumbers(const Numbers &numbers, const Separator &separator)
{
    QString result;
    for (auto number : numbers) {
        if (!result.isEmpty())
            result += separator;
        result += QString::number(number);
    }
    return result;
}

template <typename Duration>
auto toMsCount(Duration duration)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

} // namespace

CommandLineParser::CommandLineParser()
{
    m_parser.setApplicationDescription(QStringLiteral(
            "The application tests QtMultimedia media frame inputs with media "
            "recording and shows a simplistic result preview via the media player."));
    m_parser.addHelpOption();
}

CommandLineParser::Result CommandLineParser::process()
{
    m_parser.process(*QApplication::instance());

    auto noCheck = []() -> const char * { return nullptr; };

    if (auto parsedStreams = parsedValue(m_options.streams, noCheck, toStreams, m_streams))
        m_streams = *parsedStreams;

    if (auto mode = parsedValue(m_options.mode, noCheck, toMode, m_modes))
        m_mode = *mode;

    auto checkAudioStream = [this]() -> const char * {
        return contains(m_streams, m_audioStream) ? nullptr : "audio stream is not enabled";
    };

    auto checkVideoStream = [this]() -> const char * {
        return contains(m_streams, m_videoStream) ? nullptr : "video stream is not enabled";
    };

    auto checkPushMode = [this]() -> const char * {
        return equals(m_mode, m_pushMode) ? nullptr : "push mode is not enabled";
    };

    if (auto duration = parsedValue(m_options.duration, noCheck, toUInt)) {
        m_audioGenerationSettings.duration = std::chrono::milliseconds(*duration);
        m_videoGenerationSettings.duration = std::chrono::milliseconds(*duration);
    }

    if (auto frequencies =
                parsedValue(m_options.channelFrequencies, checkAudioStream, toChannelFrequencies))
        m_audioGenerationSettings.channelFrequencies = std::move(*frequencies);

    if (auto bufferDuration = parsedValue(m_options.audioBufferDuration, checkAudioStream, toUInt))
        m_audioGenerationSettings.bufferDuration = std::chrono::milliseconds(*bufferDuration);

    if (auto format = parsedValue(m_options.sampleFormat, checkAudioStream, toSampleFormat,
                                  m_sampleFormatsStr))
        m_audioGenerationSettings.sampleFormat = *format;

    if (auto config = parsedValue(m_options.channelConfig, checkAudioStream, toChannelConfig,
                                  m_channelConfigs, m_channelConfigsStr))
        m_audioGenerationSettings.channelConfig = *config;

    if (auto frameRate = parsedValue(m_options.frameRate, checkVideoStream, toUInt))
        m_videoGenerationSettings.frameRate = *frameRate;

    if (auto resolution = parsedValue(m_options.resolution, checkVideoStream, toSize))
        m_videoGenerationSettings.resolution = *resolution;

    if (auto patternWidth = parsedValue(m_options.framePatternWidth, checkVideoStream, toUInt))
        m_videoGenerationSettings.patternWidth = *patternWidth;

    if (auto patternSpeed = parsedValue(m_options.framePatternSpeed, checkVideoStream, toUInt))
        m_videoGenerationSettings.patternSpeed = *patternSpeed;

    if (auto location = parsedValue(m_options.outputLocation, noCheck, toUrl))
        m_outputLocation = *location;

    if (auto producingRate =
                parsedValue(m_options.pushModeProducingRate, checkPushMode, toPositiveFloat))
        m_pushModeSettings.producingRate = *producingRate;

    if (auto maxQueueSize = parsedValue(m_options.pushModeMaxQueueSize, checkPushMode, toUInt))
        m_pushModeSettings.maxQueueSize = *maxQueueSize;

    return takeResult();
}

CommandLineParser::Result CommandLineParser::takeResult()
{
    Result result;

    if (contains(m_streams, m_audioStream))
        result.audioGenerationSettings = std::move(m_audioGenerationSettings);

    if (contains(m_streams, m_videoStream))
        result.videoGenerationSettings = std::move(m_videoGenerationSettings);

    if (equals(m_mode, m_pushMode))
        result.pushModeSettings = m_pushModeSettings;

    result.outputLocation = std::move(m_outputLocation);

    return result;
}

template <typename ContextChecker, typename... ResultGetter>
auto CommandLineParser::parsedValue(const QString &option, ContextChecker &&contextChecker,
                                    ResultGetter &&...resultGetter)
        -> std::optional<decltype(std::invoke(resultGetter..., QString(), std::declval<bool &>()))>
{
    Q_ASSERT(!option.isEmpty());

    if (!m_parser.isSet(option))
        return {};

    const QString value = m_parser.value(option);
    if (value.isEmpty())
        return {};

    bool ok = true;
    auto result = std::invoke(std::forward<ResultGetter>(resultGetter)..., value, ok);
    if (!ok) {
        qWarning() << "Invalid value for option" << option << ":" << value;
    } else if (const auto error = contextChecker()) {
        ok = false;
        qWarning() << error << ", so the option" << option << "will not be applied";
    }

    if (!ok)
        return {};

    return { std::move(result) };
}

QString CommandLineParser::addOption(QCommandLineOption option)
{
    m_parser.addOption(option);
    return option.names().constFirst();
}

CommandLineParser::Options CommandLineParser::createOptions()
{
    Options result;
    result.streams = addOption({ QStringLiteral("streams"),
                                 QStringLiteral("Types of generated streams (%1).\nDefaults to %2.")
                                         .arg(m_streams.join(u", "), m_streams.join(',')),
                                 QStringLiteral("list of enum") });
    result.mode = addOption({ QStringLiteral("mode"),
                              QStringLiteral("Media frames generation mode (%1).\nDefaults to %2.")
                                      .arg(m_modes.join(u", "), m_mode),
                              QStringLiteral("list of enum") });
    result.outputLocation = addOption({ QStringLiteral("outputLocation"),
                                        QStringLiteral("Output location for the media."),
                                        QStringLiteral("file or directory") });
    result.duration = addOption({ QStringLiteral("duration"),
                                  QStringLiteral("Media duration.\nDefaults to %1.")
                                          .arg(toMsCount(m_audioGenerationSettings.duration)),
                                  QStringLiteral("ms") });
    result.channelFrequencies = addOption(
            { QStringLiteral("frequencies"),
              QStringLiteral("Generated audio frequencies for each channel. It's "
                             "possible to set one or several ones: x,y,z.\nDefaults to %1.")
                      .arg(joinNumbers(m_audioGenerationSettings.channelFrequencies, u",")),
              QStringLiteral("list of hz") });
    result.audioBufferDuration =
            addOption({ QStringLiteral("audioBufferDuration"),
                        QStringLiteral("Duration of single audio buffer.\nDefaults to %1")
                                .arg(toMsCount(m_audioGenerationSettings.bufferDuration)),
                        QStringLiteral("ms") });
    result.sampleFormat = addOption(
            { QStringLiteral("sampleFormat"),
              QStringLiteral("Generated audio sample format (%1).\nDefaults to %2.")
                      .arg(m_sampleFormatsStr.join(u", "),
                           m_sampleFormatsStr.at(m_audioGenerationSettings.sampleFormat - 1)),
              QStringLiteral("enum") });
    result.channelConfig =
            addOption({ QStringLiteral("channelConfig"),
                        QStringLiteral("Generated audio channel config (%1).\nDefaults to %2.")
                                .arg(m_channelConfigsStr.join(u", "),
                                     m_channelConfigsStr.at(m_channelConfigs.indexOf(
                                             m_audioGenerationSettings.channelConfig))),
                        QStringLiteral("enum") });
    result.frameRate = addOption({ QStringLiteral("frameRate"),
                                   QStringLiteral("Generated video frame rate.\nDefaults to %1.")
                                           .arg(m_videoGenerationSettings.frameRate),
                                   QStringLiteral("uint") });
    result.resolution = addOption(
            { QStringLiteral("resolution"),
              QStringLiteral("Generated video frame resolution.\nDefaults to %1x%2.")
                      .arg(QString::number(m_videoGenerationSettings.resolution.width()),
                           QString::number(m_videoGenerationSettings.resolution.height())),
              QStringLiteral("WxH") });
    result.framePatternWidth =
            addOption({ QStringLiteral("framePatternWidth"),
                        QStringLiteral("Generated video frame patern pixel width.\nDefaults to %1.")
                                .arg(m_videoGenerationSettings.patternWidth),
                        QStringLiteral("uint") });
    result.framePatternSpeed = addOption(
            { QStringLiteral("framePatternSpeed"),
              QStringLiteral("Relative speed of moving the frame pattern.\nDefaults to %1.")
                      .arg(m_videoGenerationSettings.patternSpeed),
              QStringLiteral("float") });
    result.pushModeProducingRate = addOption(
            { QStringLiteral("pushModeProducingRate"),
              QStringLiteral(
                      "Relative factor of media producing rate in push mode.\nDefaults to %1.")
                      .arg(m_pushModeSettings.producingRate),
              QStringLiteral("positive float") });
    result.pushModeMaxQueueSize = addOption(
            { QStringLiteral("pushModeMaxQueueSize"),
              QStringLiteral("Max number of media frames in the local queue.\nDefaults to %1.")
                      .arg(m_pushModeSettings.maxQueueSize),
              QStringLiteral("uint") });
    return result;
}
