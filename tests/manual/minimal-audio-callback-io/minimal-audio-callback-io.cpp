// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtCore/private/qexpected_p.h>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QAudioSink>
#include <QtMultimedia/QAudioSource>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/private/qaudiosystem_p.h>
#include <QtMultimedia/private/qaudioringbuffer_p.h>
#include <QtMultimedia/private/qautoresetevent_p.h>

#include <ranges>
#include <variant>

#define DR_WAV_IMPLEMENTATION // header-only use
#define DRWAV_API static // emit symbols with hidden visibility
#define DRWAV_PRIVATE static
#define DR_WAV_NO_WCHAR
#include "dr_wav.h"

using namespace Qt::Literals;

QTextStream &operator<<(QTextStream &os, const QAudioFormat &fmt)
{
    QString str;
    QDebug dbg(&str);
    dbg << fmt;
    os << str;
    return os;
}

namespace {

namespace CLI {

struct IOConfiguration
{
    int deviceIndex;
    int bufferSize = 1024;

    std::optional<int> numberOfChannels;
    std::optional<int> sampleRate;
};

struct Output
{
    IOConfiguration config;
};

struct Input
{
    IOConfiguration config;
};

struct ListDevices
{
};

using Arguments = std::variant<ListDevices, Input, Output>;

Arguments parseArguments(const QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Minimal test for audio callback io.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption bufferSizeOption{
        { u"b"_s, u"buffer-size"_s },
        "Set the hardware buffer size in frames (integer). Used by 'input' and 'output' commands.",
        "size",
        "1024",
    };

    QCommandLineOption deviceIndexOption{
        { u"d"_s, u"device"_s },
        "Specify the device index (string). Used by 'input' and 'output' commands.",
        "index",
        "0",
    };

    QCommandLineOption samplerate{
        { u"s"_s, u"samplerate"_s },
        "Specify the sample rate. Used by 'input' and 'output' commands.",
        "samplerate",
        "-1",
    };

    QCommandLineOption channels{
        { u"c"_s, u"channels"_s },
        "Specify the number of channels rate. Used by 'input' and 'output' commands.",
        "channels",
        "-1",
    };

    parser.addOption(bufferSizeOption);
    parser.addOption(deviceIndexOption);
    parser.addOption(samplerate);
    parser.addOption(channels);
    parser.addPositionalArgument("command", "The command to execute: list-devices, input, output.");
    parser.process(app);

    const QStringList positionalArguments = parser.positionalArguments();

    if (positionalArguments.isEmpty()) {
        qCritical() << "Error: No command specified.";
        parser.showHelp(1); // Show help and exit with error code 1
    }

    const QString command =
            positionalArguments.first().toLower(); // Get the first positional arg as the command

    if (command == u"list-devices"_s)
        return ListDevices{};

    auto parseOptionalString = [&](const QCommandLineOption &option)
            -> q23::expected<std::optional<int>, std::error_code> {
        if (!parser.isSet(option))
            return std::nullopt;

        QString str = parser.value(option);
        bool ok = false;
        int value = str.toInt(&ok);
        if (!ok)
            return q23::unexpected(std::make_error_code(std::errc::invalid_argument));

        if (value == -1)
            return std::nullopt;
        return value;
    };

    auto bufferSize = parseOptionalString(bufferSizeOption);
    auto deviceIndex = parseOptionalString(deviceIndexOption);
    auto sampleRate = parseOptionalString(samplerate);
    auto channelCount = parseOptionalString(channels);

    if (!bufferSize || !deviceIndex || !sampleRate || !channelCount) {
        qCritical() << "Error: Invalid command line arguments.";
        parser.showHelp(1);
    }

    if (*bufferSize && *bufferSize <= 0) {
        qCritical() << "Error: Invalid buffer size '" << **bufferSize
                    << "'. Must be a positive integer.";
        parser.showHelp(1);
    }

    if (*deviceIndex && *deviceIndex < 0) {
        qCritical() << "Error: Invalid device'" << **deviceIndex
                    << "'. Must be a non-negative integer.";
        parser.showHelp(1);
    }

    if (*sampleRate && *sampleRate <= 0) {
        qCritical() << "Error: Invalid sample rate '" << **sampleRate
                    << "'. Must be a positive integer.";
        parser.showHelp(1);
    }

    if (*channelCount && *channelCount <= 0) {
        qCritical() << "Error: Invalid channel count '" << **channelCount
                    << "'. Must be a positive integer.";
        parser.showHelp(1);
    }

    IOConfiguration conf{
        .deviceIndex = *deviceIndex.value_or(0),
        .bufferSize = (*bufferSize).value_or(1024),
        .numberOfChannels = *channelCount,
        .sampleRate = *sampleRate,
    };

    if (command == u"input"_s)
        return Input{ conf };

    if (command == u"output"_s)
        return Output{ conf };

    qCritical() << "Error: Unknown command '" << command << "'.";
    parser.showHelp(1);
    Q_UNREACHABLE_RETURN(ListDevices{});
}

} // namespace CLI

void printAudioDeviceList(const QList<QAudioDevice> &devices, const QString &title = QString())
{
    QTextStream out(stdout);
    out << title << "\n";

    if (devices.isEmpty()) {
        out << "No audio devices found.\n";
        out.flush(); // Ensure output is immediate
        return;
    }

    int index = 0;
    for (const QAudioDevice &device : devices) {
        QString description = device.description(); // User-friendly name
        out << "\t" << index++ << ": " << description << "\n";
    }
    out.flush(); // Ensure output is displayed immediately
}

int runCommand(CLI::ListDevices)
{
    QList<QAudioDevice> inputs = QMediaDevices::audioInputs();
    QList<QAudioDevice> outputs = QMediaDevices::audioOutputs();

    printAudioDeviceList(inputs, u"Audio Inputs"_s);
    printAudioDeviceList(outputs, u"Audio Outputs"_s);
    return 0;
}

QAudioFormat makeAudioFormat(const QAudioDevice &device, const CLI::IOConfiguration &cfg)
{
    QAudioFormat format = device.preferredFormat();
    format.setSampleFormat(QAudioFormat::Float);
    if (cfg.sampleRate)
        format.setSampleRate(*cfg.sampleRate);
    if (cfg.numberOfChannels) {
        format.setChannelCount(*cfg.numberOfChannels);
        format.setChannelConfig(
                QAudioFormat::defaultChannelConfigForChannelCount(*cfg.numberOfChannels));
    }

    return format;
}

int runCommand(const CLI::Input &input)
{
    QTextStream out(stdout);

    QAudioDevice device =
            QMediaDevices::audioInputs().value(input.config.deviceIndex, QAudioDevice{});
    if (device.isNull()) {
        qCritical() << "Error: Invalid input device index" << input.config.deviceIndex;
        return 1;
    };

    QAudioFormat format = makeAudioFormat(device, input.config);

    out << "Opening " << device.description() << " with " << format << "\n";

    QAudioSource source(device, format);
    QPlatformAudioSource *platformSource = QPlatformAudioSource::get(source);
    platformSource->setHardwareBufferFrames(input.config.bufferSize);

    using namespace QtPrivate;
    drwav_data_format wavFormat{
        .container = drwav_container_w64,
        .format = DR_WAVE_FORMAT_IEEE_FLOAT,
        .channels = static_cast<drwav_uint32>(format.channelCount()),
        .sampleRate = static_cast<drwav_uint32>(format.sampleRate()),
        .bitsPerSample = 32,
    };

    drwav wav;

    QString fileName = QDir::tempPath() + u"/"_s
            + QUuid::createUuid().toString(QUuid::WithoutBraces) + u".wav"_s;

    drwav_bool32 fileOpen =
            drwav_init_file_write(&wav, fileName.toUtf8().constData(), &wavFormat, nullptr);
    if (!fileOpen) {
        qCritical() << "Error opening WAV file for writing:" << fileName;
        return 1;
    }

    auto closeFile = qScopeGuard([&] {
        if (!drwav_uninit(&wav))
            qCritical() << "Failed to close WAV file:" << fileName;
    });

    out << "Writing audio data to file: " << fileName << "\n";
    out.flush();

    QAudioRingBuffer<float> ringBuffer{ 96000 * 10 }; // 10s

    QAutoResetEvent bufferReadyEvent;
    bufferReadyEvent.callOnActivated([&] {
        ringBuffer.consumeSome([&](QSpan<float> data) {
            auto frames = data.size() / format.channelCount();

            drwav_write_pcm_frames(&wav, frames, data.data());

            QSpan<float> take = std::views::take(data, frames);
            return take;
        });
    });

    platformSource->start([&](QSpan<const float> output) mutable {
        ringBuffer.write(output);
        bufferReadyEvent.set();
    });

    if (source.error() != QAudio::NoError) {
        qCritical() << "Error starting audio sink:" << source.error();
        return 1;
    }

    return qApp->exec();
}

int runCommand(const CLI::Output &output)
{
    QAudioDevice device =
            QMediaDevices::audioOutputs().value(output.config.deviceIndex, QAudioDevice{});
    if (device.isNull()) {
        qCritical() << "Error: Invalid output device index" << output.config.deviceIndex;
        return 1;
    };

    QAudioFormat format = makeAudioFormat(device, output.config);
    QAudioSink sink(device, format);

    QPlatformAudioSink *platformSink = QPlatformAudioSink::get(sink);
    platformSink->setHardwareBufferFrames(output.config.bufferSize);

    float phaseIncrement = 2 * M_PI * 220.f / format.sampleRate(); // 220 Hz tone
    platformSink->start([&, phase = 0.f](QSpan<float> output) mutable {
        int channels = format.channelCount();

        while (!output.isEmpty()) {
            std::ranges::fill(std::views::take(output, channels), std::sin(phase));

            output = std::views::drop(output, channels);
            phase += phaseIncrement;
            if (phase >= 2 * float(M_PI))
                phase -= 2 * float(M_PI); // Wrap phase
        }
    });

    if (sink.error() != QAudio::NoError) {
        qCritical() << "Error starting audio sink:" << sink.error();
        return 1;
    }

    return qApp->exec();
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("Minimal test for audio callback io");
    QCoreApplication::setApplicationVersion("1.0");

    auto command = CLI::parseArguments(app);

    return std::visit([](auto &cmd) {
        return runCommand(cmd);
    }, command);
}
