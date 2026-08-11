// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QTimer>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QUrl>
#include <QtCore/QDateTime>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtMultimedia/QCameraDevice>
#include <QtMultimedia/QCameraFormat>
#include <QtMultimedia/QAudioDevice>
#include <QtMultimedia/QMediaCaptureSession>
#include <QtMultimedia/QMediaRecorder>
#include <QtMultimedia/QMediaFormat>
#include <QtMultimedia/QMediaDevices>
#include <QtMultimedia/QCamera>
#include <QtMultimedia/QAudioInput>
#include <QtMultimedia/QVideoFrameFormat>
#include <algorithm>
#include <chrono>
#include <signal.h>
#include <filesystem>
#include <optional>

using namespace std::chrono_literals;
using namespace Qt::Literals;

namespace {

struct CommandLineArgs
{
    std::optional<std::chrono::seconds> recordingDuration;
    std::optional<std::filesystem::path> outputPath;
    std::optional<QMediaFormat::VideoCodec> videoCodec;
    std::optional<QVideoFrameFormat::PixelFormat> cameraPixelFormat;
    std::optional<QSize> resolution;
};

QString generateOutputPath()
{
    QDateTime now = QDateTime::currentDateTime();
    QString filename = u"recording_%1.mp4"_s.arg(now.toString(u"yyyyMMdd_HHmmss"_s));
    QString moviesPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    return QDir(moviesPath).filePath(filename);
}

} // namespace

std::optional<CommandLineArgs> parseCommandLine(QCoreApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription(u"Minimal Camera Recorder"_s);
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption durationOption{
        QList{ u"d"_s, u"duration"_s },
        u"Duration of recording in seconds"_s,
        u"seconds"_s,
        u"5"_s,
    };
    parser.addOption(durationOption);

    const QCommandLineOption outputOption{
        QList{ u"o"_s, u"output"_s },
        u"Output video file path"_s,
        u"path"_s,
    };
    parser.addOption(outputOption);

    const QCommandLineOption videoCodecOption{
        QList{ u"c"_s, u"video-codec"_s },
        u"Video codec (h264, h265, vp8, vp9, av1, mjpeg, mpeg1, mpeg2, mpeg4, theora)"_s,
        u"codec"_s,
    };
    parser.addOption(videoCodecOption);

    const QCommandLineOption cameraPixelFormatOption{
        QList{ u"p"_s, u"camera-pixel-format"_s },
        u"Camera pixel format (yuyv, uyvy, nv12, nv21, jpeg, yuv420p, yuv422p, y8, y16, "
        u"argb8888, xrgb8888)"_s,
        u"format"_s,
    };
    parser.addOption(cameraPixelFormatOption);

    const QCommandLineOption resolutionOption{
        QList{ u"r"_s, u"resolution"_s },
        u"Camera resolution, e.g. 1280x720"_s,
        u"WxH"_s,
    };
    parser.addOption(resolutionOption);

    parser.process(app);

    CommandLineArgs args;

    // Parse duration
    bool ok = false;
    int durationSeconds = parser.value(durationOption).toInt(&ok);
    if (ok && durationSeconds > 0) {
        args.recordingDuration = std::chrono::seconds(durationSeconds);
    } else {
        qWarning() << "Invalid duration value, using default 5 seconds";
        args.recordingDuration = std::chrono::seconds(5);
    }

    // Parse output path
    if (parser.isSet(outputOption))
        args.outputPath = std::filesystem::path(parser.value(outputOption).toStdString());
    else
        args.outputPath = std::filesystem::path(generateOutputPath().toStdString());

    // Parse video codec
    if (parser.isSet(videoCodecOption)) {
        QString codecStr = parser.value(videoCodecOption).toLower();
        if (codecStr == u"h264"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::H264;
        } else if (codecStr == u"h265"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::H265;
        } else if (codecStr == u"vp8"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::VP8;
        } else if (codecStr == u"vp9"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::VP9;
        } else if (codecStr == u"av1"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::AV1;
        } else if (codecStr == u"mjpeg"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::MotionJPEG;
        } else if (codecStr == u"mpeg1"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::MPEG1;
        } else if (codecStr == u"mpeg2"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::MPEG2;
        } else if (codecStr == u"mpeg4"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::MPEG4;
        } else if (codecStr == u"theora"_s) {
            args.videoCodec = QMediaFormat::VideoCodec::Theora;
        } else {
            qWarning() << "Unknown video codec:" << codecStr << ", using H264";
        }
    }

    // Parse camera pixel format
    if (parser.isSet(cameraPixelFormatOption)) {
        QString pixelFormatStr = parser.value(cameraPixelFormatOption).toLower();
        if (pixelFormatStr == u"yuyv"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_YUYV;
        } else if (pixelFormatStr == u"uyvy"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_UYVY;
        } else if (pixelFormatStr == u"nv12"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_NV12;
        } else if (pixelFormatStr == u"nv21"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_NV21;
        } else if (pixelFormatStr == u"jpeg"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_Jpeg;
        } else if (pixelFormatStr == u"yuv420p"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_YUV420P;
        } else if (pixelFormatStr == u"yuv422p"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_YUV422P;
        } else if (pixelFormatStr == u"y8"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_Y8;
        } else if (pixelFormatStr == u"y16"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_Y16;
        } else if (pixelFormatStr == u"argb8888"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_ARGB8888;
        } else if (pixelFormatStr == u"xrgb8888"_s) {
            args.cameraPixelFormat = QVideoFrameFormat::Format_XRGB8888;
        } else {
            qWarning() << "Unknown camera pixel format:" << pixelFormatStr;
            return std::nullopt;
        }
    }

    // Parse resolution
    if (parser.isSet(resolutionOption)) {
        QString resolutionStr = parser.value(resolutionOption);
        QStringList parts = resolutionStr.split(u'x', Qt::SkipEmptyParts);
        bool widthOk = false;
        bool heightOk = false;
        int width = parts.size() == 2 ? parts[0].toInt(&widthOk) : 0;
        int height = parts.size() == 2 ? parts[1].toInt(&heightOk) : 0;
        if (!widthOk || !heightOk || width <= 0 || height <= 0) {
            qWarning() << "Invalid resolution:" << resolutionStr << ", expected format WxH";
            return std::nullopt;
        }
        args.resolution = QSize(width, height);
    }

    return args;
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    std::optional<CommandLineArgs> args = parseCommandLine(app);
    if (!args)
        return 1;

    std::chrono::seconds duration = args->recordingDuration.value_or(std::chrono::seconds(5));
    std::filesystem::path outputPath =
            args->outputPath.value_or(std::filesystem::path(generateOutputPath().toStdString()));

    QString outputPathStr = QString::fromStdString(outputPath.string());
    QUrl mediaUrl = QUrl::fromLocalFile(outputPathStr);

    auto camera = QCamera(QMediaDevices::defaultVideoInput());
    auto audioInput = QAudioInput(QMediaDevices::defaultAudioInput());

    qInfo() << "recording from camera:" << camera.cameraDevice().description();
    qInfo() << "recording from audio device:" << audioInput.device().description();

    if (args->cameraPixelFormat || args->resolution) {
        const QList<QCameraFormat> formats = camera.cameraDevice().videoFormats();
        auto it = std::find_if(formats.begin(), formats.end(), [&](const QCameraFormat &format) {
            if (args->cameraPixelFormat && format.pixelFormat() != *args->cameraPixelFormat)
                return false;
            if (args->resolution && format.resolution() != *args->resolution)
                return false;
            return true;
        });
        if (it == formats.end()) {
            qInfo() << "Requested camera format not supported. Supported formats:";
            for (const QCameraFormat &format : formats) {
                qInfo() << " " << format.pixelFormat() << format.resolution()
                        << format.minFrameRate() << "-" << format.maxFrameRate() << "fps";
            }
            return 1;
        }
        qInfo() << "selecting camera format:" << it->pixelFormat() << it->resolution()
                << it->minFrameRate() << "-" << it->maxFrameRate() << "fps";
        camera.setCameraFormat(*it);
    }

    // Create capture session and recorder
    QMediaCaptureSession session;
    session.setCamera(&camera);
    session.setAudioInput(&audioInput);

    camera.start();

    QMediaRecorder recorder;
    session.setRecorder(&recorder);
    recorder.setOutputLocation(mediaUrl);

    if (args->videoCodec) {
        auto format = recorder.mediaFormat();
        format.setVideoCodec(*args->videoCodec);
        recorder.setMediaFormat(format);
    }

    static QMediaRecorder &recorderRef = recorder; // for signal handler

    signal(SIGINT, [](int signal) {
        if (signal == SIGINT) {
            qInfo() << "Received SIGINT, stopping recording...";
            recorderRef.stop();
            if (qApp)
                qApp->quit();
        }
    });

    // Start recording
    recorder.record();

    qInfo() << "Recording" << duration.count() << "seconds of video to" << outputPathStr;
    qInfo() << "Actual location:" << recorder.actualLocation().toLocalFile();

    QTimer::singleShot(duration, &app, [&] {
        qInfo() << "Duration reached, stopping recording...";
        recorder.stop();
        QCoreApplication::quit();
    });

    QObject::connect(&recorder, &QMediaRecorder::recorderStateChanged, &app,
                     [&](QMediaRecorder::RecorderState state) {
        if (state == QMediaRecorder::StoppedState) {
            qInfo() << "Recording stopped";
            QCoreApplication::quit();
        }
    });

    return QCoreApplication::exec();
}
