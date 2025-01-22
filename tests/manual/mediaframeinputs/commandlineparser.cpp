// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "commandlineparser.h"

#include <QApplication>

namespace {
bool contains(const QStringList &list, const QStringView &str)
{
    return list.contains(str, Qt::CaseInsensitive);
}

bool equals(const QStringView &a, const QStringView &b)
{
    return a.compare(b, Qt::CaseInsensitive) == 0;
}

template <typename Container, typename Separator, typename ToString>
QString join(const Container &container, const Separator &separator, ToString &&toString)
{
    QString result;
    for (const auto &element : container) {
        if (!result.isEmpty())
            result += separator;
        result += toString(element);
    }
    return result;
}

template <typename Numbers, typename Separator>
QString joinNumberElements(const Numbers &numbers, const Separator &separator)
{
    return join(numbers, separator, [](auto number) { return QString::number(number); });
}

template <typename Map, typename Separator>
QString joinMapStringValues(const Map &map, const Separator &separator)
{
    return join(map, separator, [](const auto &pair) { return pair.second; });
}
auto toUInt(const QString &value, bool &ok)
{
    return value.toUInt(&ok);
}

auto toUIntList(const QString &value, bool &ok)
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

auto toPositiveFloat(const QString &value, bool &ok)
{
    const float result = value.toFloat(&ok);
    ok = ok && result > 0;
    return result;
}

auto toUrl(const QString &value, [[maybe_unused]] bool &ok)
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

auto toEnum = [](const auto &enumMap, const QString &value, bool &ok) {
    auto found = std::find_if(enumMap.begin(), enumMap.end(),
                              [&](const auto &pair) { return equals(pair.second, value); });
    ok = found != enumMap.end();
    return ok ? found->first : decltype(found->first)();
};

template <typename Duration>
auto toMsCount(Duration duration)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

const char *noCheck()
{
    return nullptr;
};

} // namespace

CommandLineParser::CommandLineParser()
    : m_result{ AudioGeneratorSettings{}, VideoGeneratorSettings{}, PushModeSettings{}, {} }
{
    m_parser.setApplicationDescription(QStringLiteral(
            "The application tests QtMultimedia media frame inputs with media "
            "recording and shows a simplistic result preview via the media player."));
    m_parser.addHelpOption();
}

CommandLineParser::Result CommandLineParser::process()
{
    m_parser.process(*QApplication::instance());

    parseCommonSettings(); // parse common settings first
    parseAudioGeneratorSettings();
    parseVideoGeneratorSettings();
    parsePushModeSettings();
    parseRecorderSettings();

    return std::move(m_result);
}

void CommandLineParser::parseCommonSettings()
{
    if (auto duration = parsedValue(m_options.duration, noCheck, toUInt)) {
        m_result.audioGeneratorSettings->duration = std::chrono::milliseconds(*duration);
        m_result.videoGeneratorSettings->duration = std::chrono::milliseconds(*duration);
    }

    if (auto streams = parsedValue(m_options.streams, noCheck, toStreams, m_streams)) {
        if (!contains(*streams, m_audioStream))
            m_result.audioGeneratorSettings.reset();
        if (!contains(*streams, m_videoStream))
            m_result.videoGeneratorSettings.reset();
    }

    if (auto mode = parsedValue(m_options.mode, noCheck, toMode, m_modes))
        m_mode = *mode;

    if (!equals(m_mode, m_pushMode))
        m_result.pushModeSettings.reset();
}

void CommandLineParser::parseAudioGeneratorSettings()
{
    AudioGeneratorSettingsOpt &settings = m_result.audioGeneratorSettings;

    auto check = [&settings]() -> const char * {
        return settings ? nullptr : "audio stream is not enabled";
    };

    if (auto frequencies = parsedValue(m_options.channelFrequencies, check, toUIntList))
        settings->channelFrequencies = std::move(*frequencies);

    if (auto bufferDuration = parsedValue(m_options.audioBufferDuration, check, toUInt))
        settings->bufferDuration = std::chrono::milliseconds(*bufferDuration);

    if (auto format = parsedValue(m_options.sampleFormat, check, toEnum, m_sampleFormats))
        settings->sampleFormat = *format;

    if (auto config = parsedValue(m_options.channelConfig, check, toEnum, m_channelConfigs))
        settings->channelConfig = *config;
}

void CommandLineParser::parseVideoGeneratorSettings()
{
    VideoGeneratorSettingsOpt &settings = m_result.videoGeneratorSettings;

    auto check = [&settings]() -> const char * {
        return settings ? nullptr : "video stream is not enabled";
    };

    if (auto frameRate = parsedValue(m_options.frameRate, check, toUInt))
        settings->frameRate = *frameRate;

    if (auto resolution = parsedValue(m_options.resolution, check, toSize))
        settings->resolution = *resolution;

    if (auto patternWidth = parsedValue(m_options.framePatternWidth, check, toUInt))
        settings->patternWidth = *patternWidth;

    if (auto patternSpeed = parsedValue(m_options.framePatternSpeed, check, toUInt))
        settings->patternSpeed = *patternSpeed;
}

void CommandLineParser::parsePushModeSettings()
{
    PushModeSettingsOpt &settings = m_result.pushModeSettings;

    auto check = [&settings]() -> const char * {
        return settings ? nullptr : "push mode is not enabled";
    };

    if (auto producingRate = parsedValue(m_options.pushModeProducingRate, check, toPositiveFloat))
        settings->producingRate = *producingRate;

    if (auto maxQueueSize = parsedValue(m_options.pushModeMaxQueueSize, check, toUInt))
        settings->maxQueueSize = *maxQueueSize;
}

void CommandLineParser::parseRecorderSettings()
{
    RecorderSettings &settings = m_result.recorderSettings;

    if (auto location = parsedValue(m_options.outputLocation, noCheck, toUrl))
        settings.outputLocation = *location;

    if (auto frameRate = parsedValue(m_options.recorderFrameRate, noCheck, toUInt))
        settings.frameRate = *frameRate;

    if (auto resolution = parsedValue(m_options.recorderResolution, noCheck, toSize))
        settings.resolution = *resolution;

    settings.quality = parsedValue(m_options.recorderQuality, noCheck, toEnum, m_recorderQualities);
    settings.audioCodec = parsedValue(m_options.recorderAudioCodec, noCheck, toEnum, m_audioCodecs);
    settings.videoCodec = parsedValue(m_options.recorderVideoCodec, noCheck, toEnum, m_videoCodecs);
    settings.fileFormat = parsedValue(m_options.recorderFileFormat, noCheck, toEnum, m_fileFormats);
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
    Q_ASSERT(m_result.audioGeneratorSettings && m_result.videoGeneratorSettings
             && m_result.pushModeSettings);

    const AudioGeneratorSettings &audioGeneratorSettings = *m_result.audioGeneratorSettings;
    const VideoGeneratorSettings &videoGeneratorSettings = *m_result.videoGeneratorSettings;
    const PushModeSettings &pushModeSettings = *m_result.pushModeSettings;

    result.streams = addOption({ QStringLiteral("streams"),
                                 QStringLiteral("Types of generated streams (%1).\nDefaults to %2.")
                                         .arg(m_streams.join(u", "), m_streams.join(',')),
                                 QStringLiteral("list of enum") });
    result.mode = addOption({ QStringLiteral("mode"),
                              QStringLiteral("Media frames generation mode (%1).\nDefaults to %2.")
                                    .arg(m_modes.join(u", "), m_pullMode),
                              QStringLiteral("list of enum") });
    result.duration =
            addOption({ { QStringLiteral("generator.duration"), QStringLiteral("duration") },
                        QStringLiteral("Media duration.\nDefaults to %1.")
                                    .arg(toMsCount(audioGeneratorSettings.duration)),
                        QStringLiteral("ms") });
    result.channelFrequencies = addOption(
            { { QStringLiteral("generator.frequencies"), QStringLiteral("frequencies") },
              QStringLiteral("Generated audio frequencies for each channel. It's "
                             "possible to set one or several ones: x,y,z.\nDefaults to %1.")
                      .arg(joinNumberElements(audioGeneratorSettings.channelFrequencies, u",")),
              QStringLiteral("list of hz") });
    result.audioBufferDuration =
                addOption({ { QStringLiteral("generator.audioBufferDuration"),
                              QStringLiteral("audioBufferDuration") },
                            QStringLiteral("Duration of single audio buffer.\nDefaults to %1.")
                                    .arg(toMsCount(audioGeneratorSettings.bufferDuration)),
                        QStringLiteral("ms") });
    result.sampleFormat = addOption(
                { { QStringLiteral("generator.sampleFormat"), QStringLiteral("sampleFormat") },
              QStringLiteral("Generated audio sample format (%1).\nDefaults to %2.")
                          .arg(joinMapStringValues(m_sampleFormats, u", "),
                               m_sampleFormats.at(audioGeneratorSettings.sampleFormat)),
              QStringLiteral("enum") });
    result.channelConfig = addOption(
            { { QStringLiteral("generator.channelConfig"), QStringLiteral("channelConfig") },
              QStringLiteral("Generated audio channel config (%1).\nDefaults to %2.")
                      .arg(joinMapStringValues(m_channelConfigs, u", "),
                               m_channelConfigs.at(audioGeneratorSettings.channelConfig)),
              QStringLiteral("enum") });
    result.frameRate =
            addOption({ { QStringLiteral("generator.frameRate"), QStringLiteral("frameRate") },
                        QStringLiteral("Generated video frame rate.\nDefaults to %1.")
                                    .arg(videoGeneratorSettings.frameRate),
                        QStringLiteral("uint") });
    result.resolution = addOption(
                { { QStringLiteral("generator.resolution"), QStringLiteral("resolution") },
              QStringLiteral("Generated video frame resolution.\nDefaults to %1x%2.")
                          .arg(QString::number(videoGeneratorSettings.resolution.width()),
                               QString::number(videoGeneratorSettings.resolution.height())),
              QStringLiteral("WxH") });
    result.framePatternWidth =
            addOption({ { QStringLiteral("generator.framePatternWidth"),
                          QStringLiteral("framePatternWidth") },
                        QStringLiteral("Generated video frame patern pixel width.\nDefaults to %1.")
                          .arg(videoGeneratorSettings.patternWidth),
                        QStringLiteral("uint") });
    result.framePatternSpeed = addOption(
                { { QStringLiteral("generator.framePatternSpeed"),
                    QStringLiteral("framePatternSpeed") },
              QStringLiteral("Relative speed of moving the frame pattern.\nDefaults to %1.")
                          .arg(videoGeneratorSettings.patternSpeed),
              QStringLiteral("float") });
    result.pushModeProducingRate = addOption(
                { { QStringLiteral("generator.pushMode.producingRate"),
                    QStringLiteral("producingRate") },
              QStringLiteral(
                      "Relative factor of media producing rate in push mode.\nDefaults to %1.")
                          .arg(pushModeSettings.producingRate),
              QStringLiteral("positive float") });
    result.pushModeMaxQueueSize =
            addOption({ {
                                QStringLiteral("generator.pushMode.maxQueueSize"),
                                QStringLiteral("maxQueueSize"),
                        },
                        QStringLiteral("Max number of generated media frames in the queue on "
                                       "the front of media recorder.\nDefaults to %1.")
                                    .arg(pushModeSettings.maxQueueSize),
                        QStringLiteral("uint") });
    result.outputLocation = addOption(
            { { QStringLiteral("recorder.outputLocation"), QStringLiteral("outputLocation") },
              QStringLiteral("Output location for the media."),
              QStringLiteral("file or directory") });
    result.recorderFrameRate =
            addOption({ QStringLiteral("recorder.frameRate"),
                        QStringLiteral("Video frame rate for media recorder.\nNo default value."),
                        QStringLiteral("uint") });
    result.recorderQuality =
            addOption({ { QStringLiteral("recorder.quality"), QStringLiteral("quality") },
                        QStringLiteral("Quality of media recording (%1).\nNo default value.")
                                .arg(joinMapStringValues(m_recorderQualities, u", ")),
                        QStringLiteral("enum") });
    result.recorderResolution =
            addOption({ QStringLiteral("recorder.resolution"),
                        QStringLiteral("Video resolution for media recorder.\nNo default value."),
                        QStringLiteral("WxH") });
    result.recorderAudioCodec =
            addOption({ { QStringLiteral("recorder.audioCodec"), QStringLiteral("audioCodec") },
                        QStringLiteral("Audio codec for media recorder. \nNo default value."),
                        QStringLiteral("enum") });
    result.recorderVideoCodec =
            addOption({ { QStringLiteral("recorder.videoCodec"), QStringLiteral("videoCodec") },
                        QStringLiteral("Video codec for media recorder.\nNo default value."),
                        QStringLiteral("enum") });
    result.recorderFileFormat =
            addOption({ { QStringLiteral("recorder.fileFormat"), QStringLiteral("fileFormat") },
                        QStringLiteral("File format for media recorder.\nNo default value."),
                        QStringLiteral("enum") });

    return result;
}
