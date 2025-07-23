// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegaudioencoder_p.h"
#include "qffmpegrecordingengineutils_p.h"
#include "qffmpegaudioencoderutils_p.h"
#include "qffmpegaudioinput_p.h"
#include "qffmpegencoderoptions_p.h"
#include "qffmpegmuxer_p.h"
#include "qffmpegrecordingengine_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegcodecstorage_p.h"
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegAudioEncoder, "qt.multimedia.ffmpeg.audioencoder");

namespace {
void setupStreamParameters(AVStream *stream, const Codec &codec,
                           const AVAudioFormat &requestedAudioFormat)
{
    const auto channelLayouts = codec.channelLayouts();
#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    stream->codecpar->ch_layout =
            adjustChannelLayout(channelLayouts, requestedAudioFormat.channelLayout);
#else
    stream->codecpar->channel_layout =
            adjustChannelLayout(channelLayouts, requestedAudioFormat.channelLayoutMask);
    stream->codecpar->channels = qPopulationCount(stream->codecpar->channel_layout);
#endif
    const auto sampleRates = codec.sampleRates();
    const auto sampleRate = adjustSampleRate(sampleRates, requestedAudioFormat.sampleRate);

    stream->codecpar->sample_rate = sampleRate;
    stream->codecpar->frame_size = 1024;
    const auto sampleFormats = codec.sampleFormats();
    stream->codecpar->format = adjustSampleFormat(sampleFormats, requestedAudioFormat.sampleFormat);

    stream->time_base = AVRational{ 1, sampleRate };

    qCDebug(qLcFFmpegAudioEncoder)
            << "set stream time_base" << stream->time_base.num << "/" << stream->time_base.den;
}

bool openCodecContext(AVCodecContext *codecContext, AVStream *stream,
                      const QMediaEncoderSettings &settings)
{
    Q_ASSERT(codecContext);
    codecContext->time_base = stream->time_base;

    avcodec_parameters_to_context(codecContext, stream->codecpar);

    // if avcodec_open2 fails, it may clean codecContext->codec
    Codec codec{ codecContext->codec };

    AVDictionaryHolder opts;
    applyAudioEncoderOptions(settings, QByteArray{ codec.name() }, codecContext, opts);
    applyExperimentalCodecOptions(codec, opts);

    const int res = avcodec_open2(codecContext, codec.get(), opts);

    if (res != 0) {
        qCWarning(qLcFFmpegAudioEncoder)
                << "Cannot open audio codec" << codec.name() << "; result:" << AVError(res);
        return false;
    }

    qCDebug(qLcFFmpegAudioEncoder) << "audio codec params: fmt=" << codecContext->sample_fmt
                                   << "rate=" << codecContext->sample_rate;

    avcodec_parameters_from_context(stream->codecpar, codecContext);

    return true;
}

} // namespace

AudioEncoder::AudioEncoder(RecordingEngine &recordingEngine, const QAudioFormat &sourceFormat,
                           const QMediaEncoderSettings &settings)
    : EncoderThread(recordingEngine), m_sourceFormat(sourceFormat), m_settings(settings)
{
    setObjectName(QLatin1String("AudioEncoder"));
    qCDebug(qLcFFmpegAudioEncoder) << "AudioEncoder" << settings.audioCodec();

    const AVCodecID codecID = QFFmpegMediaFormatInfo::codecIdForAudioCodec(settings.audioCodec());
    Q_ASSERT(avformat_query_codec(recordingEngine.avFormatContext()->oformat, codecID,
                                  FF_COMPLIANCE_NORMAL));

    Q_ASSERT(QFFmpeg::findAVEncoder(codecID));

    m_stream = avformat_new_stream(recordingEngine.avFormatContext(), nullptr);
    m_stream->id = recordingEngine.avFormatContext()->nb_streams - 1;
    m_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    m_stream->codecpar->codec_id = codecID;
}

void AudioEncoder::addBuffer(const QAudioBuffer &buffer)
{
    if (!buffer.isValid()) {
        setEndOfSourceStream();
        return;
    }

    {
        const std::chrono::microseconds bufferDuration(buffer.duration());
        auto guard = lockLoopData();

        resetEndOfSourceStream();

        if (m_paused)
            return;

        // TODO: apply logic with canPushFrame

        m_audioBufferQueue.push(buffer);
        m_queueDuration += bufferDuration;
    }

    dataReady();
}

QAudioBuffer AudioEncoder::takeBuffer()
{
    auto locker = lockLoopData();
    QAudioBuffer result = dequeueIfPossible(m_audioBufferQueue);
    m_queueDuration -= std::chrono::microseconds(result.duration());
    return result;
}

bool AudioEncoder::init()
{
    const AVAudioFormat requestedAudioFormat(m_sourceFormat);

    QFFmpeg::findAndOpenAVEncoder(
            m_stream->codecpar->codec_id,
            [&](const Codec &codec) {
                AVScore result = DefaultAVScore;

                // Attempt to find no-conversion format
                if (auto fmts = codec.sampleFormats(); !fmts.empty())
                    result += hasValue(fmts, requestedAudioFormat.sampleFormat) ? 1 : -1;

                if (auto rates = codec.sampleRates(); !rates.empty())
                    result += hasValue(rates, requestedAudioFormat.sampleRate) ? 1 : -1;

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
                if (auto layouts = codec.channelLayouts(); !layouts.empty())
                    result += hasValue(layouts, requestedAudioFormat.channelLayout) ? 1 : -1;
#else
                if (auto layouts = codec.channelLayouts(); !layouts.empty())
                    result += hasValue(layouts, requestedAudioFormat.channelLayoutMask) ? 1 : -1;
#endif

                return result;
            },
            [&](const Codec &codec) {
                AVCodecContextUPtr codecContext(avcodec_alloc_context3(codec.get()));
                if (!codecContext)
                    return false;

                setupStreamParameters(m_stream, Codec{ codecContext->codec }, requestedAudioFormat);
                if (!openCodecContext(codecContext.get(), m_stream, m_settings))
                    return false;

                m_codecContext = std::move(codecContext);
                return true;
            });

    if (!m_codecContext) {
        qCWarning(qLcFFmpegAudioEncoder) << "Unable to open any audio codec";
        emit m_recordingEngine.sessionError(QMediaRecorder::FormatError,
                                            QStringLiteral("Cannot open any audio codec"));
        return false;
    }

    qCDebug(qLcFFmpegAudioEncoder) << "found audio codec" << m_codecContext->codec->name;

    updateResampler(m_sourceFormat);

    // TODO: try to address this dependency here.
    if (auto input = qobject_cast<QFFmpegAudioInput *>(source()))
        input->setBufferSize(m_codecContext->frame_size);

    return EncoderThread::init();
}

void AudioEncoder::cleanup()
{
    while (!m_audioBufferQueue.empty())
        processOne();

    if (m_avFrameSamplesOffset) {
        // the size of the last frame can be less than m_codecContext->frame_size

        retrievePackets();
        sendPendingFrameToAVCodec();
    }

    while (avcodec_send_frame(m_codecContext.get(), nullptr) == AVERROR(EAGAIN))
        retrievePackets();
    retrievePackets();
}

bool AudioEncoder::hasData() const
{
    return !m_audioBufferQueue.empty();
}

void AudioEncoder::retrievePackets()
{
    while (true) {
        AVPacketUPtr packet(av_packet_alloc());
        int ret = avcodec_receive_packet(m_codecContext.get(), packet.get());
        if (ret < 0) {
            if (ret != AVERROR(EOF))
                break;
            if (ret != AVERROR(EAGAIN)) {
                char errStr[1024];
                av_strerror(ret, errStr, 1024);
                qCDebug(qLcFFmpegAudioEncoder) << "receive packet" << ret << errStr;
            }
            break;
        }

        // qCDebug(qLcFFmpegEncoder) << "writing audio packet" << packet->size << packet->pts <<
        // packet->dts;
        packet->stream_index = m_stream->id;
        m_recordingEngine.getMuxer()->addPacket(std::move(packet));
    }
}

void AudioEncoder::processOne()
{
    QAudioBuffer buffer = takeBuffer();
    Q_ASSERT(buffer.isValid());

    //    qCDebug(qLcFFmpegEncoder) << "new audio buffer" << buffer.byteCount() << buffer.format()
    //    << buffer.frameCount() << codec->frame_size;

    if (buffer.format() != m_sourceFormat && !updateResampler(buffer.format()))
        return;

    int samplesOffset = 0;
    const int bufferSamplesCount = static_cast<int>(buffer.frameCount());

    while (samplesOffset < bufferSamplesCount)
        handleAudioData(buffer.constData<uint8_t>(), samplesOffset, bufferSamplesCount);

    Q_ASSERT(samplesOffset == bufferSamplesCount);
}

bool AudioEncoder::checkIfCanPushFrame() const
{
    if (m_encodingStarted)
        return m_audioBufferQueue.size() <= 1 || m_queueDuration < m_maxQueueDuration;
    if (!isFinished())
        return m_audioBufferQueue.empty();

    return false;
}

bool AudioEncoder::updateResampler(const QAudioFormat &sourceFormat)
{
    m_resampler.reset();

    const AVAudioFormat requestedAudioFormat(sourceFormat);
    const AVAudioFormat codecAudioFormat(m_codecContext.get());

    if (requestedAudioFormat != codecAudioFormat) {
        m_resampler = createResampleContext(requestedAudioFormat, codecAudioFormat);
        if (!swr_is_initialized(m_resampler.get())) {
            m_sourceFormat = {};
            qCWarning(qLcFFmpegAudioEncoder) << "Cannot initialize resampler for audio encoder";
            emit m_recordingEngine.sessionError(
                    QMediaRecorder::FormatError,
                    QStringLiteral("Cannot initialize resampler for audio encoder"));
            return false;
        }
        qCDebug(qLcFFmpegAudioEncoder) << "Created resampler with audio formats conversion\n"
                                       << requestedAudioFormat << "->" << codecAudioFormat;
    } else {
        qCDebug(qLcFFmpegAudioEncoder) << "Resampler is not needed due to no-conversion format\n"
                                       << requestedAudioFormat;
    }

    m_sourceFormat = sourceFormat;

    return true;
}

void AudioEncoder::ensurePendingFrame(int availableSamplesCount)
{
    Q_ASSERT(availableSamplesCount >= 0);

    if (m_avFrame)
        return;

    m_avFrame = makeAVFrame();

    m_avFrame->format = m_codecContext->sample_fmt;
#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    m_avFrame->ch_layout = m_codecContext->ch_layout;
#else
    m_avFrame->channel_layout = m_codecContext->channel_layout;
    m_avFrame->channels = m_codecContext->channels;
#endif
    m_avFrame->sample_rate = m_codecContext->sample_rate;

    const bool isFixedFrameSize =
            !(m_codecContext->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
            && m_codecContext->frame_size;
    m_avFrame->nb_samples = isFixedFrameSize ? m_codecContext->frame_size : availableSamplesCount;
    if (m_avFrame->nb_samples)
        av_frame_get_buffer(m_avFrame.get(), 0);

    const auto &timeBase = m_stream->time_base;
    const auto pts = timeBase.den && timeBase.num
            ? timeBase.den * m_samplesWritten / (m_codecContext->sample_rate * timeBase.num)
            : m_samplesWritten;
    setAVFrameTime(*m_avFrame, pts, timeBase);
}

void AudioEncoder::writeDataToPendingFrame(const uchar *data, int &samplesOffset, int samplesCount)
{
    Q_ASSERT(m_avFrame);
    Q_ASSERT(m_avFrameSamplesOffset <= m_avFrame->nb_samples);

    const int bytesPerSample = av_get_bytes_per_sample(m_codecContext->sample_fmt);
    const bool isPlanar = av_sample_fmt_is_planar(m_codecContext->sample_fmt);

#if QT_FFMPEG_HAS_AV_CHANNEL_LAYOUT
    const int channelsCount = m_codecContext->ch_layout.nb_channels;
#else
    const int channelsCount = m_codecContext->channels;
#endif

    const int audioDataOffset = isPlanar ? bytesPerSample * m_avFrameSamplesOffset
                                         : bytesPerSample * m_avFrameSamplesOffset * channelsCount;

    const int planesCount = isPlanar ? channelsCount : 1;
    m_avFramePlanesData.resize(planesCount);
    for (int plane = 0; plane < planesCount; ++plane)
        m_avFramePlanesData[plane] = m_avFrame->extended_data[plane] + audioDataOffset;

    const int samplesToWrite = m_avFrame->nb_samples - m_avFrameSamplesOffset;
    int samplesToRead =
            (samplesToWrite * m_sourceFormat.sampleRate() + m_codecContext->sample_rate / 2)
            / m_codecContext->sample_rate;
    // the lower bound is need to get round infinite loops in corner cases
    samplesToRead = qBound(1, samplesToRead, samplesCount - samplesOffset);

    data += m_sourceFormat.bytesForFrames(samplesOffset);

    if (m_resampler) {
        m_avFrameSamplesOffset += swr_convert(m_resampler.get(), m_avFramePlanesData.data(),
                                              samplesToWrite, &data, samplesToRead);
    } else {
        Q_ASSERT(planesCount == 1);
        m_avFrameSamplesOffset += samplesToRead;
        memcpy(m_avFramePlanesData[0], data, m_sourceFormat.bytesForFrames(samplesToRead));
    }

    samplesOffset += samplesToRead;
}

void AudioEncoder::sendPendingFrameToAVCodec()
{
    Q_ASSERT(m_avFrame);
    Q_ASSERT(m_avFrameSamplesOffset <= m_avFrame->nb_samples);

    m_avFrame->nb_samples = m_avFrameSamplesOffset;

    m_samplesWritten += m_avFrameSamplesOffset;

    const qint64 time = m_sourceFormat.durationForFrames(
            m_samplesWritten * m_sourceFormat.sampleRate() / m_codecContext->sample_rate);
    m_recordingEngine.newTimeStamp(time / 1000);

    // qCDebug(qLcFFmpegEncoder) << "sending audio frame" << buffer.byteCount() << frame->pts <<
    //   ((double)buffer.frameCount()/frame->sample_rate);

    int ret = avcodec_send_frame(m_codecContext.get(), m_avFrame.get());
    if (ret < 0) {
        char errStr[AV_ERROR_MAX_STRING_SIZE];
        av_strerror(ret, errStr, AV_ERROR_MAX_STRING_SIZE);
        qCDebug(qLcFFmpegAudioEncoder) << "error sending frame" << ret << errStr;
    }

    m_avFrame = nullptr;
    m_avFrameSamplesOffset = 0;
    std::fill(m_avFramePlanesData.begin(), m_avFramePlanesData.end(), nullptr);
}

void AudioEncoder::handleAudioData(const uchar *data, int &samplesOffset, int samplesCount)
{
    ensurePendingFrame(samplesCount - samplesOffset);

    writeDataToPendingFrame(data, samplesOffset, samplesCount);

    // The frame is not ready yet
    if (m_avFrameSamplesOffset < m_avFrame->nb_samples)
        return;

    retrievePackets();

    sendPendingFrameToAVCodec();
}

} // namespace QFFmpeg

QT_END_NAMESPACE
