// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/QCoreApplication>
#include <QtCore/QMimeType>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtMultimedia/QMediaFormat>

namespace {

void printContainerCodecs(QTextStream &out, QMediaFormat::ConversionMode mode)
{
    for (QMediaFormat::FileFormat format : QMediaFormat().supportedFileFormats(mode)) {
        QMediaFormat fmt(format);

        const QMimeType mime = fmt.mimeType();
        out << QMediaFormat::fileFormatName(format);
        if (mime.isValid())
            out << " [" << mime.suffixes().join(", ") << "]";
        out << "\n";

        QStringList videoCodecs;
        for (QMediaFormat::VideoCodec codec : fmt.supportedVideoCodecs(mode))
            videoCodecs << QMediaFormat::videoCodecName(codec);
        if (!videoCodecs.isEmpty())
            out << "  Video: " << videoCodecs.join(", ") << "\n";

        QStringList audioCodecs;
        for (QMediaFormat::AudioCodec codec : fmt.supportedAudioCodecs(mode))
            audioCodecs << QMediaFormat::audioCodecName(codec);
        if (!audioCodecs.isEmpty())
            out << "  Audio: " << audioCodecs.join(", ") << "\n";

        out << "\n";
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv); // QtMultimedia needs an application singleton

    QTextStream out(stdout);

    out << "=== Encoders ===\n\n";
    printContainerCodecs(out, QMediaFormat::Encode);
    out << "=== Decoders ===\n\n";
    printContainerCodecs(out, QMediaFormat::Decode);

    return 0;
}
