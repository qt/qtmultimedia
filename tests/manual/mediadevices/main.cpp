// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioformat.h>
#include <QtMultimedia/qcameradevice.h>
#include <QtMultimedia/qmediadevices.h>

#include <QtGui/qguiapplication.h>

#include <QtCore/qcommandlineoption.h>
#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtCore/qset.h>
#include <QtCore/qstring.h>
#include <QtCore/qtextstream.h>
#include <QtCore/qthread.h>

#include <optional>

using namespace Qt::Literals;

struct CLIArgs
{
    bool audio; // covers both audio input and audio output
    bool video;
    bool monitor;
    bool listInThread;
};

struct DeviceSnapshot
{
    QList<QAudioDevice> audioInputs;
    QList<QAudioDevice> audioOutputs;
    QList<QCameraDevice> videoInputs;
};

static QString formatToString(QAudioFormat::SampleFormat sampleFormat)
{
    switch (sampleFormat) {
    case QAudioFormat::UInt8:
        return "UInt8";
    case QAudioFormat::Int16:
        return "Int16";
    case QAudioFormat::Int32:
        return "Int32";
    case QAudioFormat::Float:
        return "Float";
    default:
        return "Unknown";
    }
}

static QString positionToString(QCameraDevice::Position position)
{
    switch (position) {
    case QCameraDevice::BackFace:
        return "BackFace";
    case QCameraDevice::FrontFace:
        return "FrontFace";
    default:
        return "Unspecified";
    }
}

static void printAudioDeviceInfo(QTextStream &out, const QAudioDevice &deviceInfo)
{
    const auto isDefault = deviceInfo.isDefault() ? "Yes" : "No";
    const auto preferredFormat = deviceInfo.preferredFormat();
    const auto supportedFormats = deviceInfo.supportedSampleFormats();
    out.setFieldWidth(30);
    out.setFieldAlignment(QTextStream::AlignLeft);
    out << "Name: " << deviceInfo.description() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Id: " << QString::fromLatin1(deviceInfo.id()) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Default: " << isDefault << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Preferred Format: " << formatToString(preferredFormat.sampleFormat())
        << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Preferred Rate: " << preferredFormat.sampleRate() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Preferred Channels: " << preferredFormat.channelCount() << qSetFieldWidth(0)
        << Qt::endl;
    out.setFieldWidth(30);
    out.setIntegerBase(16);
    out << "Preferred Channel Config: " << preferredFormat.channelConfig() << qSetFieldWidth(0)
        << Qt::endl;
    out.setIntegerBase(10);
    out.setFieldWidth(30);
    out << "Supported Formats: ";
    for (auto &format : supportedFormats)
        out << qSetFieldWidth(0) << formatToString(format) << " ";
    out << Qt::endl;
    out.setFieldWidth(30);
    out << "Supported Rates: " << qSetFieldWidth(0) << deviceInfo.minimumSampleRate() << " - "
        << deviceInfo.maximumSampleRate() << Qt::endl;
    out.setFieldWidth(30);
    out << "Supported Channels: " << qSetFieldWidth(0) << deviceInfo.minimumChannelCount() << " - "
        << deviceInfo.maximumChannelCount() << Qt::endl;

    out << Qt::endl;
}

static void printVideoDeviceInfo(QTextStream &out, const QCameraDevice &cameraDevice)
{
    const auto isDefault = cameraDevice.isDefault() ? "Yes" : "No";
    const auto position = cameraDevice.position();
    const auto photoResolutions = cameraDevice.photoResolutions();
    const auto videoFormats = cameraDevice.videoFormats();

    out.setFieldWidth(30);
    out.setFieldAlignment(QTextStream::AlignLeft);
    out << "Name: " << cameraDevice.description() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Id: " << QString::fromLatin1(cameraDevice.id()) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Default: " << isDefault << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Position: " << positionToString(position) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Photo Resolutions: ";
    for (auto &resolution : photoResolutions) {
        QString s = QStringLiteral("%1x%2").arg(resolution.width()).arg(resolution.height());
        out << qSetFieldWidth(0) << s << ", ";
    }
    out.setFieldWidth(10);
    out << Qt::endl << Qt::endl;
    out << "Supported Video Formats: " << qSetFieldWidth(0) << Qt::endl;
    for (auto &format : videoFormats) {
        out.setFieldWidth(30);
        QString s =
                QStringLiteral("%1x%2").arg(format.resolution().width()).arg(format.resolution().height());
        out << "Resolution: " << s << qSetFieldWidth(0) << Qt::endl;
        out.setFieldWidth(30);
        out << "Frame Rate: " << qSetFieldWidth(0) << "Min:" << format.minFrameRate()
            << " Max:" << format.maxFrameRate() << Qt::endl;
        out.setFieldWidth(30);
        out << "Format: " << qSetFieldWidth(0)
            << QVideoFrameFormat::pixelFormatToString(format.pixelFormat()) << Qt::endl;
        out << Qt::endl;
    }

    out << Qt::endl;
}

static DeviceSnapshot listDevices(QTextStream &out, bool audio, bool video)
{
    DeviceSnapshot snapshot;

    if (audio) {
        snapshot.audioInputs = QMediaDevices::audioInputs();
        snapshot.audioOutputs = QMediaDevices::audioOutputs();

        out << "Audio devices detected: " << Qt::endl;
        out << Qt::endl << "Input" << Qt::endl;
        for (auto &deviceInfo : snapshot.audioInputs)
            printAudioDeviceInfo(out, deviceInfo);
        out << Qt::endl << "Output" << Qt::endl;
        for (auto &deviceInfo : snapshot.audioOutputs)
            printAudioDeviceInfo(out, deviceInfo);
    }

    if (video) {
        snapshot.videoInputs = QMediaDevices::videoInputs();

        out << Qt::endl << "Video devices detected: " << Qt::endl;
        for (auto &cameraDevice : snapshot.videoInputs)
            printVideoDeviceInfo(out, cameraDevice);
    }

    return snapshot;
}

// Diffs `previous` against `current` by device id, prints what was added
// (full info, via printAdded) and removed (id/description only), then
// updates `previous` to `current` as the baseline for the next change.
template <typename Device, typename PrintAddedFn>
static void reportDeviceListChange(QTextStream &out, QLatin1StringView label,
                                    QList<Device> &previous, const QList<Device> &current,
                                    PrintAddedFn printAdded)
{
    QSet<QByteArray> oldIds;
    for (const auto &device : previous)
        oldIds.insert(device.id());
    QSet<QByteArray> newIds;
    for (const auto &device : current)
        newIds.insert(device.id());

    out << label << " changed:" << Qt::endl;
    bool any = false;

    for (const auto &device : current) {
        if (!oldIds.contains(device.id())) {
            out << "  Added:" << Qt::endl;
            printAdded(out, device);
            any = true;
        }
    }
    for (const auto &device : previous) {
        if (!newIds.contains(device.id())) {
            out << "  Removed: " << device.description() << " ("
                << QString::fromLatin1(device.id()) << ")" << Qt::endl;
            any = true;
        }
    }
    // defaultAudioInput/Output/VideoInput notify via these same *Changed
    // signals, so a pure default-device switch reports no added/removed ids.
    if (!any)
        out << "  (no additions/removals -- likely a default-device change)" << Qt::endl;
    out << Qt::endl;

    previous = current;
}

std::optional<CLIArgs> parseArgs(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(u"List multimedia devices"_s);
    parser.addHelpOption();

    QCommandLineOption listInThreadOption{
        u"list-devices-in-thread"_s,
        u"List devices from a worker thread."_s,
    };
    parser.addOption(listInThreadOption);

    QCommandLineOption noAudioOption{
        u"no-audio"_s,
        u"Disable audio input/output device listing and monitoring."_s,
    };
    parser.addOption(noAudioOption);

    QCommandLineOption noVideoOption{
        u"no-video"_s,
        u"Disable video input device listing and monitoring."_s,
    };
    parser.addOption(noVideoOption);

    QCommandLineOption monitorOption{
        u"monitor"_s,
        u"Watch for device changes after the initial listing, until interrupted (Ctrl-C)."_s,
    };
    parser.addOption(monitorOption);

    parser.process(app);

    const bool noAudio = parser.isSet(noAudioOption);
    const bool noVideo = parser.isSet(noVideoOption);

    if (noAudio && noVideo) {
        qInfo() << "Cannot disable both audio and video";
        return std::nullopt;
    }

    return CLIArgs{
        !noAudio, !noVideo, parser.isSet(monitorOption), parser.isSet(listInThreadOption),
    };
}

int run(const CLIArgs &args)
{
    QTextStream out(stdout);
    DeviceSnapshot snapshot;

    if (args.listInThread) {
        QThread t;
        t.start();
        QObject o;
        o.moveToThread(&t);

        QMetaObject::invokeMethod(&o, [&] {
            return listDevices(out, args.audio, args.video);
        }, Qt::BlockingQueuedConnection, &snapshot);

        t.quit();
        t.wait();
    } else {
        snapshot = listDevices(out, args.audio, args.video);
    }

    if (!args.monitor)
        return 0;

    // Constructed here, on the main thread, after any worker thread above has
    // already been joined. Connecting each signal lazily activates backend
    // device-change monitoring for that category only, so this also verifies
    // that off-thread enumeration above didn't leave that activation broken.
    QMediaDevices devices;

    if (args.audio) {
        QObject::connect(&devices, &QMediaDevices::audioInputsChanged, &devices, [&] {
            reportDeviceListChange(out, "Audio inputs"_L1, snapshot.audioInputs,
                                    QMediaDevices::audioInputs(), printAudioDeviceInfo);
        });
        QObject::connect(&devices, &QMediaDevices::audioOutputsChanged, &devices, [&] {
            reportDeviceListChange(out, "Audio outputs"_L1, snapshot.audioOutputs,
                                    QMediaDevices::audioOutputs(), printAudioDeviceInfo);
        });
    }
    if (args.video) {
        QObject::connect(&devices, &QMediaDevices::videoInputsChanged, &devices, [&] {
            reportDeviceListChange(out, "Video inputs"_L1, snapshot.videoInputs,
                                    QMediaDevices::videoInputs(), printVideoDeviceInfo);
        });
    }

    qInfo() << "Monitoring device changes. Press Ctrl-C to quit.";
    return QCoreApplication::exec();
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_MACOS
    // QGuiApplication (not QCoreApplication) so the Cocoa platform plugin
    // loads a CFRunLoop-integrated event dispatcher on macOS -- without it,
    // AVFoundation/CoreAudio device hotplug callbacks (which are delivered
    // via the main run loop / main dispatch queue) are silently never
    // invoked, and --monitor never fires.
    using AppType = QGuiApplication;
#else
    using AppType = QCoreApplication;
#endif

    AppType app(argc, argv);

    std::optional<CLIArgs> args = parseArgs(app);
    if (!args)
        return 1;

    return run(*args);
}
