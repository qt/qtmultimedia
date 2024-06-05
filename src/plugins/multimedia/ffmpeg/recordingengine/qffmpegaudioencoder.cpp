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
#include <QtCore/qloggingcategory.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

Q_STATIC_LOGGING_CATEGORY(qLcFFmpegAudioEncoder, "qt.multimedia.ffmpeg.audioencoder");

AudioEncoder::AudioEncoder(RecordingEngine &recordingEngine, const QAudioFormat &sourceFormat,
                           const QMediaEncoderSettings &settings)
    : EncoderThread(recordingEngine), m_format(sourceFormat), m_settings(settings)
{
    setObjectName(QLatin1String("AudioEncoder"));
    qCDebug(qLcFFmpegAudioEncoder) << "AudioEncoder" << settings.audioCodec();

    auto codecID = QFFmpegMediaFormatInfo::codecIdForAudioCodec(settings.audioCodec());
    Q_ASSERT(avformat_query_codec(recordingEngine.avFormatContext()->oformat, codecID,
                                  FF_COMPLIANCE_NORMAL));

    const AVAudioFormat requestedAudioFormat(m_format);

    m_avCodec = QFFmpeg::findAVEncoder(codecID, {}, requestedAudioFormat.sampleFormat);

    if (!m_avCodec)
        m_avCodec = QFFmpeg::findAVEncoder(codecID);

    qCDebug(qLcFFmpegAudioEncoder) << "found audio codec" << m_avCodec->name;

    Q_ASSERT(m_avCodec);

    m_stream = avformat_new_stream(recordingEngine.avFormatContext(), nullptr);
    m_stream->id = recordingEngine.avFormatContext()->nb_streams - 1;
    m_stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    m_stream->codecpar->codec_id = codecID;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    m_stream->codecpar->channel_layout =
            adjustChannelLayout(m_avCodec->channel_layouts, requestedAudioFormat.channelLayoutMask);
    m_stream->codecpar->channels = qPopulationCount(m_stream->codecpar->channel_layout);
#else
    m_stream->codecpar->ch_layout =
            adjustChannelLayout(m_avCodec->ch_layouts, requestedAudioFormat.channelLayout);
#endif
    const auto sampleRate =
            adjustSampleRate(m_avCodec->supported_samplerates, requestedAudioFormat.sampleRate);

    m_stream->codecpar->sample_rate = sampleRate;
    m_stream->codecpar->frame_size = 1024;
    m_stream->codecpar->format =
            adjustSampleFormat(m_avCodec->sample_fmts, requestedAudioFormat.sampleFormat);

    m_stream->time_base = AVRational{ 1, sampleRate };

    qCDebug(qLcFFmpegAudioEncoder) << "set stream time_base" << m_stream->time_base.num << "/"
                              << m_stream->time_base.den;
}

void AudioEncoder::open()
{
    m_codecContext.reset(avcodec_alloc_context3(m_avCodec));

    if (m_stream->time_base.num != 1 || m_stream->time_base.den != m_format.sampleRate()) {
        qCDebug(qLcFFmpegAudioEncoder) << "Most likely, av_format_write_header changed time base from"
                                  << 1 << "/" << m_format.sampleRate() << "to"
                                  << m_stream->time_base;
    }

    m_codecContext->time_base = m_stream->time_base;

    avcodec_parameters_to_context(m_codecContext.get(), m_stream->codecpar);

    AVDictionaryHolder opts;
    applyAudioEncoderOptions(m_settings, m_avCodec->name, m_codecContext.get(), opts);
    applyExperimentalCodecOptions(m_avCodec, opts);

    const int res = avcodec_open2(m_codecContext.get(), m_avCodec, opts);

    qCDebug(qLcFFmpegAudioEncoder) << "audio codec opened" << res;
    qCDebug(qLcFFmpegAudioEncoder) << "audio codec params: fmt=" << m_codecContext->sample_fmt
                              << "rate=" << m_codecContext->sample_rate;

    updateResampler();
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

void AudioEncoder::init()
{
    open();

    // TODO: try to address this dependency here.
    if (auto input = qobject_cast<QFFmpegAudioInput *>(source()))
        input->setFrameSize(m_codecContext->frame_size);

    qCDebug(qLcFFmpegAudioEncoder) << "AudioEncoder::init started audio device thread.";
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
    while (1) {
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

    if (buffer.format() != m_format) {
        m_format = buffer.format();
        updateResampler();
    }

    int samplesOffset = 0;
    const int bufferSamplesCount = static_cast<int>(buffer.frameCount());

    while (samplesOffset < bufferSamplesCount)
        handleAudioData(buffer.constData<uint8_t>(), samplesOffset, bufferSamplesCount);

    Q_ASSERT(samplesOffset == bufferSamplesCount);
}

bool AudioEncoder::checkIfCanPushFrame() const
{
    if (isRunning())
        return m_audioBufferQueue.size() <= 1 || m_queueDuration < m_maxQueueDuration;
    if (!isFinished())
        return m_audioBufferQueue.empty();

    return false;
}

void AudioEncoder::updateResampler()
{
    m_resampler.reset();

    const AVAudioFormat requestedAudioFormat(m_format);
    const AVAudioFormat codecAudioFormat(m_codecContext.get());

    if (requestedAudioFormat != codecAudioFormat)
        m_resampler = createResampleContext(requestedAudioFormat, codecAudioFormat);

    qCDebug(qLcFFmpegAudioEncoder)
            << "Resampler updated. Input format:" << m_format << "Resampler:" << m_resampler.get();
}

void AudioEncoder::ensurePendingFrame(int availableSamplesCount)
{
    Q_ASSERT(availableSamplesCount >= 0);

    if (m_avFrame)
        return;

    m_avFrame = makeAVFrame();

    m_avFrame->format = m_codecContext->sample_fmt;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    m_avFrame->channel_layout = m_codecContext->channel_layout;
    m_avFrame->channels = m_codecContext->channels;
#else
    m_avFrame->ch_layout = m_codecContext->ch_layout;
#endif
    m_avFrame->sample_rate = m_codecContext->sample_rate;

    const bool isFixedFrameSize = !(m_avCodec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
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

#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    const int channelsCount = m_codecContext->channels;
#else
    const int channelsCount = m_codecContext->ch_layout.nb_channels;
#endif

    const int audioDataOffset = isPlanar ? bytesPerSample * m_avFrameSamplesOffset
                                         : bytesPerSample * m_avFrameSamplesOffset * channelsCount;

    const int planesCount = isPlanar ? channelsCount : 1;
    m_avFramePlanesData.resize(planesCount);
    for (int plane = 0; plane < planesCount; ++plane)
        m_avFramePlanesData[plane] = m_avFrame->extended_data[plane] + audioDataOffset;

    const int samplesToRead =
            std::min(m_avFrame->nb_samples - m_avFrameSamplesOffset, samplesCount - samplesOffset);

    data += m_format.bytesForFrames(samplesOffset);

    if (m_resampler) {
        m_avFrameSamplesOffset += swr_convert(m_resampler.get(), m_avFramePlanesData.data(),
                                              samplesToRead, &data, samplesToRead);
    } else {
        Q_ASSERT(planesCount == 1);
        m_avFrameSamplesOffset += samplesToRead;
        memcpy(m_avFramePlanesData[0], data, m_format.bytesForFrames(samplesToRead));
    }

    samplesOffset += samplesToRead;
}

void AudioEncoder::sendPendingFrameToAVCodec()
{
    Q_ASSERT(m_avFrame);
    Q_ASSERT(m_avFrameSamplesOffset <= m_avFrame->nb_samples);

    m_avFrame->nb_samples = m_avFrameSamplesOffset;

    m_samplesWritten += m_avFrameSamplesOffset;

    const qint64 time = m_format.durationForFrames(m_samplesWritten);
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
