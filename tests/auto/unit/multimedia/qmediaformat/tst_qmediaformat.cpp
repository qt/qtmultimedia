// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QDebug>
#include <QtMultimedia/qmediaformat.h>

class tst_QMediaFormat : public QObject
{
    Q_OBJECT

private slots:
    void testResolveForEncoding();
    void mimetype_returnsExpectedMimeType_data();
    void mimetype_returnsExpectedMimeType();
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

void tst_QMediaFormat::mimetype_returnsExpectedMimeType_data()
{
    QTest::addColumn<QMediaFormat::FileFormat>("format");
    QTest::addColumn<QMimeType>("mimetype");

    const QMimeDatabase db;

    // clang-format off
    QTest::addRow("WMV") << QMediaFormat::FileFormat::WMV << db.mimeTypeForName("video/x-ms-wmv");
    QTest::addRow("AVI") << QMediaFormat::FileFormat::AVI << db.mimeTypeForName("video/x-msvideo");
    QTest::addRow("Matroska") << QMediaFormat::FileFormat::Matroska << db.mimeTypeForName("video/x-matroska");
    QTest::addRow("MPEG4") << QMediaFormat::FileFormat::MPEG4 << db.mimeTypeForName("video/mp4");
    QTest::addRow("Ogg") << QMediaFormat::FileFormat::Ogg << db.mimeTypeForName("video/ogg");
    QTest::addRow("QuickTime") << QMediaFormat::FileFormat::QuickTime << db.mimeTypeForName("video/quicktime");
    QTest::addRow("WebM") << QMediaFormat::FileFormat::WebM << db.mimeTypeForName("video/webm");
    QTest::addRow("Mpeg4Audio") << QMediaFormat::FileFormat::Mpeg4Audio << db.mimeTypeForName("audio/mp4");
    QTest::addRow("AAC") << QMediaFormat::FileFormat::AAC << db.mimeTypeForName("audio/aac");
    QTest::addRow("WMA") << QMediaFormat::FileFormat::WMA << db.mimeTypeForName("audio/x-ms-wma");
    QTest::addRow("MP3") << QMediaFormat::FileFormat::MP3 << db.mimeTypeForName("audio/mpeg");
    QTest::addRow("FLAC") << QMediaFormat::FileFormat::FLAC << db.mimeTypeForName("audio/flac");
    QTest::addRow("Wave") << QMediaFormat::FileFormat::Wave << db.mimeTypeForName("audio/wav");
    // clang-format on
}

void tst_QMediaFormat::mimetype_returnsExpectedMimeType()
{
    // Purpose of this test is to make sure that the
    // mapping from FileFormat to MIME type remains intact
    // Note: The test assumes that the MIME database is the
    // ground truth, and will not detect missing MIME types
    // in the MIME database.
    QFETCH(const QMediaFormat::FileFormat, format);
    QFETCH(const QMimeType, mimetype);

    const QMediaFormat mediaFormat{ format };
    QCOMPARE_EQ(mediaFormat.mimeType(), mimetype);
}

QTEST_MAIN(tst_QMediaFormat)
#include "tst_qmediaformat.moc"
