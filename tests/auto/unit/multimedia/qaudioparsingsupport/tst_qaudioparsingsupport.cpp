// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtCore/qbytearray.h>

#include <private/qaudio_parsing_support_p.h>

QT_USE_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QtMultimediaPrivate;
using namespace QtMultimediaPrivate::ParsingSupport;

Q_DECLARE_METATYPE(QtMultimediaPrivate::ParsingSupport::AudioCodec)

namespace {

QSpan<const std::byte> asBytes(const QByteArray &data)
{
    return as_bytes(QSpan{ data.constData(), data.size() });
}

// Builds an MPEG Layer III frame header.
// versionBits: 3 = MPEG1, 2 = MPEG2, 0 = MPEG2.5, 1 = reserved
// layerBits: 1 = Layer III
QByteArray makeMpegHeader(int versionBits, int layerBits, int bitrateIndex, int sampleRateIndex,
                          int padding)
{
    QByteArray header(4, '\0');
    header[0] = char(0xFF);
    header[1] = char(0xE0 | (versionBits << 3) | (layerBits << 1) | 0x1);
    header[2] = char((bitrateIndex << 4) | (sampleRateIndex << 2) | (padding << 1));
    header[3] = char(0x00);
    return header;
}

// MPEG1 Layer III, 128 kbps, 44100 Hz, no padding: 144 * 128000 / 44100 = 417 bytes.
QByteArray makeMpeg1Layer3Header(int padding = 0)
{
    return makeMpegHeader(3, 1, 9, 0, padding);
}

// Builds an ADTS AAC frame header with the given aac_frame_length.
QByteArray makeAdtsHeader(int frameLength)
{
    QByteArray header(7, '\0');
    header[0] = char(0xFF);
    header[1] = char(0xF1); // MPEG-4, Layer 00, no CRC
    header[2] = char(0x40);
    header[3] = char(0x40 | ((frameLength >> 11) & 0x03));
    header[4] = char((frameLength >> 3) & 0xFF);
    header[5] = char(((frameLength & 0x07) << 5) | 0x1F);
    header[6] = char(0xFC);
    return header;
}

} // namespace

class tst_QAudioParsingSupport : public QObject
{
    Q_OBJECT

private slots:
    void sniffCodec_detectsCodec_fromStreamMagic_data();
    void sniffCodec_detectsCodec_fromStreamMagic();
    void sniffCodec_returnsUnknown_whenHeaderIsTooShort();

    void hasFrameParser_isTrue_onlyForFrameParsedCodecs();

    void mpegFrameSize_returnsFrameLength_forValidHeader_data();
    void mpegFrameSize_returnsFrameLength_forValidHeader();
    void mpegFrameSize_returnsZero_whenHeaderIsInvalid_data();
    void mpegFrameSize_returnsZero_whenHeaderIsInvalid();
    void mpegFrameSize_returnsZero_whenOffsetIsOutOfBounds();

    void findMpegSync_returnsOffsetOfFirstValidHeader();
    void findMpegSync_skipsFalseSyncWords();
    void findMpegSync_returnsNullopt_whenNoSyncIsPresent();
    void findMpegSync_honorsStartOffset();

    void adtsFrameSize_returnsFrameLength_forValidHeader();
    void adtsFrameSize_returnsZero_whenHeaderIsInvalid_data();
    void adtsFrameSize_returnsZero_whenHeaderIsInvalid();
    void adtsFrameSize_returnsZero_whenOffsetIsOutOfBounds();

    void findAdtsSync_returnsOffsetOfFirstValidHeader();
    void findAdtsSync_returnsNullopt_whenNoSyncIsPresent();
};

void tst_QAudioParsingSupport::sniffCodec_detectsCodec_fromStreamMagic_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<AudioCodec>("expected");

    QTest::newRow("id3") << "ID3\x04"_ba << AudioCodec::Mp3;
    QTest::newRow("mpeg sync") << makeMpeg1Layer3Header() << AudioCodec::Mp3;
    QTest::newRow("flac") << "fLaC"_ba << AudioCodec::Flac;
    QTest::newRow("ogg") << "OggS"_ba << AudioCodec::Opus;
    QTest::newRow("adts mpeg4") << QByteArray::fromHex("fff15080") << AudioCodec::Aac;
    QTest::newRow("adts mpeg2") << QByteArray::fromHex("fff95080") << AudioCodec::Aac;
    QTest::newRow("adts mpeg4 with crc") << QByteArray::fromHex("fff05080") << AudioCodec::Aac;
    QTest::newRow("adts mpeg2 with crc") << QByteArray::fromHex("fff85080") << AudioCodec::Aac;
    QTest::newRow("riff") << "RIFF"_ba << AudioCodec::Wav;
    QTest::newRow("unrecognized") << "ftyp"_ba << AudioCodec::Unknown;
}

void tst_QAudioParsingSupport::sniffCodec_detectsCodec_fromStreamMagic()
{
    QFETCH(QByteArray, header);
    QFETCH(AudioCodec, expected);

    QCOMPARE(sniffCodec(asBytes(header)), expected);
}

void tst_QAudioParsingSupport::sniffCodec_returnsUnknown_whenHeaderIsTooShort()
{
    QCOMPARE(sniffCodec(asBytes(QByteArray())), AudioCodec::Unknown);
    QCOMPARE(sniffCodec(asBytes("fLa"_ba)), AudioCodec::Unknown);
}

void tst_QAudioParsingSupport::hasFrameParser_isTrue_onlyForFrameParsedCodecs()
{
    QVERIFY(hasFrameParser(AudioCodec::Mp3));
    QVERIFY(hasFrameParser(AudioCodec::Aac));
    QVERIFY(!hasFrameParser(AudioCodec::Flac));
    QVERIFY(!hasFrameParser(AudioCodec::Opus));
    QVERIFY(!hasFrameParser(AudioCodec::Wav));
    QVERIFY(!hasFrameParser(AudioCodec::Unknown));
}

void tst_QAudioParsingSupport::mpegFrameSize_returnsFrameLength_forValidHeader_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<qsizetype>("expected");

    // MPEG1 Layer III, 128 kbps, 44100 Hz: 144 * 128000 / 44100 = 417
    QTest::newRow("mpeg1 128kbps 44100") << makeMpeg1Layer3Header() << qsizetype(417);
    QTest::newRow("mpeg1 128kbps 44100 padded") << makeMpeg1Layer3Header(1) << qsizetype(418);
    // MPEG1 Layer III, 320 kbps, 48000 Hz: 144 * 320000 / 48000 = 960
    QTest::newRow("mpeg1 320kbps 48000") << makeMpegHeader(3, 1, 14, 1, 0) << qsizetype(960);
    // MPEG2 Layer III, 64 kbps, 22050 Hz: 72 * 64000 / 22050 = 208
    QTest::newRow("mpeg2 64kbps 22050") << makeMpegHeader(2, 1, 8, 0, 0) << qsizetype(208);
    // MPEG2.5 Layer III, 8 kbps, 11025 Hz: 72 * 8000 / 11025 = 52
    QTest::newRow("mpeg2.5 8kbps 11025") << makeMpegHeader(0, 1, 1, 0, 0) << qsizetype(52);
}

void tst_QAudioParsingSupport::mpegFrameSize_returnsFrameLength_forValidHeader()
{
    QFETCH(QByteArray, header);
    QFETCH(qsizetype, expected);

    QCOMPARE(mpegFrameSize(asBytes(header)), expected);
    // A header followed by payload parses identically.
    QCOMPARE(mpegFrameSize(asBytes(header + QByteArray(1024, '\0'))), expected);
}

void tst_QAudioParsingSupport::mpegFrameSize_returnsZero_whenHeaderIsInvalid_data()
{
    QTest::addColumn<QByteArray>("header");

    QTest::newRow("no sync word") << QByteArray::fromHex("00000000");
    QTest::newRow("partial sync word") << QByteArray::fromHex("ffc00000");
    QTest::newRow("layer I") << makeMpegHeader(3, 3, 9, 0, 0);
    QTest::newRow("layer II") << makeMpegHeader(3, 2, 9, 0, 0);
    QTest::newRow("reserved layer") << makeMpegHeader(3, 0, 9, 0, 0);
    QTest::newRow("reserved version") << makeMpegHeader(1, 1, 9, 0, 0);
    QTest::newRow("reserved sample rate") << makeMpegHeader(3, 1, 9, 3, 0);
    QTest::newRow("free-format bitrate") << makeMpegHeader(3, 1, 0, 0, 0);
    QTest::newRow("invalid bitrate index") << makeMpegHeader(3, 1, 15, 0, 0);
}

void tst_QAudioParsingSupport::mpegFrameSize_returnsZero_whenHeaderIsInvalid()
{
    QFETCH(QByteArray, header);

    QCOMPARE(mpegFrameSize(asBytes(header)), qsizetype(0));
}

void tst_QAudioParsingSupport::mpegFrameSize_returnsZero_whenOffsetIsOutOfBounds()
{
    const QByteArray header = makeMpeg1Layer3Header();

    QCOMPARE(mpegFrameSize(asBytes(QByteArray())), qsizetype(0));
    QCOMPARE(mpegFrameSize(asBytes(header.first(3))), qsizetype(0)); // truncated header
    QCOMPARE(mpegFrameSize(asBytes(header), 1), qsizetype(0)); // only 3 bytes left
    QCOMPARE(mpegFrameSize(asBytes(header), header.size()), qsizetype(0));
    QCOMPARE(mpegFrameSize(asBytes(header), header.size() + 100), qsizetype(0));
    QCOMPARE(mpegFrameSize(asBytes(header), -1), qsizetype(0));
}

void tst_QAudioParsingSupport::findMpegSync_returnsOffsetOfFirstValidHeader()
{
    const QByteArray stream =
            "ID3\x04"_ba + QByteArray(6, '\0') + makeMpeg1Layer3Header() + QByteArray(417, '\0');

    QCOMPARE(findMpegSync(asBytes(stream)), qsizetype(10));
}

void tst_QAudioParsingSupport::findMpegSync_skipsFalseSyncWords()
{
    // A sync word whose header is invalid (reserved sample rate) must be skipped
    // in favour of the valid header that follows.
    const QByteArray stream = makeMpegHeader(3, 1, 9, 3, 0) + makeMpeg1Layer3Header()
            + QByteArray(417, '\0');

    QCOMPARE(findMpegSync(asBytes(stream)), qsizetype(4));
}

void tst_QAudioParsingSupport::findMpegSync_returnsNullopt_whenNoSyncIsPresent()
{
    QVERIFY(!findMpegSync(asBytes(QByteArray())));
    QVERIFY(!findMpegSync(asBytes(QByteArray(64, '\0'))));
    QVERIFY(!findMpegSync(asBytes("ID3"_ba)));
}

void tst_QAudioParsingSupport::findMpegSync_honorsStartOffset()
{
    const QByteArray frame = makeMpeg1Layer3Header() + QByteArray(413, '\0');
    const QByteArray stream = frame + frame;

    QCOMPARE(findMpegSync(asBytes(stream)), qsizetype(0));
    QCOMPARE(findMpegSync(asBytes(stream), 1), qsizetype(417));
    QVERIFY(!findMpegSync(asBytes(stream), 418));
}

void tst_QAudioParsingSupport::adtsFrameSize_returnsFrameLength_forValidHeader()
{
    QCOMPARE(adtsFrameSize(asBytes(makeAdtsHeader(7))), qsizetype(7));
    QCOMPARE(adtsFrameSize(asBytes(makeAdtsHeader(100))), qsizetype(100));
    QCOMPARE(adtsFrameSize(asBytes(makeAdtsHeader(1024))), qsizetype(1024));
    QCOMPARE(adtsFrameSize(asBytes(makeAdtsHeader(8191))), qsizetype(8191)); // maximum 13-bit value
    QCOMPARE(adtsFrameSize(asBytes(makeAdtsHeader(384) + QByteArray(512, '\0'))), qsizetype(384));
}

void tst_QAudioParsingSupport::adtsFrameSize_returnsZero_whenHeaderIsInvalid_data()
{
    QTest::addColumn<QByteArray>("header");

    QTest::newRow("no sync word") << QByteArray(7, '\0');

    QByteArray partialSync = makeAdtsHeader(100);
    partialSync[1] = char(0xE1); // top 12 bits are not all set
    QTest::newRow("partial sync word") << partialSync;

    QTest::newRow("frame length below header size") << makeAdtsHeader(6);
    QTest::newRow("zero frame length") << makeAdtsHeader(0);
}

void tst_QAudioParsingSupport::adtsFrameSize_returnsZero_whenHeaderIsInvalid()
{
    QFETCH(QByteArray, header);

    QCOMPARE(adtsFrameSize(asBytes(header)), qsizetype(0));
}

void tst_QAudioParsingSupport::adtsFrameSize_returnsZero_whenOffsetIsOutOfBounds()
{
    const QByteArray header = makeAdtsHeader(100);

    QCOMPARE(adtsFrameSize(asBytes(QByteArray())), qsizetype(0));
    QCOMPARE(adtsFrameSize(asBytes(header.first(6))), qsizetype(0)); // truncated header
    QCOMPARE(adtsFrameSize(asBytes(header), 1), qsizetype(0)); // only 6 bytes left
    QCOMPARE(adtsFrameSize(asBytes(header), header.size()), qsizetype(0));
    QCOMPARE(adtsFrameSize(asBytes(header), header.size() + 100), qsizetype(0));
    QCOMPARE(adtsFrameSize(asBytes(header), -1), qsizetype(0));
}

void tst_QAudioParsingSupport::findAdtsSync_returnsOffsetOfFirstValidHeader()
{
    const QByteArray stream = QByteArray(5, '\0') + makeAdtsHeader(100) + QByteArray(93, '\0');

    QCOMPARE(findAdtsSync(asBytes(stream)), qsizetype(5));
    QVERIFY(!findAdtsSync(asBytes(stream), 6));
}

void tst_QAudioParsingSupport::findAdtsSync_returnsNullopt_whenNoSyncIsPresent()
{
    QVERIFY(!findAdtsSync(asBytes(QByteArray())));
    QVERIFY(!findAdtsSync(asBytes(QByteArray(64, '\0'))));
    QVERIFY(!findAdtsSync(asBytes(makeAdtsHeader(6)))); // sync word, invalid frame length
}

QTEST_GUILESS_MAIN(tst_QAudioParsingSupport)

#include "tst_qaudioparsingsupport.moc"
