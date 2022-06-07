// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QDebug>
#include <QtMultimedia/qmediaformat.h>

class tst_QMediaFormat : public QObject
{
    Q_OBJECT

private slots:
    void testResolveForEncoding();
};

void tst_QMediaFormat::testResolveForEncoding()
{
    QMediaFormat format;

    auto hasVideoCodecs = !format.supportedVideoCodecs(QMediaFormat::Encode).isEmpty();
    bool hasWav = format.supportedFileFormats(QMediaFormat::Encode).contains(QMediaFormat::Wave);

    // Resolve codecs for audio only stream
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QVERIFY(format.audioCodec() != QMediaFormat::AudioCodec::Unspecified);
    QVERIFY(format.fileFormat() != QMediaFormat::FileFormat::UnspecifiedFormat);
    QVERIFY(format.videoCodec() == QMediaFormat::VideoCodec::Unspecified);

    // Resolve codecs for audio/video stream
    format.resolveForEncoding(QMediaFormat::RequiresVideo);
    QVERIFY(format.audioCodec() != QMediaFormat::AudioCodec::Unspecified);
    QVERIFY(format.fileFormat() != QMediaFormat::FileFormat::UnspecifiedFormat);
    if (hasVideoCodecs)
        QVERIFY(format.videoCodec() != QMediaFormat::VideoCodec::Unspecified);
    else
        QVERIFY(format.videoCodec() == QMediaFormat::VideoCodec::Unspecified);

    // Resolve again for audio only stream
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QVERIFY(format.videoCodec() == QMediaFormat::VideoCodec::Unspecified);

    // check some specific conditions
    if (hasWav) {
        QMediaFormat f(QMediaFormat::Mpeg4Audio);
        if (!f.supportedAudioCodecs(QMediaFormat::Encode).contains(QMediaFormat::AudioCodec::Wave)) {
            qDebug() << "testing!";
            format.setFileFormat(QMediaFormat::Mpeg4Audio);
            format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
            format.resolveForEncoding(QMediaFormat::NoFlags);
            QVERIFY(format.fileFormat() == QMediaFormat::Mpeg4Audio);
            QVERIFY(format.audioCodec() != QMediaFormat::AudioCodec::Wave);

            format = {};
            format.setFileFormat(QMediaFormat::Wave);
            format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
            format.resolveForEncoding(QMediaFormat::NoFlags);
            QVERIFY(format.fileFormat() == QMediaFormat::Wave);
            QVERIFY(format.audioCodec() == QMediaFormat::AudioCodec::Wave);
        }
    }

}

QTEST_MAIN(tst_QMediaFormat)
#include "tst_qmediaformat.moc"
