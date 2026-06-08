// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qaudio_parsing_support_p.h"

#include "qaudio_qspan_support_p.h"

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
    return codec == AudioCodec::Mp3 || codec == AudioCodec::Aac;
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
