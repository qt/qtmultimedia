/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QTextStream>
#include <QString>
#include <QAudioFormat>

#include <QAudioDevice>
#include <QCameraDevice>
#include <qmediadevices.h>

#include <stdio.h>

QString formatToString(QAudioFormat::SampleFormat sampleFormat)
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

QString positionToString(QCameraDevice::Position position)
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

void printAudioDeviceInfo(QTextStream &out, const QAudioDevice &deviceInfo)
{
    const auto isDefault = deviceInfo.isDefault() ? "Yes" : "No";
    const auto preferredFormat = deviceInfo.preferredFormat();
    const auto supportedFormats = deviceInfo.supportedSampleFormats();
    out.setFieldWidth(30);
    out.setFieldAlignment(QTextStream::AlignLeft);
    out << "Name: " << deviceInfo.description() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Id: " <<  QString::fromLatin1(deviceInfo.id()) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Default: " <<  isDefault << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Preferred Format: " << formatToString(preferredFormat.sampleFormat()) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Preferred Rate: " << preferredFormat.sampleRate() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Preferred Channels: " << preferredFormat.channelCount() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Supported Formats: ";
    for (auto &format: supportedFormats)
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

void printVideoDeviceInfo(QTextStream &out, const QCameraDevice &cameraDevice)
{
    const auto isDefault = cameraDevice.isDefault() ? "Yes" : "No";
    const auto position = cameraDevice.position();
    const auto photoResolutions = cameraDevice.photoResolutions();
    const auto videoFormats = cameraDevice.videoFormats();

    out.setFieldWidth(30);
    out.setFieldAlignment(QTextStream::AlignLeft);
    out << "Name: " << cameraDevice.description() << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Id: " <<  QString::fromLatin1(cameraDevice.id()) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Default: " <<  isDefault << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Position: " << positionToString(position) << qSetFieldWidth(0) << Qt::endl;
    out.setFieldWidth(30);
    out << "Photo Resolutions: ";
    for (auto &resolution: photoResolutions) {
        QString s = QString("%1x%2").arg(resolution.width()).arg(resolution.height());
        out << qSetFieldWidth(0) << s << ", ";
    }
    out.setFieldWidth(10);
    out << Qt::endl << Qt::endl;
    out << "Supported Video Formats: " << qSetFieldWidth(0) << Qt::endl;
    for (auto &format: videoFormats) {
        out.setFieldWidth(30);
        QString s = QString("%1x%2").arg(format.resolution().width()).arg(format.resolution().height());
        out << "Resolution: " << s << qSetFieldWidth(0) << Qt::endl;
        out.setFieldWidth(30);
        out << "Frame Rate: " << qSetFieldWidth(0) << "Min:" << format.minFrameRate() << " Max:" << format.maxFrameRate() << Qt::endl;
    }

    out << Qt::endl;

}


int main(int argc, char *argv[])
{
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    QTextStream out(stdout);

    const auto audioInputDevices = QMediaDevices::audioInputs();
    const auto audioOutputDevices = QMediaDevices::audioOutputs();
    const auto videoInputDevices = QMediaDevices::videoInputs();

    out << "Audio devices detected: " << Qt::endl;
    out << Qt::endl << "Input" << Qt::endl;
    for (auto &deviceInfo: audioInputDevices)
        printAudioDeviceInfo(out, deviceInfo);
    out << Qt::endl << "Output" << Qt::endl;
    for (auto &deviceInfo: audioOutputDevices)
        printAudioDeviceInfo(out, deviceInfo);

    out << Qt::endl << "Video devices detected: " << Qt::endl;
    for (auto &cameraDevice : videoInputDevices)
        printVideoDeviceInfo(out, cameraDevice);

    return 0;
}
