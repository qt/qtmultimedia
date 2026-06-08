// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwasmaudiodecoder_p.h"

#include <emscripten.h>
#include <emscripten/val.h>

#include <QtCore/qfile.h>
#include <QtCore/qscopedvaluerollback.h>
#include <QtCore/qspan.h>

#include <private/qstdweb_p.h>
#include <QtMultimedia/private/qaudio_parsing_support_p.h>

#include <chrono>
#include <optional>

QT_BEGIN_NAMESPACE

namespace AudioParsing = QtMultimediaPrivate::ParsingSupport;

// ---------------------------------------------------------------------------
// EM_JS. Only the decoder constructor stays here because it creates JS
// closures that are impractical to express with emscripten::val.
// Everything else is implemented directly in C++ using emscripten::val.
// ---------------------------------------------------------------------------

// Creates an AudioDecoder.  The output callback stores each decoded AudioData
// object directly in decoder._qtOutputs so C++ can allocate the receive buffer
// and call copyTo() itself, with no Module._malloc dependency. Error outputs
// are stored as plain {_error} objects.
//
// Caller must close all AudioData entries and call decoder.close() when done.
EM_JS(emscripten::EM_VAL, qt_wc_create_decoder, (), {
    var outputs = [];
    var decoder = new AudioDecoder({
        output: function(audioData) {
            outputs.push(audioData);
        },
        error: function(err) {
            outputs.push({ _error: err.message || String(err) });
        }
    });
    decoder._qtOutputs = outputs;
    return Emval.toHandle(decoder);
});

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

namespace {

// How often decoded outputs are drained while decoding. Short enough that
// buffers arrive steadily, long enough that each drain handles a batch.
constexpr std::chrono::milliseconds outputDrainInterval{ 10 };

QSpan<const std::byte> asBytes(const QByteArray &data)
{
    return as_bytes(QSpan{ data.constData(), data.size() });
}

// Returns the WebCodecs codec identifier string for a sniffed codec.
// Only meaningful for codecs that can actually be configured on an
// AudioDecoder (i.e. not Unknown or Wav).
const char *codecString(AudioParsing::AudioCodec codec)
{
    switch (codec) {
    case AudioParsing::AudioCodec::Mp3:  return "mp3";
    case AudioParsing::AudioCodec::Aac:  return "mp4a.40.2";
    case AudioParsing::AudioCodec::Flac: return "flac";
    case AudioParsing::AudioCodec::Opus: return "opus";
    case AudioParsing::AudioCodec::Unknown:
    case AudioParsing::AudioCodec::Vorbis: // no WebCodecs decoder; decodeAudioData handles it
    case AudioParsing::AudioCodec::Wav:
        break;
    }
    return "";
}

// Builds the WebCodecs AudioDecoderConfig.description field for a FLAC stream,
// which is mandatory for FLAC - isConfigSupported rejects without it. Chrome
// requires all metadata blocks in the description, not just STREAMINFO, so the
// blob spans everything between the "fLaC" marker and the first audio frame.
// Also fills sampleRate and numChannels from the STREAMINFO block.
// Returns an empty array if the data is too short or malformed.
QByteArray flacDescription(const QByteArray &data, int &sampleRate, int &numChannels)
{
    const std::optional<AudioParsing::FlacStreamInfo> info =
            AudioParsing::flacStreamInfo(asBytes(data));
    if (!info)
        return {};

    sampleRate = info->sampleRate;
    numChannels = info->numChannels;

    const std::optional<qsizetype> audioOffset = AudioParsing::flacAudioOffset(asBytes(data));
    if (!audioOffset)
        return {}; // metadata not fully buffered yet
    return data.mid(4, *audioOffset - 4);
}

// Returns the stream duration recorded in the FLAC STREAMINFO block, or
// std::nullopt when the block is missing or the encoder did not record the
// total sample count. Like the other backends, the decoder reports duration
// from container metadata before decoding starts.
std::optional<std::chrono::milliseconds> flacDuration(const QByteArray &data)
{
    const std::optional<AudioParsing::FlacStreamInfo> info =
            AudioParsing::flacStreamInfo(asBytes(data));
    if (!info || info->sampleRate <= 0 || info->totalSamples <= 0)
        return std::nullopt;
    return std::chrono::milliseconds{ info->totalSamples * 1000 / info->sampleRate };
}

// Returns the sample rate the browser will decode this stream at, or 0 when it
// is unknown. Only Ogg Vorbis is reported: decodeAudioData resamples to the
// AudioContext rate, so pinning the context to the Vorbis rate keeps the
// source's own rate. Opus is deliberately excluded because it always decodes
// at 48000 Hz regardless of the encoder's input rate.
int nativeSampleRate(AudioParsing::AudioCodec codec, const QByteArray &data)
{
    if (codec != AudioParsing::AudioCodec::Vorbis)
        return 0;

    const std::optional<AudioParsing::OggStreamInfo> info =
            AudioParsing::oggStreamInfo(asBytes(data));
    if (!info || info->codec != AudioParsing::AudioCodec::Vorbis)
        return 0;

    // AudioContext rejects rates outside the range it supports, so anything
    // unusual is left to the default context rather than risking a throw.
    constexpr int minimumRate = 8000;
    constexpr int maximumRate = 96000;
    return info->sampleRate >= minimumRate && info->sampleRate <= maximumRate ? info->sampleRate
                                                                             : 0;
}

// Returns a short, human-readable description of the first bytes of a stream.
// Included in fallback error messages so that a source which is not audio at
// all, such as a truncated download or an HTML error page saved with an
// audio file name, is obvious from the message alone.
QString dataPrefixHint(const QByteArray &data)
{
    constexpr qsizetype maxHintBytes = 16;
    const QByteArray head = data.left(maxHintBytes);
    if (head.isEmpty())
        return QStringLiteral("no data");

    bool textLike = true;
    for (char character : head) {
        const auto value = static_cast<unsigned char>(character);
        const bool isWhitespace = value == '\t' || value == '\n' || value == '\r';
        if (!isWhitespace && (value < 0x20 || value > 0x7e)) {
            textLike = false;
            break;
        }
    }

    if (textLike) {
        return QStringLiteral("data starts with \"%1\"")
                .arg(QString::fromLatin1(head).simplified());
    }
    return QStringLiteral("data starts with %1")
            .arg(QString::fromLatin1(head.toHex(' ')));
}

} // namespace

// ---------------------------------------------------------------------------
// QWasmAudioDecoder
// ---------------------------------------------------------------------------

QWasmAudioDecoder::QWasmAudioDecoder(QAudioDecoder *parent)
    : QPlatformAudioDecoder(parent)
{
    m_outputDrainTimer.setInterval(outputDrainInterval);
    connect(&m_outputDrainTimer, &QTimer::timeout, this, &QWasmAudioDecoder::deliverOutputs);
}

QWasmAudioDecoder::~QWasmAudioDecoder()
{
    ++(*m_generationPtr);
    if (m_sourceDevice)
        disconnect(m_sourceDevice, nullptr, this, nullptr);
    closeDecoder();
}

QUrl QWasmAudioDecoder::source() const
{
    return m_source;
}

void QWasmAudioDecoder::setSource(const QUrl &url)
{
    stop();
    m_source = url;
    m_sourceDevice = nullptr;
    sourceChanged();
}

QIODevice *QWasmAudioDecoder::sourceDevice() const
{
    return m_sourceDevice;
}

void QWasmAudioDecoder::setSourceDevice(QIODevice *device)
{
    stop();
    m_sourceDevice = device;
    m_source = QUrl{};
    sourceChanged();
}

QAudioFormat QWasmAudioDecoder::audioFormat() const
{
    return m_audioFormat;
}

void QWasmAudioDecoder::setAudioFormat(const QAudioFormat &format)
{
    if (m_audioFormat == format)
        return;
    m_audioFormat = format;
    formatChanged(m_audioFormat);
}

QAudioBuffer QWasmAudioDecoder::read()
{
    if (m_pendingBuffers.isEmpty())
        return {};

    QAudioBuffer buffer = m_pendingBuffers.dequeue();
    if (m_pendingBuffers.isEmpty())
        bufferAvailableChanged(false);

    // QAudioBuffer::startTime is in microseconds. Convert to milliseconds
    positionChanged(std::chrono::round<std::chrono::milliseconds>(
            std::chrono::microseconds{ buffer.startTime() }));
    return buffer;
}

void QWasmAudioDecoder::start()
{
    if (isDecoding())
        return;

    if (emscripten::val::global("AudioDecoder").isUndefined()) {
        handleError(QAudioDecoder::FormatError,
                    QStringLiteral("WebCodecs AudioDecoder is not available in this browser"));
        return;
    }

    m_pendingBuffers.clear();
    bufferAvailableChanged(false);
    ++(*m_generationPtr);
    resetDecodingState();
    m_formatEmitted = false;
    setIsDecoding(true);

    if (m_sourceDevice) {
        // STREAMING PATH. Configure the decoder once we peek the codec header,
        // then feed compressed bytes as they arrive via readyRead.
        if (!m_sourceDevice->isReadable()) {
            handleError(QAudioDecoder::ResourceError,
                        QStringLiteral("Source device is not readable"));
            return;
        }
        m_decoder = emscripten::val::take_ownership(qt_wc_create_decoder());
        connect(m_sourceDevice, &QIODevice::readyRead,
                this, &QWasmAudioDecoder::onSourceReadyRead);
        connect(m_sourceDevice, &QIODevice::aboutToClose,
                this, &QWasmAudioDecoder::onSourceDone);
        onSourceReadyRead(); // drain any bytes already buffered in the device
    } else if (!m_source.isEmpty()) {

        // FILE / QRC PATH. Read everything synchronously, then chunk-feed.
        const QString scheme = m_source.scheme();

        QByteArray data;
        if (scheme == QStringLiteral("qrc") || scheme.isEmpty()) {
            const QString path = QLatin1Char(':') + m_source.path();
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                handleError(QAudioDecoder::ResourceError,
                            QStringLiteral("Cannot open Qt resource: ") + path);
                return;
            }
            data = file.readAll();
        } else if (scheme == QStringLiteral("file")) {
            QFile file(m_source.toLocalFile());
            if (!file.open(QIODevice::ReadOnly)) {
                handleError(QAudioDecoder::ResourceError,
                            QStringLiteral("Cannot open file: ") + m_source.toLocalFile());
                return;
            }
            data = file.readAll();
        } else {
            handleError(QAudioDecoder::ResourceError,
                        QStringLiteral("http/https sources are not supported with WebCodecs; "
                                       "use a local file or QIODevice"));
            return;
        }

        if (data.isEmpty()) {
            handleError(QAudioDecoder::ResourceError,
                        QStringLiteral("Audio source data is empty"));
            return;
        }

        // Save raw data before any early returns so tryDecodeAudioData() has it
        // available as a fallback for any format WebCodecs cannot handle.
        m_rawData = data;

        const Codec codec = AudioParsing::sniffCodec(asBytes(data));
        if (codec == Codec::Unknown) {
            // Unrecognized by our sniffer (e.g. M4A, WebM), so let the browser
            // try to decode it via AudioContext.decodeAudioData.
            m_fallbackReason = QStringLiteral("no known audio codec signature (%1)")
                    .arg(dataPrefixHint(data));
            tryDecodeAudioData();
            return;
        }
        if (codec == Codec::Wav) {
            handleError(QAudioDecoder::FormatError,
                        QStringLiteral("WAV is uncompressed; use QWaveDecoder directly"));
            return;
        }
        if (codec == Codec::Vorbis) {
            // WebCodecs has no Vorbis decoder, and isConfigSupported() only
            // validates the codec string, so configuring one would succeed and
            // then fail on the first chunk. Go straight to decodeAudioData.
            m_codec = codec;
            m_fallbackReason = QStringLiteral("WebCodecs has no Vorbis decoder");
            tryDecodeAudioData();
            return;
        }

        // Check browser codec support before creating the decoder.
        // For FLAC, extract real sampleRate/numChannels and the mandatory description
        // from the STREAMINFO block, since isConfigSupported rejects without it.
        int sampleRate = m_audioFormat.isValid() ? m_audioFormat.sampleRate() : 0;
        int numChannels = m_audioFormat.isValid() ? m_audioFormat.channelCount() : 0;
        QByteArray flacDesc;
        if (codec == Codec::Flac) {
            flacDesc = flacDescription(data, sampleRate, numChannels);
            if (const std::optional<std::chrono::milliseconds> duration = flacDuration(data))
                durationChanged(*duration);
        }
        if (sampleRate <= 0)
            sampleRate = 48000;
        if (numChannels <= 0)
            numChannels = 2;

        m_checkingSupport = true;
        m_decoder = emscripten::val::take_ownership(qt_wc_create_decoder());

        auto generationPtr = m_generationPtr;
        const int generation = *generationPtr;

        emscripten::val supportConfig = emscripten::val::object();
        supportConfig.set("codec", codecString(codec));
        supportConfig.set("sampleRate", sampleRate);
        supportConfig.set("numberOfChannels", numChannels);
        if (!flacDesc.isEmpty()) {
            emscripten::val descView = emscripten::val(emscripten::typed_memory_view(
                static_cast<size_t>(flacDesc.size()),
                reinterpret_cast<const uint8_t *>(flacDesc.constData())));
            supportConfig.set("description", descView.call<emscripten::val>("slice"));
        }
        emscripten::val supportPromise = emscripten::val::global("AudioDecoder")
            .call<emscripten::val>("isConfigSupported", supportConfig);

        qstdweb::Promise(supportPromise)
            .addThenFunction([this, generationPtr, generation, codec, data, sampleRate, numChannels, flacDesc]
                             (emscripten::val result) {
                if (*generationPtr != generation)
                    return;
                m_checkingSupport = false;
                if (!result["supported"].as<bool>()) {
                    m_fallbackReason = QStringLiteral("WebCodecs does not support codec \"%1\"")
                            .arg(QLatin1StringView(codecString(codec)));
                    qWarning() << "QWasmAudioDecoder:" << m_fallbackReason
                               << "— falling back to decodeAudioData";
                    closeDecoder();
                    tryDecodeAudioData();
                    return;
                }
                m_codec = codec;
                emscripten::val decoderConfig = emscripten::val::object();
                decoderConfig.set("codec", codecString(codec));
                decoderConfig.set("sampleRate", sampleRate);
                decoderConfig.set("numberOfChannels", numChannels);
                if (!flacDesc.isEmpty()) {
                    emscripten::val descView = emscripten::val(emscripten::typed_memory_view(
                        static_cast<size_t>(flacDesc.size()),
                        reinterpret_cast<const uint8_t *>(flacDesc.constData())));
                    decoderConfig.set("description", descView.call<emscripten::val>("slice"));
                }
                m_decoder.call<void>("configure", decoderConfig);
                m_outputDrainTimer.start();
                // For codecs with parseable frame sizes, feed frame-by-frame so
                // every EncodedAudioChunk contains complete access units.
                // Other codecs (Opus) go as one chunk.
                m_inputBuffer = data;
                m_inputPos = 0;
                if (AudioParsing::hasFrameParser(m_codec))
                    feedAvailableFrames(true);
                else if (m_inputPos < m_inputBuffer.size())
                    feedChunk(QByteArray(m_inputBuffer.constData() + m_inputPos,
                                        m_inputBuffer.size() - m_inputPos));
                m_inputBuffer.clear();
                m_inputPos = 0;
                startFlush();
            })
            .addCatchFunction([this, generationPtr, generation, codec](emscripten::val) {
                if (*generationPtr != generation)
                    return;
                m_checkingSupport = false;
                m_fallbackReason =
                        QStringLiteral("WebCodecs isConfigSupported rejected codec \"%1\"")
                                .arg(QLatin1StringView(codecString(codec)));
                qWarning() << "QWasmAudioDecoder:" << m_fallbackReason
                           << "— falling back to decodeAudioData";
                closeDecoder();
                tryDecodeAudioData();
            });
    } else {
        handleError(QAudioDecoder::ResourceError,
                    QStringLiteral("No audio source specified"));
    }
}

void QWasmAudioDecoder::stop()
{
    ++(*m_generationPtr);
    if (m_sourceDevice)
        disconnect(m_sourceDevice, nullptr, this, nullptr);
    closeDecoder();
    m_pendingBuffers.clear();
    bufferAvailableChanged(false);
    resetDecodingState();
    setIsDecoding(false);
    positionChanged(kInvalidPosition);
    durationChanged(kInvalidDuration);
}

// ---------------------------------------------------------------------------

// Parses and feeds as many complete encoded frames as possible from
// m_inputBuffer starting at m_inputPos, advancing m_inputPos past each
// consumed frame. Any trailing partial frame is left for the next call.
// endOfInput says no more bytes will arrive, which is what lets the final FLAC
// frame, the one with no following frame header to bound it, be emitted.
void QWasmAudioDecoder::feedAvailableFrames(bool endOfInput)
{
    // FLAC audio frames follow the container header and the metadata blocks,
    // which must be skipped rather than scanned, because a PICTURE block can
    // hold arbitrary bytes that look like a frame header.
    if (m_codec == Codec::Flac && !m_flacHeaderSkipped) {
        const std::optional<qsizetype> audioStart =
                AudioParsing::flacAudioOffset(asBytes(m_inputBuffer));
        if (!audioStart)
            return; // the metadata chain is incomplete, wait for more data
        m_inputPos = qMax(m_inputPos, *audioStart);
        m_flacHeaderSkipped = true;
    }

    while (true) {
        std::optional<qsizetype> syncPos;
        qsizetype frameSize = 0;
        qsizetype syncWordSize = 4;

        if (m_codec == Codec::Mp3) {
            syncPos = AudioParsing::findMpegSync(asBytes(m_inputBuffer), m_inputPos);
            if (syncPos)
                frameSize = AudioParsing::mpegFrameSize(asBytes(m_inputBuffer), *syncPos);
        } else if (m_codec == Codec::Aac) {
            syncPos = AudioParsing::findAdtsSync(asBytes(m_inputBuffer), m_inputPos);
            if (syncPos)
                frameSize = AudioParsing::adtsFrameSize(asBytes(m_inputBuffer), *syncPos);
        } else if (m_codec == Codec::Flac) {
            // A FLAC frame has no length field, so it runs from one frame
            // header to the next, and the last one runs to the end of input.
            syncWordSize = 16; // the largest a FLAC frame header can be
            syncPos = AudioParsing::findFlacSync(asBytes(m_inputBuffer), m_inputPos);
            if (syncPos) {
                const std::optional<qsizetype> nextSync =
                        AudioParsing::findFlacSync(asBytes(m_inputBuffer), *syncPos + 2);
                if (nextSync) {
                    frameSize = *nextSync - *syncPos;
                } else if (endOfInput) {
                    frameSize = m_inputBuffer.size() - *syncPos;
                } else {
                    // The next frame header bounds this frame, so wait for it.
                    m_inputPos = *syncPos;
                    break;
                }
            }
        }

        if (!syncPos) {
            // No sync in the remaining buffer, so keep back the bytes a header
            // needs, in case it is split across two readyRead deliveries.
            m_inputPos = qMax(m_inputPos, m_inputBuffer.size() - (syncWordSize - 1));
            break;
        }

        if (frameSize <= 0) {
            // Sync word found but header is invalid, so skip past it and retry.
            m_inputPos = *syncPos + 1;
            continue;
        }

        if (*syncPos + frameSize > m_inputBuffer.size()) {
            // Header found but the full frame hasn't arrived yet.
            m_inputPos = *syncPos;
            break;
        }

        feedChunk(QByteArray(m_inputBuffer.constData() + *syncPos, frameSize));
        m_inputPos = *syncPos + frameSize;

        // Compact the buffer every megabyte to keep memory bounded for
        // long-running or large-file streams.
        if (m_inputPos > 1024 * 1024) {
            m_inputBuffer.remove(0, m_inputPos);
            m_inputPos = 0;
        }
    }
}

// Feeds one chunk of compressed data to the decoder.
// m_byteOffset is used as the monotonically increasing chunk timestamp.
void QWasmAudioDecoder::feedChunk(const QByteArray &chunk)
{
    // typed_memory_view creates a Uint8Array view into the WASM heap.
    // slice() with no arguments copies it into a new ArrayBuffer, which is
    // required because EncodedAudioChunk may hold a reference beyond this call.
    emscripten::val dataView = emscripten::val(emscripten::typed_memory_view(
        static_cast<size_t>(chunk.size()),
        reinterpret_cast<const uint8_t *>(chunk.constData())));

    emscripten::val chunkInit = emscripten::val::object();
    chunkInit.set("type", emscripten::val("key"));
    chunkInit.set("timestamp", m_byteOffset);
    chunkInit.set("data", dataView.call<emscripten::val>("slice")["buffer"]);

    m_decoder.call<void>("decode", emscripten::val::global("EncodedAudioChunk").new_(chunkInit));
    m_byteOffset += chunk.size();
}

// Checks browser support for codec, then configures the decoder and starts
// feeding chunks from the source device. Data that arrives in the device
// while the async check is in flight stays buffered and is drained afterwards.
// rawHeader is the peeked bytes used to extract the FLAC STREAMINFO description.
void QWasmAudioDecoder::beginDecoding(Codec codec, const QByteArray &rawHeader)
{
    m_checkingSupport = true;

    int sampleRate = m_audioFormat.isValid() ? m_audioFormat.sampleRate() : 0;
    int numChannels = m_audioFormat.isValid() ? m_audioFormat.channelCount() : 0;
    QByteArray flacDesc;
    if (codec == Codec::Flac) {
        flacDesc = flacDescription(rawHeader, sampleRate, numChannels);
        if (const std::optional<std::chrono::milliseconds> duration = flacDuration(rawHeader))
            durationChanged(*duration);
    }
    if (sampleRate <= 0)
        sampleRate = 48000;
    if (numChannels <= 0)
        numChannels = 2;

    auto generationPtr = m_generationPtr;
    const int generation = *generationPtr;

    emscripten::val supportConfig = emscripten::val::object();
    supportConfig.set("codec", codecString(codec));
    supportConfig.set("sampleRate", sampleRate);
    supportConfig.set("numberOfChannels", numChannels);
    if (!flacDesc.isEmpty()) {
        emscripten::val descView = emscripten::val(emscripten::typed_memory_view(
            static_cast<size_t>(flacDesc.size()),
            reinterpret_cast<const uint8_t *>(flacDesc.constData())));
        supportConfig.set("description", descView.call<emscripten::val>("slice"));
    }
    emscripten::val supportPromise = emscripten::val::global("AudioDecoder")
        .call<emscripten::val>("isConfigSupported", supportConfig);

    qstdweb::Promise(supportPromise)
        .addThenFunction([this, generationPtr, generation, codec, sampleRate, numChannels, flacDesc](emscripten::val result) {
            if (*generationPtr != generation)
                return;
            m_checkingSupport = false;
            if (!result["supported"].as<bool>()) {
                m_fallbackReason = QStringLiteral("WebCodecs does not support codec \"%1\"")
                        .arg(QLatin1StringView(codecString(codec)));
                qWarning() << "QWasmAudioDecoder:" << m_fallbackReason
                           << "— will fall back to decodeAudioData";
                m_useDecodeAudioData = true;
                closeDecoder();
                return;
            }
            m_codec = codec;
            emscripten::val decoderConfig = emscripten::val::object();
            decoderConfig.set("codec", codecString(codec));
            decoderConfig.set("sampleRate", sampleRate);
            decoderConfig.set("numberOfChannels", numChannels);
            if (!flacDesc.isEmpty()) {
                emscripten::val descView = emscripten::val(emscripten::typed_memory_view(
                    static_cast<size_t>(flacDesc.size()),
                    reinterpret_cast<const uint8_t *>(flacDesc.constData())));
                decoderConfig.set("description", descView.call<emscripten::val>("slice"));
            }
            m_decoder.call<void>("configure", decoderConfig);
            m_outputDrainTimer.start();
            m_configured = true;
            // Drain any bytes that arrived while the support check was in flight.
            onSourceReadyRead();
        })
        .addCatchFunction([this, generationPtr, generation, codec](emscripten::val) {
            if (*generationPtr != generation)
                return;
            m_checkingSupport = false;
            m_fallbackReason = QStringLiteral("WebCodecs isConfigSupported rejected codec \"%1\"")
                    .arg(QLatin1StringView(codecString(codec)));
            qWarning() << "QWasmAudioDecoder:" << m_fallbackReason
                       << "— will fall back to decodeAudioData";
            m_useDecodeAudioData = true;
            closeDecoder();
        });
}

// Called whenever the source QIODevice signals that new bytes are available,
// and also immediately from start() to drain bytes already buffered.
void QWasmAudioDecoder::onSourceReadyRead()
{
    if (m_flushing || m_checkingSupport || !m_sourceDevice)
        return;

    // On the first call, peek the first bytes to identify the codec without
    // consuming them. 42 bytes covers "fLaC" + STREAMINFO block header + data,
    // which is needed to extract the FLAC description for isConfigSupported.
    // If too few bytes have arrived yet, wait for the next signal.
    if (!m_configured && !m_useDecodeAudioData) {
        // Peek generously to capture all metadata blocks (STREAMINFO, SEEKTABLE,
        // VORBIS_COMMENT, etc.) needed for the FLAC description field.
        // For non-FLAC codecs 4 bytes is enough to sniff, so we over-peek harmlessly.
        const QByteArray header = m_sourceDevice->peek(65536);
        if (header.size() < 4)
            return;

        const Codec codec = AudioParsing::sniffCodec(asBytes(header));

        if (codec == Codec::Unknown) {
            // Unknown container. Accumulate everything and fall back to
            // decodeAudioData.
            m_fallbackReason = QStringLiteral("no known audio codec signature (%1)")
                    .arg(dataPrefixHint(header));
            m_useDecodeAudioData = true;
            return;
        }
        if (codec == Codec::Wav) {
            handleError(QAudioDecoder::FormatError,
                        QStringLiteral("WAV is uncompressed; use QWaveDecoder directly"));
            return;
        }
        if (codec == Codec::Vorbis) {
            // No WebCodecs Vorbis decoder, so accumulate and use decodeAudioData.
            m_codec = codec;
            m_fallbackReason = QStringLiteral("WebCodecs has no Vorbis decoder");
            m_useDecodeAudioData = true;
            return;
        }

        // For FLAC, wait until all metadata blocks have arrived before proceeding.
        if (codec == Codec::Flac && !AudioParsing::flacAudioOffset(asBytes(header)))
            return;

        // Verify codec support asynchronously. Data stays buffered in the device.
        beginDecoding(codec, header);
        return;
    }

    // Accumulate newly available bytes.
    while (m_sourceDevice->bytesAvailable() > 0)
        m_inputBuffer.append(m_sourceDevice->read(65536));

    // For frame-parsed codecs (MP3, AAC), feed complete frames immediately as
    // they arrive so the decoder makes progress without waiting for the source
    // to close.  For other codecs the buffer is fed all at once in onSourceDone().
    if (AudioParsing::hasFrameParser(m_codec))
        feedAvailableFrames(false);

    // If the device is already at end, finalize without waiting for aboutToClose.
    if (m_sourceDevice->atEnd())
        onSourceDone();
}

// Called when the source device signals aboutToClose, or when atEnd() is
// detected inside onSourceReadyRead().
void QWasmAudioDecoder::onSourceDone()
{
    if (m_flushing || m_checkingSupport)
        return;

    // Disconnect before flushing so no further readyRead calls arrive.
    if (m_sourceDevice)
        disconnect(m_sourceDevice, nullptr, this, nullptr);

    // Drain any bytes that arrived between the last readyRead and now.
    if (m_sourceDevice) {
        while (m_sourceDevice->bytesAvailable() > 0)
            m_inputBuffer.append(m_sourceDevice->read(65536));
    }

    // Save the full compressed data before clearing, so tryDecodeAudioData()
    // has it available whether we fall back now or later (from startFlush catch).
    m_rawData = m_inputBuffer;

    if (m_useDecodeAudioData) {
        m_inputBuffer.clear();
        m_inputPos = 0;
        tryDecodeAudioData();
        return;
    }

    if (AudioParsing::hasFrameParser(m_codec)) {
        // Process any remaining complete frames.  A trailing partial frame
        // (e.g. truncated file) is silently discarded.
        feedAvailableFrames(true);
    } else {
        // Opus, etc.: feed the accumulated buffer as one chunk.
        if (m_inputPos < m_inputBuffer.size())
            feedChunk(QByteArray(m_inputBuffer.constData() + m_inputPos,
                                 m_inputBuffer.size() - m_inputPos));
    }

    m_inputBuffer.clear();
    m_inputPos = 0;
    startFlush();
}

// Calls decoder.flush() and, when the returned Promise resolves, delivers all
// accumulated output buffers and signals completion.
void QWasmAudioDecoder::startFlush()
{
    if (m_flushing || m_decoder.isUndefined())
        return;
    m_flushing = true;

    auto generationPtr = m_generationPtr;
    const int generation = *generationPtr;

    qstdweb::Promise(m_decoder.call<emscripten::val>("flush"))
        .addThenFunction([this, generationPtr, generation](emscripten::val) {
            if (*generationPtr != generation)
                return;
            deliverOutputs();
            // A bufferReady() slot may have stopped or restarted the decoder.
            if (*generationPtr != generation)
                return;
            closeDecoder();
            setIsDecoding(false);
            finished();
        })
        .addCatchFunction([this, generationPtr, generation](emscripten::val err) {
            if (*generationPtr != generation)
                return;
            QString errMsg;
            if (err.isString())
                errMsg = QString::fromStdString(err.as<std::string>());
            else if (!err["message"].isUndefined())
                errMsg = QString::fromStdString(err["message"].as<std::string>());
            m_fallbackReason = QStringLiteral("WebCodecs decode failed")
                    + (errMsg.isEmpty() ? QString() : QStringLiteral(": ") + errMsg);
            qWarning() << "QWasmAudioDecoder:" << m_fallbackReason
                       << "— falling back to decodeAudioData";
            closeDecoder();
            tryDecodeAudioData();
        });
}

void QWasmAudioDecoder::deliverOutputs()
{
    if (m_decoder.isUndefined() || m_deliveringOutputs)
        return;

    emscripten::val outputs = m_decoder["_qtOutputs"];
    const int count = outputs["length"].as<int>();
    if (count == 0)
        return;

    const QScopedValueRollback deliveringGuard(m_deliveringOutputs, true);
    // A slot connected to bufferReady() may call stop(), which closes every
    // remaining AudioData, or stop() and start(), which also creates a new
    // decoder. Either way the outputs below must not be touched again.
    const int generation = *m_generationPtr;

    for (int i = 0; i < count; ++i) {
        emscripten::val output = outputs[i];

        // Error entries have an _error string property. AudioData objects do not.
        emscripten::val errorProp = output["_error"];
        if (errorProp.isString()) {
            handleError(QAudioDecoder::FormatError,
                        QStringLiteral("Decode error: ")
                        + QString::fromStdString(errorProp.as<std::string>()));
            // handleError() called closeDecoder() which sets m_decoder to
            // undefined, so stop iterating to avoid use-after-free.
            return;
        }

        const int numChannels = output["numberOfChannels"].as<int>();
        const int numFrames = output["numberOfFrames"].as<int>();
        const int sampleRate = output["sampleRate"].as<int>();
        // output["timestamp"] echoes back the synthetic byte offset given to
        // the EncodedAudioChunk, so compute the real start time from the
        // frames decoded so far.
        const qint64 startTimeUs =
                sampleRate > 0 ? m_decodedFrames * 1000000LL / sampleRate : 0;

        // Allocate the receive buffer in C++, with no Module._malloc from JS.
        const qint64 totalSamples = static_cast<qint64>(numChannels) * numFrames;
        const qint64 totalBytes = totalSamples * static_cast<qint64>(sizeof(float));
        QByteArray pcm(static_cast<qsizetype>(totalBytes), Qt::Uninitialized);

        // typed_memory_view creates a Float32Array view directly into the WASM
        // heap so copyTo() writes the decoded PCM without an extra copy.
        emscripten::val destView = emscripten::val(emscripten::typed_memory_view(
            static_cast<size_t>(totalSamples), // totalSamples fits size_t on wasm32
            reinterpret_cast<float *>(pcm.data())));
        emscripten::val copyConfig = emscripten::val::object();
        copyConfig.set("planeIndex", 0);
        copyConfig.set("format", emscripten::val("f32"));
        output.call<void>("copyTo", destView, copyConfig);
        output.call<void>("close");

        QAudioFormat outputFormat;
        outputFormat.setChannelCount(numChannels);
        outputFormat.setSampleRate(sampleRate);
        outputFormat.setSampleFormat(QAudioFormat::Float);

        if (!m_formatEmitted) {
            m_formatEmitted = true;
            m_audioFormat = outputFormat;
            formatChanged(outputFormat);
        }

        QAudioBuffer buffer(pcm, outputFormat, startTimeUs);
        m_decodedFrames += numFrames;

        const bool wasEmpty = m_pendingBuffers.isEmpty();
        m_pendingBuffers.enqueue(std::move(buffer));
        if (wasEmpty)
            bufferAvailableChanged(true);
        bufferReady();
        if (*m_generationPtr != generation || m_decoder.isUndefined())
            return;
    }

    // Remove only the entries handled above. The decoder can append more
    // while signals are emitted if a slot runs a nested event loop.
    outputs.call<void>("splice", 0, count);
}

// Falls back to Web Audio API AudioContext.decodeAudioData() when WebCodecs
// fails or is unsupported. Takes m_rawData as input and delivers one
// QAudioBuffer with the fully decoded interleaved PCM.
void QWasmAudioDecoder::tryDecodeAudioData()
{
    if (m_rawData.isEmpty()) {
        handleError(QAudioDecoder::FormatError,
                    decorateWithFallbackReason(
                            QStringLiteral("no data available for the decodeAudioData fallback")));
        return;
    }

    // Copy compressed bytes into a JS ArrayBuffer.
    emscripten::val uint8View = emscripten::val(emscripten::typed_memory_view(
        static_cast<size_t>(m_rawData.size()),
        reinterpret_cast<const uint8_t *>(m_rawData.constData())));
    emscripten::val arrayBuffer = uint8View.call<emscripten::val>("slice")["buffer"];

    // decodeAudioData resamples to the context's sample rate, which otherwise
    // defaults to the output device's rate. Pin the context to the stream's own
    // rate where it is known, so the decoded buffers keep the source's sample
    // rate instead of silently becoming whatever the device runs at.
    const int nativeRate = nativeSampleRate(m_codec, m_rawData);
    emscripten::val audioCtx = emscripten::val::undefined();
    if (nativeRate > 0) {
        emscripten::val contextOptions = emscripten::val::object();
        contextOptions.set("sampleRate", nativeRate);
        audioCtx = emscripten::val::global("AudioContext").new_(contextOptions);
    } else {
        audioCtx = emscripten::val::global("AudioContext").new_();
    }

    auto generationPtr = m_generationPtr;
    const int generation = *generationPtr;

    qstdweb::Promise(audioCtx.call<emscripten::val>("decodeAudioData", arrayBuffer))
        .addThenFunction([this, generationPtr, generation, audioCtx](emscripten::val audioBuffer) {
            if (*generationPtr != generation) {
                audioCtx.call<emscripten::val>("close");
                return;
            }

            const int numChannels = audioBuffer["numberOfChannels"].as<int>();
            const int numFrames = audioBuffer["length"].as<int>();
            const int sampleRate = audioBuffer["sampleRate"].as<int>();

            // Interleave per-channel Float32Arrays into one QByteArray.
            const qint64 totalSamples = static_cast<qint64>(numChannels) * numFrames;
            QByteArray pcm(totalSamples * static_cast<qint64>(sizeof(float)), Qt::Uninitialized);
            auto *out = reinterpret_cast<float *>(pcm.data());

            for (int ch = 0; ch < numChannels; ++ch) {
                emscripten::val channelData = audioBuffer.call<emscripten::val>("getChannelData", ch);
                // Copy the Float32Array into a temporary WASM buffer, then interleave.
                QByteArray chanBuf(numFrames * static_cast<int>(sizeof(float)), Qt::Uninitialized);
                emscripten::val destView = emscripten::val(emscripten::typed_memory_view(
                    static_cast<size_t>(numFrames),
                    reinterpret_cast<float *>(chanBuf.data())));
                destView.call<void>("set", channelData);
                const float *src = reinterpret_cast<const float *>(chanBuf.constData());
                for (int i = 0; i < numFrames; ++i)
                    out[i * numChannels + ch] = src[i];
            }

            audioCtx.call<emscripten::val>("close");

            QAudioFormat outputFormat;
            outputFormat.setChannelCount(numChannels);
            outputFormat.setSampleRate(sampleRate);
            outputFormat.setSampleFormat(QAudioFormat::Float);

            if (!m_formatEmitted) {
                m_formatEmitted = true;
                m_audioFormat = outputFormat;
                formatChanged(outputFormat);
            }

            QAudioBuffer buffer(pcm, outputFormat, 0);
            m_pendingBuffers.enqueue(std::move(buffer));
            bufferAvailableChanged(true);
            bufferReady();
            durationChanged(std::chrono::milliseconds{ numFrames * 1000LL / sampleRate });

            m_rawData.clear();
            setIsDecoding(false);
            finished();
        })
        .addCatchFunction([this, generationPtr, generation, audioCtx](emscripten::val err) {
            if (*generationPtr != generation) {
                audioCtx.call<emscripten::val>("close");
                return;
            }
            audioCtx.call<emscripten::val>("close");
            QString errMsg = QStringLiteral("decodeAudioData failed");
            if (!err["message"].isUndefined())
                errMsg += QStringLiteral(": ") + QString::fromStdString(err["message"].as<std::string>());
            handleError(QAudioDecoder::FormatError, decorateWithFallbackReason(errMsg));
        });
}

void QWasmAudioDecoder::closeDecoder()
{
    m_outputDrainTimer.stop();
    if (!m_decoder.isUndefined()) {
        // Close any AudioData objects that were never consumed (error paths).
        emscripten::val outputs = m_decoder["_qtOutputs"];
        const int length = outputs["length"].as<int>();
        for (int i = 0; i < length; ++i) {
            emscripten::val output = outputs[i];
            if (!output["close"].isUndefined())
                output.call<void>("close");
        }
        outputs.set("length", 0);

        // Guard against closing an already-closed decoder (its error callback
        // transitions the state to 'closed' before we get here).
        if (m_decoder["state"].as<std::string>() != "closed")
            m_decoder.call<void>("close");

        m_decoder = emscripten::val::undefined();
    }
}

// Returns the per-decode members to their initial values. Signals are left to
// the caller, and decoded buffers already queued stay readable.
void QWasmAudioDecoder::resetDecodingState()
{
    m_inputBuffer.clear();
    m_inputPos = 0;
    m_rawData.clear();
    m_codec = Codec::Unknown;
    m_byteOffset = 0.0;
    m_decodedFrames = 0;
    m_checkingSupport = false;
    m_flushing = false;
    m_configured = false;
    m_useDecodeAudioData = false;
    m_flacHeaderSkipped = false;
    m_fallbackReason.clear();
}

// Prefixes an error message from the decodeAudioData fallback with the reason
// the fallback was taken, so the reported error names the actual problem
// instead of only the last step that failed.
QString QWasmAudioDecoder::decorateWithFallbackReason(const QString &message) const
{
    if (m_fallbackReason.isEmpty())
        return message;
    return m_fallbackReason + QStringLiteral(", and ") + message;
}

void QWasmAudioDecoder::handleError(QAudioDecoder::Error code, const QString &message)
{
    qWarning() << "QWasmAudioDecoder:" << message;
    // Invalidate pending promise and drain callbacks, so nothing from this
    // decode, such as finished() or a fallback result, arrives after error().
    ++(*m_generationPtr);
    if (m_sourceDevice)
        disconnect(m_sourceDevice, nullptr, this, nullptr);
    closeDecoder();
    resetDecodingState();
    // error() also clears isDecoding. Like the FFmpeg backend, the duration
    // and position of the failed decode are reset afterwards.
    error(code, message);
    durationChanged(kInvalidDuration);
    positionChanged(kInvalidPosition);
}

QT_END_NAMESPACE
