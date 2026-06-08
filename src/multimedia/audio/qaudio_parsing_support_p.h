// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QAUDIO_PARSING_SUPPORT_P_H
#define QAUDIO_PARSING_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/qtmultimediaglobal.h>
#include <QtCore/qspan.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate::ParsingSupport {

// Audio codecs that can be identified from the first bytes of a stream.
enum class AudioCodec {
    Unknown,
    Mp3,
    Aac,
    Flac,
    Opus,
    Vorbis,
    Wav,
};

// The sample rate and channel count from the identification packet of the
// first page of an Ogg stream.
struct OggStreamInfo
{
    AudioCodec codec = AudioCodec::Unknown;
    int sampleRate = 0;
    int numChannels = 0;
};

// The parts of a FLAC STREAMINFO metadata block that describe the stream format.
struct FlacStreamInfo
{
    int sampleRate = 0;
    int numChannels = 0;
    // Total samples per channel. 0 when the encoder did not record it.
    qint64 totalSamples = 0;
};

// Identifies the codec from the leading bytes of a stream. At least 4 bytes are
// needed. Anything shorter or unrecognized returns AudioCodec::Unknown.
Q_MULTIMEDIA_EXPORT AudioCodec sniffCodec(QSpan<const std::byte> header);

// Returns true for codecs whose individual frame sizes can be computed from the
// frame header, i.e. the codecs mpegFrameSize()/adtsFrameSize() understand.
Q_MULTIMEDIA_EXPORT bool hasFrameParser(AudioCodec codec);

// Parses the FLAC STREAMINFO metadata block at the beginning of a FLAC stream.
Q_MULTIMEDIA_EXPORT std::optional<FlacStreamInfo> flacStreamInfo(QSpan<const std::byte> data);

// Parses the identification packet on the first page of an Ogg stream, which
// says whether the stream is Vorbis or Opus and carries its format. Returns
// std::nullopt if the data is too short or is not a recognized Ogg audio
// stream. For Opus the rate is the encoder's input rate. Opus itself always
// decodes at 48000 Hz, so callers must not treat it as the decoded rate.
Q_MULTIMEDIA_EXPORT std::optional<OggStreamInfo> oggStreamInfo(QSpan<const std::byte> data);

// Returns the byte offset of the first FLAC audio frame in data, i.e. the
// position just past the "fLaC" marker and all metadata blocks. The result is
// always within data, so callers can use it as a slice position directly.
// std::nullopt covers a metadata block whose declared length runs past the
// end of the data.
Q_MULTIMEDIA_EXPORT std::optional<qsizetype> flacAudioOffset(QSpan<const std::byte> data);

// Scans data[from..] for the first valid FLAC frame header: the 14-bit sync
// code, header fields that are not reserved values, and a matching header
// CRC-8. FLAC frames carry no length field, so a frame spans from one frame
// header to the next, or to the end of the stream for the final frame.
Q_MULTIMEDIA_EXPORT std::optional<qsizetype> findFlacSync(QSpan<const std::byte> data,
                                                          qsizetype from = 0);

// Returns the byte length of the MPEG Layer III frame whose 4-byte header starts
// at data[offset], or 0 if the header is invalid or out of bounds.
Q_MULTIMEDIA_EXPORT qsizetype mpegFrameSize(QSpan<const std::byte> data, qsizetype offset = 0);

// Scans data[from..] for the first valid MPEG Layer III sync word + header.
Q_MULTIMEDIA_EXPORT std::optional<qsizetype> findMpegSync(QSpan<const std::byte> data,
                                                          qsizetype from = 0);

// Returns the byte length of the ADTS AAC frame (including header) starting at
// data[offset], or 0 if the header is invalid or out of bounds.
Q_MULTIMEDIA_EXPORT qsizetype adtsFrameSize(QSpan<const std::byte> data, qsizetype offset = 0);

// Scans data[from..] for the first valid ADTS AAC sync word + header.
Q_MULTIMEDIA_EXPORT std::optional<qsizetype> findAdtsSync(QSpan<const std::byte> data,
                                                          qsizetype from = 0);

} // namespace QtMultimediaPrivate::ParsingSupport

QT_END_NAMESPACE

#endif // QAUDIO_PARSING_SUPPORT_P_H
