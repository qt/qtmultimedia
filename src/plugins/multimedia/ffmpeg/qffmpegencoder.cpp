/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "qffmpegencoder_p.h"
#include "qffmpegmediaformatinfo_p.h"

#include <qdebug.h>
#include <qiodevice.h>
#include <qaudiosource.h>
#include <qaudiobuffer.h>
#include "qffmpegaudioinput_p.h"

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcFFmpegEncoder, "qt.multimedia.ffmpeg.encoder")

namespace QFFmpeg
{

Encoder::Encoder(const QMediaEncoderSettings &settings, const QUrl &url)
    : settings(settings)
{
    auto *avFormat = QFFmpegMediaFormatInfo::outputFormatForFileFormat(settings.fileFormat());

    formatContext = avformat_alloc_context();
    formatContext->oformat = avFormat;

    QByteArray encoded = url.toEncoded();
    formatContext->url = (char *)av_malloc(encoded.size() + 1);
    memcpy(formatContext->url, encoded.constData(), encoded.size() + 1);
    formatContext->pb = nullptr;
    avio_open2(&formatContext->pb, formatContext->url, AVIO_FLAG_WRITE, nullptr, nullptr);
    qDebug() << "opened" << formatContext->url;

    muxer = new Muxer(this);
}

Encoder::~Encoder()
{
    if (formatContext)
        finalize();
}

void Encoder::addAudioInput(QFFmpegAudioInput *input)
{
    audioEncode = new AudioEncoder(this, input, settings);
    connect(input, &QFFmpegAudioInput::newAudioBuffer, this, &Encoder::newAudioBuffer);
}

void Encoder::start()
{
    qDebug() << "Encoder::start!";
    int res = avformat_write_header(formatContext, nullptr);
    if (res < 0)
        qWarning() << "could not write header" << res;

    muxer->start();
    if (audioEncode)
        audioEncode->start();
}

void Encoder::finalize()
{
    qDebug() << ">>>>>>>>>>>>>>> finalize";

    if (audioEncode) {
        audioEncode->kill();
        audioEncode->wait();
    }
    muxer->kill();
    muxer->wait();

    int res = av_write_trailer(formatContext);
    if (res < 0)
        qWarning() << "could not write trailer" << res;

    avformat_free_context(formatContext);
    formatContext = nullptr;
}

void Encoder::newAudioBuffer(const QAudioBuffer &buffer)
{
    if (audioEncode)
        audioEncode->addBuffer(buffer);
}

Muxer::Muxer(Encoder *encoder)
    : encoder(encoder)
{
    setObjectName(QLatin1String("Muxer"));
}

void Muxer::addPacket(AVPacket *packet)
{
    qDebug() << "Muxer::addPacket" << packet->pts;
    QMutexLocker locker(&queueMutex);
    packetQueue.enqueue(packet);
    wake();
}

AVPacket *Muxer::takePacket()
{
    QMutexLocker locker(&queueMutex);
    if (packetQueue.isEmpty())
        return nullptr;
    qDebug() << "Muxer::takePacket" << packetQueue.first()->pts;
    return packetQueue.dequeue();
}

void Muxer::init()
{
}

void Muxer::cleanup()
{
}

bool QFFmpeg::Muxer::shouldWait() const
{
    QMutexLocker locker(&queueMutex);
    return packetQueue.isEmpty();
}

void Muxer::loop()
{
    auto *packet = takePacket();
    qDebug() << "writing packet to file" << packet->pts << packet->duration << packet->stream_index;
    av_interleaved_write_frame(encoder->formatContext, packet);
}


static AVSampleFormat bestMatchingSampleFormat(AVSampleFormat requested, const AVSampleFormat *available)
{
    if (!available)
        return requested;

    const AVSampleFormat *f = available;
    AVSampleFormat best = *f;

    enum {
        First,
        Planar,
        Exact,
    } score = First;

    for (; *f != AV_SAMPLE_FMT_NONE; ++f) {
        qDebug() << "format:" << *f;
        if (*f == requested) {
            best = *f;
            score = Exact;
            break;
        }

        if (av_get_planar_sample_fmt(requested) == *f) {
            score = Planar;
            best = *f;
        }
    }
    return best;
}

AudioEncoder::AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input, const QMediaEncoderSettings &settings)
    : encoder(encoder)
    , input(input)
{
    setObjectName(QLatin1String("AudioEncoder"));
    qDebug() << "AudioEncoder" << settings.audioCodec();

    format = input->device.preferredFormat();
    auto codecID = QFFmpegMediaFormatInfo::codecIdForAudioCodec(settings.audioCodec());
    Q_ASSERT(avformat_query_codec(encoder->formatContext->oformat, codecID, FF_COMPLIANCE_NORMAL));

    auto *avCodec = avcodec_find_encoder(codecID);

    AVSampleFormat requested = QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat());
    AVSampleFormat bestSampleFormat = bestMatchingSampleFormat(requested, avCodec->sample_fmts);

    stream = avformat_new_stream(encoder->formatContext, nullptr);
    stream->id = encoder->formatContext->nb_streams - 1;
    stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    stream->codecpar->codec_id = codecID;
    stream->codecpar->channel_layout = av_get_default_channel_layout(format.channelCount());
    stream->codecpar->channels = format.channelCount();
    stream->codecpar->sample_rate = format.sampleRate();
    stream->codecpar->frame_size = 1024;
    stream->codecpar->format = bestSampleFormat;
    stream->time_base = AVRational{ 1, format.sampleRate() };

    Q_ASSERT(avCodec);
    codec = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(codec, stream->codecpar);
    int res = avcodec_open2(codec, avCodec, nullptr);
    qDebug() << "audio codec opened" << res;

    if (codec->sample_fmt != requested) {
        resampler = swr_alloc_set_opts(nullptr,  // we're allocating a new context
                                       codec->channel_layout,  // out_ch_layout
                                       codec->sample_fmt,    // out_sample_fmt
                                       codec->sample_rate,                // out_sample_rate
                                       av_get_default_channel_layout(format.channelCount()), // in_ch_layout
                                       requested,   // in_sample_fmt
                                       format.sampleRate(),                // in_sample_rate
                                       0,                    // log_offset
                                       nullptr);
        swr_init(resampler);
    }

}

void AudioEncoder::addBuffer(const QAudioBuffer &buffer)
{
    QMutexLocker locker(&queueMutex);
    audioBufferQueue.enqueue(buffer);
    wake();
}

QAudioBuffer AudioEncoder::takeBuffer()
{
    QMutexLocker locker(&queueMutex);
    if (audioBufferQueue.isEmpty())
        return QAudioBuffer();
    return audioBufferQueue.dequeue();
}

void AudioEncoder::init()
{
    if (input) {
        input->setFrameSize(codec->frame_size);
        input->setRunning(true);
    }
    qDebug() << "AudioEncoder::init started audio device thread.";
}

void AudioEncoder::cleanup()
{
    input->setRunning(false);
    while (!audioBufferQueue.isEmpty())
        loop();
    int ret = avcodec_send_frame(codec, nullptr);
    if (ret != 0)
        qWarning() << "error sending final audio packet" << ret;
}

bool AudioEncoder::shouldWait() const
{
    QMutexLocker locker(&queueMutex);
    return audioBufferQueue.isEmpty();
}

void AudioEncoder::loop()
{
    QAudioBuffer buffer = takeBuffer();
    if (!buffer.isValid())
        return;

    qDebug() << "new audio buffer" << buffer.byteCount() << buffer.format() << buffer.frameCount() << codec->frame_size;
    while (1) {
        AVPacket *packet = av_packet_alloc();
        int ret = avcodec_receive_packet(codec, packet);
        if (ret < 0) {
            av_packet_unref(packet);
            if (ret != AVERROR(EAGAIN)) {
                char errStr[1024];
                av_strerror(ret, errStr, 1024);
                qDebug() << "receive frame" << ret << errStr;
            }
            break;
        }
        qDebug() << "writing packet" << packet->size << packet->pts;
        encoder->muxer->addPacket(packet);
    }


    AVFrame *frame = av_frame_alloc();
    frame->format = codec->sample_fmt;
    frame->channel_layout = codec->channel_layout;
    frame->channels = codec->channels;
    frame->sample_rate = codec->sample_rate;
    frame->nb_samples = buffer.frameCount();
    if (frame->nb_samples)
        av_frame_get_buffer(frame, 0);

    if (resampler) {
        const uint8_t *data = buffer.constData<uint8_t>();
        swr_convert(resampler, frame->extended_data, frame->nb_samples, &data, frame->nb_samples);
    } else {
        memcpy(frame->buf[0]->data, buffer.constData<uint8_t>(), buffer.byteCount());
    }

    qDebug() << buffer.byteCount() << frame->buf[0]->size;
    frame->pts = samplesWritten;
    samplesWritten += buffer.sampleCount();

    qDebug() << "sending frame" << buffer.byteCount();
    int ret = avcodec_send_frame(codec, frame);
    if (ret < 0) {
        char errStr[1024];
        av_strerror(ret, errStr, 1024);
        qDebug() << "error sending frame" << ret << errStr;
    }
}

}

QT_END_NAMESPACE
