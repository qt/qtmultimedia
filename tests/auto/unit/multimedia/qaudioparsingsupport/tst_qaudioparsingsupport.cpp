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

// Builds a FLAC metadata block: 4-byte header followed by dataLength payload bytes.
QByteArray makeFlacBlock(int blockType, bool isLast, int dataLength)
{
    QByteArray block(4, '\0');
    block[0] = char((isLast ? 0x80 : 0x00) | (blockType & 0x7F));
    block[1] = char((dataLength >> 16) & 0xFF);
    block[2] = char((dataLength >> 8) & 0xFF);
    block[3] = char(dataLength & 0xFF);
    block.append(QByteArray(dataLength, '\0'));
    return block;
}

// Builds a complete FLAC STREAMINFO block carrying the given format.
QByteArray makeFlacStreamInfoBlock(int sampleRate, int numChannels, bool isLast = true,
                                   qint64 totalSamples = 0)
{
    QByteArray block = makeFlacBlock(0, isLast, 34);
    // STREAMINFO data starts at block[4]. The sample rate occupies the top 20
    // bits of the three bytes at data offset 10..12, followed by 3 channel-count
    // bits.
    block[4 + 10] = char((sampleRate >> 12) & 0xFF);
    block[4 + 11] = char((sampleRate >> 4) & 0xFF);
    block[4 + 12] = char(((sampleRate & 0x0F) << 4) | (((numChannels - 1) & 0x07) << 1));
    // The 36-bit total sample count follows, starting in the low 4 bits of
    // data offset 13.
    block[4 + 13] = char((totalSamples >> 32) & 0x0F);
    block[4 + 14] = char((totalSamples >> 24) & 0xFF);
    block[4 + 15] = char((totalSamples >> 16) & 0xFF);
    block[4 + 16] = char((totalSamples >> 8) & 0xFF);
    block[4 + 17] = char(totalSamples & 0xFF);
    return block;
}

// CRC-8 with polynomial 0x07 and initial value 0, as used by the FLAC frame
// header. Duplicated here so the test computes the expected value itself
// rather than trusting the implementation under test.
quint8 flacHeaderCrc(QByteArrayView data)
{
    quint8 crc = 0;
    for (char character : data) {
        crc ^= quint8(character);
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80) ? quint8((crc << 1) ^ 0x07) : quint8(crc << 1);
    }
    return crc;
}

// Builds a FLAC frame header with a one-byte coded frame number, block size
// code 5 (4608 samples), sample rate code 9 (44100 Hz), stereo, 16 bit, and a
// correct trailing CRC-8. That is the shape the reference encoder emits.
QByteArray makeFlacFrameHeader(int frameNumber = 0)
{
    QByteArray header(5, '\0');
    header[0] = char(0xFF);
    header[1] = char(0xF8); // sync, reserved 0, fixed block size
    header[2] = char(0x59); // block size code 5, sample rate code 9
    header[3] = char(0x18); // channel assignment 1 (left/side), sample size 100
    header[4] = char(frameNumber & 0x7F);
    header.append(char(flacHeaderCrc(header)));
    return header;
}

QByteArray makeFlacStream(int sampleRate = 44100, int numChannels = 2, qint64 totalSamples = 0)
{
    return "fLaC"_ba + makeFlacStreamInfoBlock(sampleRate, numChannels, true, totalSamples);
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

    void flacStreamInfo_returnsFormat_fromStreamInfoBlock_data();
    void flacStreamInfo_returnsFormat_fromStreamInfoBlock();
    void flacStreamInfo_returnsNullopt_whenDataIsInvalid_data();
    void flacStreamInfo_returnsNullopt_whenDataIsInvalid();

    void flacAudioOffset_returnsOffsetPastMetadata_whenSingleBlock();
    void flacAudioOffset_returnsOffsetPastMetadata_whenMultipleBlocks();
    void flacAudioOffset_returnsNullopt_whenDataIsInvalid_data();
    void flacAudioOffset_returnsNullopt_whenDataIsInvalid();

    void findFlacSync_returnsOffsetOfFirstValidHeader();
    void findFlacSync_skipsHeadersWithBadCrc();
    void findFlacSync_rejectsReservedHeaderFields_data();
    void findFlacSync_rejectsReservedHeaderFields();
    void findFlacSync_returnsNullopt_whenNoSyncIsPresent();
    void findFlacSync_honorsStartOffset();

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
    QVERIFY(hasFrameParser(AudioCodec::Flac));
    QVERIFY(!hasFrameParser(AudioCodec::Opus));
    QVERIFY(!hasFrameParser(AudioCodec::Wav));
    QVERIFY(!hasFrameParser(AudioCodec::Unknown));
}

void tst_QAudioParsingSupport::flacStreamInfo_returnsFormat_fromStreamInfoBlock_data()
{
    QTest::addColumn<int>("sampleRate");
    QTest::addColumn<int>("numChannels");
    QTest::addColumn<qint64>("totalSamples");

    QTest::newRow("44100 stereo") << 44100 << 2 << qint64(698194);
    QTest::newRow("48000 stereo, unknown length") << 48000 << 2 << qint64(0);
    QTest::newRow("8000 mono") << 8000 << 1 << qint64(8250520);
    QTest::newRow("192000 8 channels, full 36 bits") << 192000 << 8 << qint64(0xFFFFFFFFF);
}

void tst_QAudioParsingSupport::flacStreamInfo_returnsFormat_fromStreamInfoBlock()
{
    QFETCH(int, sampleRate);
    QFETCH(int, numChannels);
    QFETCH(qint64, totalSamples);

    const std::optional<FlacStreamInfo> info =
            flacStreamInfo(asBytes(makeFlacStream(sampleRate, numChannels, totalSamples)));
    QVERIFY(info);
    QCOMPARE(info->sampleRate, sampleRate);
    QCOMPARE(info->numChannels, numChannels);
    QCOMPARE(info->totalSamples, totalSamples);
}

void tst_QAudioParsingSupport::flacStreamInfo_returnsNullopt_whenDataIsInvalid_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("shorter than minimum") << makeFlacStream().first(41);
    QTest::newRow("wrong magic") << ("fLaX"_ba + makeFlacStreamInfoBlock(44100, 2));

    QByteArray wrongBlockType = "fLaC"_ba + makeFlacBlock(4, true, 34);
    QTest::newRow("first block is not STREAMINFO") << wrongBlockType;

    QByteArray shortBlock = "fLaC"_ba + makeFlacBlock(0, true, 33) + QByteArray(8, '\0');
    QTest::newRow("STREAMINFO block too short") << shortBlock;

    QByteArray truncatedBlock = "fLaC"_ba + makeFlacBlock(0, true, 100);
    QTest::newRow("block length exceeds data") << truncatedBlock.first(50);
}

void tst_QAudioParsingSupport::flacStreamInfo_returnsNullopt_whenDataIsInvalid()
{
    QFETCH(QByteArray, data);

    QVERIFY(!flacStreamInfo(asBytes(data)));
}

void tst_QAudioParsingSupport::flacAudioOffset_returnsOffsetPastMetadata_whenSingleBlock()
{
    // "fLaC" + 4-byte block header + 34-byte STREAMINFO
    QCOMPARE(flacAudioOffset(asBytes(makeFlacStream())), qsizetype(42));
}

void tst_QAudioParsingSupport::flacAudioOffset_returnsOffsetPastMetadata_whenMultipleBlocks()
{
    QByteArray stream = "fLaC"_ba;
    stream += makeFlacStreamInfoBlock(44100, 2, /*isLast=*/false);
    stream += makeFlacBlock(3, /*isLast=*/false, 18); // SEEKTABLE
    stream += makeFlacBlock(4, /*isLast=*/true, 64); // VORBIS_COMMENT
    stream += QByteArray(128, '\xAA'); // audio frames

    // 4 + (4 + 34) + (4 + 18) + (4 + 64)
    QCOMPARE(flacAudioOffset(asBytes(stream)), qsizetype(132));
}

void tst_QAudioParsingSupport::flacAudioOffset_returnsNullopt_whenDataIsInvalid_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("empty") << QByteArray();
    QTest::newRow("shorter than block header") << ("fLaC"_ba + QByteArray(2, '\0'));
    QTest::newRow("wrong magic") << ("RIFF"_ba + makeFlacStreamInfoBlock(44100, 2));

    QByteArray noLastBlock = "fLaC"_ba + makeFlacStreamInfoBlock(44100, 2, false);
    QTest::newRow("metadata chain not terminated") << noLastBlock;

    // A last block declaring a length that runs past the end of the data must not
    // produce an offset outside the buffer, which callers slice with directly.
    QByteArray overrunningLastBlock = "fLaC"_ba + makeFlacBlock(0, true, 0xFFFFFF);
    QTest::newRow("last block overruns data") << overrunningLastBlock.first(8);

    QByteArray overrunningMiddleBlock = "fLaC"_ba + makeFlacBlock(0, false, 0xFFFFFF);
    QTest::newRow("middle block overruns data") << overrunningMiddleBlock.first(8);
}

void tst_QAudioParsingSupport::flacAudioOffset_returnsNullopt_whenDataIsInvalid()
{
    QFETCH(QByteArray, data);

    QVERIFY(!flacAudioOffset(asBytes(data)));
}

void tst_QAudioParsingSupport::findFlacSync_returnsOffsetOfFirstValidHeader()
{
    const QByteArray stream =
            QByteArray(7, '\0') + makeFlacFrameHeader() + QByteArray(64, '\0');

    QCOMPARE(findFlacSync(asBytes(stream)), qsizetype(7));
}

void tst_QAudioParsingSupport::findFlacSync_skipsHeadersWithBadCrc()
{
    QByteArray corrupt = makeFlacFrameHeader();
    corrupt[5] = char(quint8(corrupt[5]) ^ 0xFF); // break the CRC-8 only

    const QByteArray stream = corrupt + makeFlacFrameHeader(1) + QByteArray(64, '\0');

    // The first header looks like a sync word but fails the CRC, so the scan
    // must run on to the second one.
    QCOMPARE(findFlacSync(asBytes(stream)), qsizetype(corrupt.size()));
}

void tst_QAudioParsingSupport::findFlacSync_rejectsReservedHeaderFields_data()
{
    QTest::addColumn<int>("byteIndex");
    QTest::addColumn<int>("byteValue");

    QTest::newRow("reserved bit in sync byte") << 1 << 0xFA;
    QTest::newRow("block size code 0") << 2 << 0x09;
    QTest::newRow("sample rate code 15") << 2 << 0x5F;
    QTest::newRow("channel assignment 11") << 3 << 0xB8;
    QTest::newRow("sample size code 3") << 3 << 0x16;
    QTest::newRow("reserved bit in byte 3") << 3 << 0x19;
}

void tst_QAudioParsingSupport::findFlacSync_rejectsReservedHeaderFields()
{
    QFETCH(const int, byteIndex);
    QFETCH(const int, byteValue);

    QByteArray header = makeFlacFrameHeader();
    header[byteIndex] = char(byteValue);
    // Recompute the CRC so the header is rejected for the reserved field
    // itself, not for a CRC that no longer matches.
    header[5] = char(flacHeaderCrc(QByteArrayView(header).first(5)));

    QVERIFY(!findFlacSync(asBytes(header + QByteArray(64, '\0'))));
}

void tst_QAudioParsingSupport::findFlacSync_returnsNullopt_whenNoSyncIsPresent()
{
    QVERIFY(!findFlacSync(asBytes(QByteArray())));
    QVERIFY(!findFlacSync(asBytes(QByteArray(64, '\0'))));
    // A header truncated before its CRC-8 byte cannot be validated.
    QVERIFY(!findFlacSync(asBytes(makeFlacFrameHeader().first(5))));
}

void tst_QAudioParsingSupport::findFlacSync_honorsStartOffset()
{
    const QByteArray header = makeFlacFrameHeader();
    const QByteArray stream = header + makeFlacFrameHeader(1) + QByteArray(64, '\0');

    QCOMPARE(findFlacSync(asBytes(stream)), qsizetype(0));
    QCOMPARE(findFlacSync(asBytes(stream), 1), qsizetype(header.size()));
    QVERIFY(!findFlacSync(asBytes(stream), header.size() + 1));
    QCOMPARE(findFlacSync(asBytes(stream), -5), qsizetype(0)); // negative start clamps to 0
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
