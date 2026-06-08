// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qaudio_parsing_support_p.h"

#include "qaudio_qspan_support_p.h"
#include "qmultimedia_ranges_p.h"

#include <QtCore/qbytearrayview.h>

#include <array>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate::ParsingSupport {

namespace {

constexpr std::byte operator""_byte(unsigned long long value)
{
    return std::byte(value);
}

// QSpan::operator[] asserts on out-of-bounds access, so every read below is
// bounds-checked in debug builds.
constexpr quint8 byteAt(QSpan<const std::byte> data, qsizetype index)
{
    return std::to_integer<quint8>(data[index]);
}

// Returns true if data begins with marker.
bool hasMarker(QSpan<const std::byte> data, QByteArrayView marker)
{
    return QByteArrayView(data).startsWith(marker);
}

// Scans data[from..] for a frame sync word: a 0xFF byte whose successor matches
// secondByteValue under secondByteMask, and whose header frameSize() accepts.
// headerSize is the number of bytes frameSize() needs in order to decide.
std::optional<qsizetype> findSyncWord(QSpan<const std::byte> data, qsizetype from,
                                      std::byte secondByteMask, std::byte secondByteValue,
                                      qsizetype headerSize,
                                      qsizetype (*frameSize)(QSpan<const std::byte>, qsizetype))
{
    const QByteArrayView view(data);
    const qsizetype last = view.size() - headerSize;

    for (qsizetype i = qMax<qsizetype>(from, 0); i <= last;) {
        const qsizetype sync = view.indexOf(char(0xFF), i);
        if (sync < 0 || sync > last)
            return std::nullopt;
        if ((data[sync + 1] & secondByteMask) == secondByteValue && frameSize(data, sync) > 0) {
            return sync;
        }
        i = sync + 1;
    }
    return std::nullopt;
}

// Reads 1-4 bytes as an unsigned big-endian integer.
quint32 fromBigEndian(QSpan<const std::byte> data)
{
    Q_ASSERT(data.size() <= 4);
    quint32 value = 0;
    for (std::byte b : data)
        value = (value << 8) | std::to_integer<quint32>(b);
    return value;
}

// CRC-8 with polynomial x^8 + x^2 + x + 1 (0x07) and initial value 0, as used
// by the FLAC frame header.
quint8 crc8(QSpan<const std::byte> data)
{
    quint8 crc = 0;
    for (std::byte b : data) {
        crc ^= std::to_integer<quint8>(b);
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80) ? quint8((crc << 1) ^ 0x07) : quint8(crc << 1);
    }
    return crc;
}

// Returns the length in bytes of the FLAC frame header starting at
// data[offset], including its trailing CRC-8 byte, or 0 if the header is
// invalid or runs past the end of data. A FLAC frame header is between 6 and
// 16 bytes long, so a truncated tail is reported as invalid rather than
// guessed at.
qsizetype flacFrameHeaderSize(QSpan<const std::byte> data, qsizetype offset)
{
    if (offset < 0)
        return 0;

    const QSpan<const std::byte> frame = drop(data, offset);
    if (frame.size() < 6) // 4 header bytes + 1 coded-number byte + CRC-8
        return 0;

    // 14-bit sync code 0b11111111111110, then a reserved bit that must be 0,
    // then the blocking strategy bit.
    if (frame[0] != 0xFF_byte || (frame[1] & 0xFE_byte) != 0xF8_byte)
        return 0;

    const quint8 blockSizeCode = byteAt(frame, 2) >> 4;
    const quint8 sampleRateCode = byteAt(frame, 2) & 0x0F;
    const quint8 channelCode = byteAt(frame, 3) >> 4;
    const quint8 sampleSizeCode = (byteAt(frame, 3) >> 1) & 0x07;

    // Block size code 0, sample rate code 15, channel assignments above 10 and
    // sample size code 3 are all reserved, as is the last bit of byte 3.
    if (blockSizeCode == 0 || sampleRateCode == 0x0F || channelCode > 10
        || sampleSizeCode == 3 || (byteAt(frame, 3) & 0x01) != 0) {
        return 0;
    }

    // The coded frame or sample number uses a UTF-8 style variable length
    // encoding of one to seven bytes.
    const std::byte codedNumberLead = frame[4];
    qsizetype codedNumberSize = 0;
    if ((codedNumberLead & 0x80_byte) == 0x00_byte)
        codedNumberSize = 1;
    else if ((codedNumberLead & 0xE0_byte) == 0xC0_byte)
        codedNumberSize = 2;
    else if ((codedNumberLead & 0xF0_byte) == 0xE0_byte)
        codedNumberSize = 3;
    else if ((codedNumberLead & 0xF8_byte) == 0xF0_byte)
        codedNumberSize = 4;
    else if ((codedNumberLead & 0xFC_byte) == 0xF8_byte)
        codedNumberSize = 5;
    else if ((codedNumberLead & 0xFE_byte) == 0xFC_byte)
        codedNumberSize = 6;
    else if (codedNumberLead == 0xFE_byte)
        codedNumberSize = 7;
    else
        return 0;

    if (frame.size() < 4 + codedNumberSize)
        return 0;
    const bool continuationBytesValid =
            ranges::all_of(frame.subspan(5, codedNumberSize - 1), [](std::byte b) {
        return (b & std::byte{ 0xC0 }) == std::byte{ 0x80 };
    });
    if (!continuationBytesValid)
        return 0;

    qsizetype headerSize = 4 + codedNumberSize;

    // Block size codes 6 and 7, and sample rate codes 12 to 14, carry their
    // value in extra bytes placed after the coded number.
    if (blockSizeCode == 6)
        headerSize += 1;
    else if (blockSizeCode == 7)
        headerSize += 2;

    if (sampleRateCode == 12)
        headerSize += 1;
    else if (sampleRateCode == 13 || sampleRateCode == 14)
        headerSize += 2;

    // The CRC-8 covers every header byte before it. Checking it is what makes
    // a sync scan reliable, since the sync code alone occurs often in the
    // compressed subframe data that follows.
    if (frame.size() < headerSize + 1)
        return 0;
    if (crc8(take(frame, headerSize)) != byteAt(frame, headerSize))
        return 0;

    return headerSize + 1;
}

} // namespace

AudioCodec sniffCodec(QSpan<const std::byte> header)
{
    if (header.size() < 4)
        return AudioCodec::Unknown;

    // ID3 tag -> MP3
    if (hasMarker(header, "ID3"))
        return AudioCodec::Mp3;

    // AAC ADTS sync word. Checked before the MPEG audio sync word below, whose
    // 0xFFE0 mask also matches an ADTS header. The two are told apart by the
    // layer bits: MPEG Layer III uses 01, ADTS always leaves them 00.
    if (header[0] == 0xFF_byte && (header[1] & 0xF6_byte) == 0xF0_byte)
        return AudioCodec::Aac;

    // MPEG audio sync word (0xFFE0 mask) -> MP3
    if (header[0] == 0xFF_byte && (header[1] & 0xE0_byte) == 0xE0_byte)
        return AudioCodec::Mp3;

    // FLAC stream marker
    if (hasMarker(header, "fLaC"))
        return AudioCodec::Flac;

    // Ogg container. Assume Opus, which is the more common of the two on the
    // web, though the stream could also be Vorbis.
    if (hasMarker(header, "OggS"))
        return AudioCodec::Opus;

    // WAV / RIFF - uncompressed
    if (hasMarker(header, "RIFF"))
        return AudioCodec::Wav;

    return AudioCodec::Unknown;
}

bool hasFrameParser(AudioCodec codec)
{
    return codec == AudioCodec::Mp3 || codec == AudioCodec::Aac || codec == AudioCodec::Flac;
}

std::optional<FlacStreamInfo> flacStreamInfo(QSpan<const std::byte> data)
{
    // "fLaC" marker + 4-byte block header + 34-byte STREAMINFO = 42 bytes minimum
    if (data.size() < 42 || !hasMarker(data, "fLaC"))
        return std::nullopt;

    // Block header byte: bit 7 = last-metadata-block flag, bits [6:0] = block type.
    // Block type 0 is STREAMINFO, which must be the first metadata block.
    if ((data[4] & 0x7F_byte) != 0x00_byte)
        return std::nullopt;

    const qsizetype blockLength = fromBigEndian(data.subspan(5, 3));
    if (blockLength < 34 || data.size() < 8 + blockLength)
        return std::nullopt;

    FlacStreamInfo info;
    // STREAMINFO data starts at byte 8.
    // Sample rate: top 20 bits spanning bytes [18..20]
    info.sampleRate = fromBigEndian(data.subspan(18, 3)) >> 4;
    // Number of channels: next 3 bits (bits [3:1] of byte 20)
    info.numChannels = ((byteAt(data, 20) >> 1) & 0x7) + 1;
    // Total samples: the 36 bits after the 5-bit sample size, which is the low
    // 4 bits of byte 21 followed by bytes [22..25].
    info.totalSamples = (qint64(byteAt(data, 21) & 0x0F) << 32)
            | (qint64(byteAt(data, 22)) << 24) | (byteAt(data, 23) << 16)
            | (byteAt(data, 24) << 8) | byteAt(data, 25);
    return info;
}

std::optional<qsizetype> flacAudioOffset(QSpan<const std::byte> data)
{
    if (data.size() < 8 || !hasMarker(data, "fLaC"))
        return std::nullopt;

    qsizetype offset = 4; // skip "fLaC"
    while (offset + 4 <= data.size()) {
        const bool isLast = (data[offset] & 0x80_byte) != 0x00_byte;
        const qsizetype blockLength = fromBigEndian(data.subspan(offset + 1, 3));
        offset += 4 + blockLength;
        if (isLast)
            return offset <= data.size() ? std::optional(offset) : std::nullopt;
    }
    return std::nullopt; // metadata blocks not fully buffered yet
}

std::optional<qsizetype> findFlacSync(QSpan<const std::byte> data, qsizetype from)
{
    const QByteArrayView view(data);
    for (qsizetype i = qMax<qsizetype>(from, 0); i < view.size();) {
        const qsizetype sync = view.indexOf(char(0xFF), i);
        if (sync < 0)
            return std::nullopt;
        if (flacFrameHeaderSize(data, sync) > 0)
            return sync;
        i = sync + 1;
    }
    return std::nullopt;
}

qsizetype mpegFrameSize(QSpan<const std::byte> data, qsizetype offset)
{
    if (offset < 0)
        return 0;

    const QSpan<const std::byte> header = take(drop(data, offset), 4);
    if (header.size() < 4)
        return 0;

    if (header[0] != 0xFF_byte || (header[1] & 0xE0_byte) != 0xE0_byte)
        return 0;

    const int versionBits = (byteAt(header, 1) >> 3) & 0x3; // 3=MPEG1, 2=MPEG2, 0=MPEG2.5
    const int layerBits = (byteAt(header, 1) >> 1) & 0x3; // 1=LayerIII
    const int bitrateIndex = (byteAt(header, 2) >> 4) & 0xF;
    const int sampleRateIndex = (byteAt(header, 2) >> 2) & 0x3;
    const int padding = (byteAt(header, 2) >> 1) & 0x1;

    if (layerBits != 1)
        return 0; // only Layer III
    if (sampleRateIndex == 3)
        return 0; // reserved sample rate
    if (bitrateIndex == 0 || bitrateIndex == 15)
        return 0; // free-format / bad index

    static constexpr std::array<int, 16> bitratesV1 = // MPEG1 Layer III (kbps)
        { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 };
    static constexpr std::array<int, 16> bitratesV2 = // MPEG2/2.5 Layer III (kbps)
        { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 };

    static constexpr std::array<std::array<int, 3>, 4> sampleRates = {{
        { 11025, 12000, 8000 }, // MPEG 2.5 (versionBits == 0)
        { 0, 0, 0 }, // reserved (versionBits == 1)
        { 22050, 24000, 16000 }, // MPEG 2 (versionBits == 2)
        { 44100, 48000, 32000 }, // MPEG 1 (versionBits == 3)
    }};

    const int sampleRate = sampleRates[versionBits][sampleRateIndex];
    if (sampleRate == 0)
        return 0;

    const bool isMpeg1 = (versionBits == 3);
    const int bitrateBps = (isMpeg1 ? bitratesV1 : bitratesV2)[bitrateIndex] * 1000;
    if (bitrateBps == 0)
        return 0;

    // MPEG1 Layer III:     frameSize = floor(144 * bitrate / sampleRate) + padding
    // MPEG2/2.5 Layer III: frameSize = floor( 72 * bitrate / sampleRate) + padding
    const int coefficient = isMpeg1 ? 144 : 72;
    const qsizetype frameSize = (coefficient * bitrateBps / sampleRate) + padding;
    return frameSize > 4 ? frameSize : 0; // must be larger than the header itself
}

std::optional<qsizetype> findMpegSync(QSpan<const std::byte> data, qsizetype from)
{
    return findSyncWord(data, from, 0xE0_byte, 0xE0_byte, 4, mpegFrameSize);
}

qsizetype adtsFrameSize(QSpan<const std::byte> data, qsizetype offset)
{
    if (offset < 0)
        return 0;

    const QSpan<const std::byte> header = take(drop(data, offset), 7);
    if (header.size() < 7)
        return 0;

    // 12-bit sync word: 0xFFF
    if (header[0] != 0xFF_byte || (header[1] & 0xF0_byte) != 0xF0_byte)
        return 0;

    // aac_frame_length is a 13-bit field at bits [30:18] of the ADTS header
    // that gives the total byte length including the header itself.
    const qsizetype frameLength = ((byteAt(header, 3) & 0x03) << 11) | (byteAt(header, 4) << 3)
            | (byteAt(header, 5) >> 5);
    return frameLength >= 7 ? frameLength : 0;
}

std::optional<qsizetype> findAdtsSync(QSpan<const std::byte> data, qsizetype from)
{
    return findSyncWord(data, from, 0xF0_byte, 0xF0_byte, 7, adtsFrameSize);
}

} // namespace QtMultimediaPrivate::ParsingSupport

QT_END_NAMESPACE
