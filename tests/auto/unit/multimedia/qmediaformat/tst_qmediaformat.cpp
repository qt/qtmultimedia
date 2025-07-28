// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qmimetype.h>
#include <QtCore/qmimedatabase.h>
#include <QDebug>
#include <QtMultimedia/qmediaformat.h>
#include <private/qplatformmediaformatinfo_p.h>
#include "qmockintegration.h"

Q_ENABLE_MOCK_MULTIMEDIA_PLUGIN

class tst_QMediaFormat : public QObject
{
    Q_OBJECT

public slots:
    void init() { //
        *m_formatInfo = {};
    }

private slots:
    // clang-format off
    void resolveForEncoding_setsVideoCodecToUnspecified_whenCalledWithNoFlags();
    void resolveForEncoding_setsVideoCodecToSpecificCodec_whenCalledWithRequiresVideoAndVideoSupported();
    void resolveForEncoding_setsVideoCodecToSpecificCodec_whenCalledWithRequiresVideoAndVideoNotSupported();
    void resolveForEncoding_resetsVideoCodecToUnspecified_whenCalledWithNoFlags();
    void resolveForEncoding_setsAudioCodecToSpecificCodec_whenCalledWithAnyFlags();
    void resolveForEncoding_setsFileFormatToSpecificFormat_whenCalledWithNoFlags();
    void resolveForEncoding_setsFileFormatToSpecificFormat_whenCalledWithRequiresVideoAndVideoSupported();
    void resolveForEncoding_setsFileFormatToSpecificFormat_whenCalledWithRequiresVideoAndVideoNotSupported();
    void resolveForEncoding_selectsFallback_whenPrimaryCodecIsNotAvailable();
    void mimetype_returnsExpectedMimeType_data();
    void mimetype_returnsExpectedMimeType();
    // clang-format on
private:
    QPlatformMediaFormatInfo *m_formatInfo = QMockIntegration::instance()->getWritableFormatInfo();
};

void tst_QMediaFormat::resolveForEncoding_setsVideoCodecToUnspecified_whenCalledWithNoFlags()
{
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::MP3, { QMediaFormat::AudioCodec::MP3 }, {} });

    QMediaFormat format;
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QCOMPARE_EQ(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
}

void tst_QMediaFormat::
        resolveForEncoding_setsVideoCodecToSpecificCodec_whenCalledWithRequiresVideoAndVideoSupported()
{
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::MPEG4, {}, { QMediaFormat::VideoCodec::H264 } });

    QMediaFormat format;
    format.resolveForEncoding(QMediaFormat::RequiresVideo);

    QCOMPARE_NE(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
}

void tst_QMediaFormat::
        resolveForEncoding_setsVideoCodecToSpecificCodec_whenCalledWithRequiresVideoAndVideoNotSupported()
{
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::MP3, { QMediaFormat::AudioCodec::MP3 }, {} });

    QMediaFormat format;
    format.resolveForEncoding(QMediaFormat::RequiresVideo);

    QCOMPARE_EQ(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
}

void tst_QMediaFormat::resolveForEncoding_resetsVideoCodecToUnspecified_whenCalledWithNoFlags()
{
    QMediaFormat format;
    format.resolveForEncoding(QMediaFormat::RequiresVideo);
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QCOMPARE_EQ(format.videoCodec(), QMediaFormat::VideoCodec::Unspecified);
}

void tst_QMediaFormat::resolveForEncoding_setsAudioCodecToSpecificCodec_whenCalledWithAnyFlags()
{
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::MP3, { QMediaFormat::AudioCodec::MP3 }, {} });

    m_formatInfo->encoders.append({ QMediaFormat::FileFormat::MPEG4,
                                    { QMediaFormat::AudioCodec::MP3 },
                                    { QMediaFormat::VideoCodec::H264 } });

    QMediaFormat audioFormat;
    audioFormat.resolveForEncoding(QMediaFormat::NoFlags);
    QCOMPARE_NE(audioFormat.audioCodec(), QMediaFormat::AudioCodec::Unspecified);

    QMediaFormat videoFormat;
    videoFormat.resolveForEncoding(QMediaFormat::RequiresVideo);
    QCOMPARE_NE(videoFormat.audioCodec(), QMediaFormat::AudioCodec::Unspecified);
}

void tst_QMediaFormat::resolveForEncoding_setsFileFormatToSpecificFormat_whenCalledWithNoFlags()
{
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::MP3, { QMediaFormat::AudioCodec::MP3 }, {} });

    QMediaFormat audioFormat;
    audioFormat.resolveForEncoding(QMediaFormat::NoFlags);
    QCOMPARE_NE(audioFormat.fileFormat(), QMediaFormat::FileFormat::UnspecifiedFormat);
}

void tst_QMediaFormat::
        resolveForEncoding_setsFileFormatToSpecificFormat_whenCalledWithRequiresVideoAndVideoSupported()
{
    m_formatInfo->encoders.append({ QMediaFormat::FileFormat::MPEG4,
                                    { QMediaFormat::AudioCodec::MP3 },
                                    { QMediaFormat::VideoCodec::H264 } });
    QMediaFormat videoFormat;
    videoFormat.resolveForEncoding(QMediaFormat::RequiresVideo);
    QCOMPARE_NE(videoFormat.fileFormat(), QMediaFormat::FileFormat::UnspecifiedFormat);
}

void tst_QMediaFormat::
        resolveForEncoding_setsFileFormatToSpecificFormat_whenCalledWithRequiresVideoAndVideoNotSupported()
{
    m_formatInfo->encoders.append({ QMediaFormat::FileFormat::MP3,
                                    { QMediaFormat::AudioCodec::MP3 },
                                    { QMediaFormat::VideoCodec::H264 } });
    QMediaFormat videoFormat;
    videoFormat.resolveForEncoding(QMediaFormat::RequiresVideo);
    QCOMPARE_EQ(videoFormat.fileFormat(), QMediaFormat::FileFormat::UnspecifiedFormat);
}

void tst_QMediaFormat::resolveForEncoding_selectsFallback_whenPrimaryCodecIsNotAvailable()
{
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::Wave, { QMediaFormat::AudioCodec::MP3 }, {} });
    m_formatInfo->encoders.append(
            { QMediaFormat::FileFormat::Mpeg4Audio, { QMediaFormat::AudioCodec::MP3 }, {} });

    QMediaFormat format;

    QMediaFormat f(QMediaFormat::Mpeg4Audio);
    format.setFileFormat(QMediaFormat::Mpeg4Audio);
    format.setAudioCodec(QMediaFormat::AudioCodec::Wave);
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QCOMPARE_EQ(format.fileFormat(), QMediaFormat::Mpeg4Audio);
    QCOMPARE_NE(format.audioCodec(), QMediaFormat::AudioCodec::Wave);

    format = {};
    format.setFileFormat(QMediaFormat::Wave);
    format.setAudioCodec(QMediaFormat::AudioCodec::AAC);
    format.resolveForEncoding(QMediaFormat::NoFlags);
    QCOMPARE_EQ(format.fileFormat(), QMediaFormat::Wave);
    QCOMPARE_EQ(format.audioCodec(), QMediaFormat::AudioCodec::MP3);
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
