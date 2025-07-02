// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QAudioDevice>
#include <QAudioFormat>
#include <QCameraDevice>
#include <QCoreApplication>
#include <QMediaDevices>
#include <QString>
#include <QTextStream>

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

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv); // QtMultimedia needs an application singleton

    QTextStream out(stdout);

    const auto audioInputDevices = QMediaDevices::audioInputs();
    const auto audioOutputDevices = QMediaDevices::audioOutputs();
    const auto videoInputDevices = QMediaDevices::videoInputs();

    out << "Audio devices detected: " << Qt::endl;
    out << Qt::endl << "Input" << Qt::endl;
    for (auto &deviceInfo : audioInputDevices)
        printAudioDeviceInfo(out, deviceInfo);
    out << Qt::endl << "Output" << Qt::endl;
    for (auto &deviceInfo : audioOutputDevices)
        printAudioDeviceInfo(out, deviceInfo);

    out << Qt::endl << "Video devices detected: " << Qt::endl;
    for (auto &cameraDevice : videoInputDevices)
        printVideoDeviceInfo(out, cameraDevice);
}
