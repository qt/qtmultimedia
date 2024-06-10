// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QMimeType>
#include <QtCore/QTextStream>
#include <QtMultimedia/QMediaFormat>

#include <stdio.h>

namespace {

void printFileFormatEntry(QMediaFormat::FileFormat format, QTextStream &out)
{
    out << "    " << QMediaFormat::fileFormatName(format) << " - "
        << QMediaFormat::fileFormatDescription(format);

    QMimeType mimeType = QMediaFormat(format).mimeType();
    if (mimeType.isValid()) {
        out << " (" << mimeType.name() << ")\n";
        out << "        " << mimeType.suffixes().join(", ") << "\n";
    }
}

void printCodecEntry(QMediaFormat::AudioCodec codec, QTextStream &out)
{
    out << "    " << QMediaFormat::audioCodecName(codec) << " - "
        << QMediaFormat::audioCodecDescription(codec) << "\n";
}

void printCodecEntry(QMediaFormat::VideoCodec codec, QTextStream &out)
{
    out << "    " << QMediaFormat::videoCodecName(codec) << " - "
        << QMediaFormat::videoCodecDescription(codec) << "\n";
}

void printFileFormats(QTextStream &out)
{
    out << "Supported file formats for decoding: \n";
    for (QMediaFormat::FileFormat format :
         QMediaFormat().supportedFileFormats(QMediaFormat::Decode))
        printFileFormatEntry(format, out);

    out << "\nSupported file formats for encoding: \n";
    for (QMediaFormat::FileFormat format :
         QMediaFormat().supportedFileFormats(QMediaFormat::Encode))
        printFileFormatEntry(format, out);
}

void printAudioCodecs(QTextStream &out)
{
    out << "Supported audio codecs for decoding: \n";
    for (QMediaFormat::AudioCodec codec : QMediaFormat().supportedAudioCodecs(QMediaFormat::Decode))
        printCodecEntry(codec, out);

    out << "\nSupported audio codecs for encoding: \n";
    for (QMediaFormat::AudioCodec codec : QMediaFormat().supportedAudioCodecs(QMediaFormat::Encode))
        printCodecEntry(codec, out);
}

void printVideoCodecs(QTextStream &out)
{
    out << "Supported video codecs for decoding: \n";
    for (QMediaFormat::VideoCodec codec : QMediaFormat().supportedVideoCodecs(QMediaFormat::Decode))
        printCodecEntry(codec, out);

    out << "\nSupported video codecs for encoding: \n";
    for (QMediaFormat::VideoCodec codec : QMediaFormat().supportedVideoCodecs(QMediaFormat::Encode))
        printCodecEntry(codec, out);
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv); // QtMultimedia needs an application singleton

    QTextStream out(stdout);

    printFileFormats(out);
    out << "\n";
    printAudioCodecs(out);
    out << "\n";
    printVideoCodecs(out);

    return 0;
}
