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

    if (auto format =
                parsedValue(m_options.sampleFormat, checkAudioStream, toEnum, m_sampleFormats))
        m_audioGenerationSettings.sampleFormat = *format;

    if (auto config =
                parsedValue(m_options.channelConfig, checkAudioStream, toEnum, m_channelConfigs))
        m_audioGenerationSettings.channelConfig = *config;

    if (auto frameRate = parsedValue(m_options.frameRate, checkVideoStream, toUInt))
        m_videoGenerationSettings.frameRate = *frameRate;

    if (auto resolution = parsedValue(m_options.resolution, checkVideoStream, toSize))
        m_videoGenerationSettings.resolution = *resolution;

    if (auto patternWidth = parsedValue(m_options.framePatternWidth, checkVideoStream, toUInt))
        m_videoGenerationSettings.patternWidth = *patternWidth;

    if (auto patternSpeed = parsedValue(m_options.framePatternSpeed, checkVideoStream, toUInt))
        m_videoGenerationSettings.patternSpeed = *patternSpeed;

    if (auto producingRate =
                parsedValue(m_options.pushModeProducingRate, checkPushMode, toPositiveFloat))
        m_pushModeSettings.producingRate = *producingRate;

    if (auto maxQueueSize = parsedValue(m_options.pushModeMaxQueueSize, checkPushMode, toUInt))
        m_pushModeSettings.maxQueueSize = *maxQueueSize;

    // recording settings
    if (auto location = parsedValue(m_options.outputLocation, noCheck, toUrl))
        m_recorderSettings.outputLocation = *location;

    if (auto frameRate = parsedValue(m_options.recorderFrameRate, noCheck, toUInt))
        m_recorderSettings.frameRate = *frameRate;

    if (auto resolution = parsedValue(m_options.recorderResolution, noCheck, toSize))
        m_recorderSettings.resolution = *resolution;

    m_recorderSettings.quality =
            parsedValue(m_options.recorderQuality, noCheck, toEnum, m_recorderQualities);

    m_recorderSettings.audioCodec =
            parsedValue(m_options.recorderAudioCodec, noCheck, toEnum, m_audioCodecs);

    m_recorderSettings.videoCodec =
            parsedValue(m_options.recorderVideoCodec, noCheck, toEnum, m_videoCodecs);

    m_recorderSettings.fileFormat =
            parsedValue(m_options.recorderFileFormat, noCheck, toEnum, m_fileFormats);

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

    result.recorderSettings = std::move(m_recorderSettings);

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
    result.duration =
            addOption({ { QStringLiteral("generator.duration"), QStringLiteral("duration") },
                        QStringLiteral("Media duration.\nDefaults to %1.")
                                .arg(toMsCount(m_audioGenerationSettings.duration)),
                        QStringLiteral("ms") });
    result.channelFrequencies = addOption(
            { { QStringLiteral("generator.frequencies"), QStringLiteral("frequencies") },
              QStringLiteral("Generated audio frequencies for each channel. It's "
                             "possible to set one or several ones: x,y,z.\nDefaults to %1.")
                      .arg(joinNumberElements(m_audioGenerationSettings.channelFrequencies, u",")),
              QStringLiteral("list of hz") });
    result.audioBufferDuration =
                addOption({ { QStringLiteral("generator.audioBufferDuration"),
                              QStringLiteral("audioBufferDuration") },
                            QStringLiteral("Duration of single audio buffer.\nDefaults to %1.")
                                .arg(toMsCount(m_audioGenerationSettings.bufferDuration)),
                        QStringLiteral("ms") });
    result.sampleFormat = addOption(
                { { QStringLiteral("generator.sampleFormat"), QStringLiteral("sampleFormat") },
              QStringLiteral("Generated audio sample format (%1).\nDefaults to %2.")
                          .arg(joinMapStringValues(m_sampleFormats, u", "),
                               m_sampleFormats.at(m_audioGenerationSettings.sampleFormat)),
              QStringLiteral("enum") });
    result.channelConfig = addOption(
            { { QStringLiteral("generator.channelConfig"), QStringLiteral("channelConfig") },
              QStringLiteral("Generated audio channel config (%1).\nDefaults to %2.")
                      .arg(joinMapStringValues(m_channelConfigs, u", "),
                           m_channelConfigs.at(m_audioGenerationSettings.channelConfig)),
              QStringLiteral("enum") });
    result.frameRate =
            addOption({ { QStringLiteral("generator.frameRate"), QStringLiteral("frameRate") },
                        QStringLiteral("Generated video frame rate.\nDefaults to %1.")
                                .arg(m_videoGenerationSettings.frameRate),
                        QStringLiteral("uint") });
    result.resolution = addOption(
                { { QStringLiteral("generator.resolution"), QStringLiteral("resolution") },
              QStringLiteral("Generated video frame resolution.\nDefaults to %1x%2.")
                      .arg(QString::number(m_videoGenerationSettings.resolution.width()),
                           QString::number(m_videoGenerationSettings.resolution.height())),
              QStringLiteral("WxH") });
    result.framePatternWidth =
            addOption({ { QStringLiteral("generator.framePatternWidth"),
                          QStringLiteral("framePatternWidth") },
                        QStringLiteral("Generated video frame patern pixel width.\nDefaults to %1.")
                                .arg(m_videoGenerationSettings.patternWidth),
                        QStringLiteral("uint") });
    result.framePatternSpeed = addOption(
                { { QStringLiteral("generator.framePatternSpeed"),
                    QStringLiteral("framePatternSpeed") },
              QStringLiteral("Relative speed of moving the frame pattern.\nDefaults to %1.")
                      .arg(m_videoGenerationSettings.patternSpeed),
              QStringLiteral("float") });
    result.pushModeProducingRate = addOption(
                { { QStringLiteral("generator.pushMode.producingRate"),
                    QStringLiteral("producingRate") },
              QStringLiteral(
                      "Relative factor of media producing rate in push mode.\nDefaults to %1.")
                      .arg(m_pushModeSettings.producingRate),
              QStringLiteral("positive float") });
    result.pushModeMaxQueueSize =
            addOption({ {
                                QStringLiteral("generator.pushMode.maxQueueSize"),
                                QStringLiteral("maxQueueSize"),
                        },
                        QStringLiteral("Max number of generated media frames in the queue on "
                                       "the front of media recorder.\nDefaults to %1.")
                                .arg(m_pushModeSettings.maxQueueSize),
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
