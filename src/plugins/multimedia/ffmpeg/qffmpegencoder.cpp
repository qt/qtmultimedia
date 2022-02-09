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
#include <private/qplatformcamera_p.h>
#include "qffmpegvideobuffer_p.h"

#include <qloggingcategory.h>

extern "C" {
#include <libavutil/pixdesc.h>
}

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
    input->setRunning(true);
}

void Encoder::addVideoSource(QPlatformCamera *source)
{
    videoEncode = new VideoEncoder(this, source, settings);
    connect(source, &QPlatformCamera::newVideoFrame, this, &Encoder::newVideoFrame);
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
    if (videoEncode)
        videoEncode->start();
    isRecording = true;
}

void Encoder::finalize()
{
    qDebug() << ">>>>>>>>>>>>>>> finalize";

    isRecording = false;
    if (audioEncode) {
        audioEncode->kill();
        audioEncode->wait();
    }
    if (videoEncode) {
        videoEncode->kill();
        videoEncode->wait();
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
    if (audioEncode && isRecording)
        audioEncode->addBuffer(buffer);
}

void Encoder::newVideoFrame(const QVideoFrame &frame)
{
    if (videoEncode && isRecording)
        videoEncode->addFrame(frame);
}

Muxer::Muxer(Encoder *encoder)
    : encoder(encoder)
{
    setObjectName(QLatin1String("Muxer"));
}

void Muxer::addPacket(AVPacket *packet)
{
//    qDebug() << "Muxer::addPacket" << packet->pts << packet->stream_index;
    QMutexLocker locker(&queueMutex);
    packetQueue.enqueue(packet);
    wake();
}

AVPacket *Muxer::takePacket()
{
    QMutexLocker locker(&queueMutex);
    if (packetQueue.isEmpty())
        return nullptr;
//    qDebug() << "Muxer::takePacket" << packetQueue.first()->pts;
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
//    qDebug() << "writing packet to file" << packet->pts << packet->duration << packet->stream_index;
    av_interleaved_write_frame(encoder->formatContext, packet);
}


static AVSampleFormat bestMatchingSampleFormat(AVSampleFormat requested, const AVSampleFormat *available)
{
    if (!available)
        return requested;

    const AVSampleFormat *f = available;
    AVSampleFormat best = *f;
/*
    enum {
        First,
        Planar,
        Exact,
    } score = First;
*/
    for (; *f != AV_SAMPLE_FMT_NONE; ++f) {
        qDebug() << "format:" << *f;
        if (*f == requested) {
            best = *f;
//            score = Exact;
            break;
        }

        if (av_get_planar_sample_fmt(requested) == *f) {
//            score = Planar;
            best = *f;
        }
    }
    return best;
}


void EncoderThread::retrievePackets()
{
    while (1) {
        AVPacket *packet = av_packet_alloc();
        int ret = avcodec_receive_packet(codec, packet);
        if (ret < 0) {
            av_packet_unref(packet);
            if (ret != AVERROR(EOF))
                break;
            if (ret != AVERROR(EAGAIN)) {
                char errStr[1024];
                av_strerror(ret, errStr, 1024);
                qDebug() << "receive packet" << ret << errStr;
            }
            break;
        }

        //        qDebug() << "writing video packet" << packet->size << packet->pts << timeStamp(packet->pts, stream->time_base) << packet->stream_index;
        packet->stream_index = stream->id;
        encoder->muxer->addPacket(packet);
    }
}

void EncoderThread::cleanup()
{
    while (avcodec_send_frame(codec, nullptr) == AVERROR(EAGAIN))
        retrievePackets();
    retrievePackets();
}


AudioEncoder::AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input, const QMediaEncoderSettings &settings)
    : input(input)
{
    this->encoder = encoder;

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
    }
    qDebug() << "AudioEncoder::init started audio device thread.";
}

void AudioEncoder::cleanup()
{
    while (!audioBufferQueue.isEmpty())
        loop();
    EncoderThread::cleanup();
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

//    qDebug() << "new audio buffer" << buffer.byteCount() << buffer.format() << buffer.frameCount() << codec->frame_size;
    retrievePackets();

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

    frame->pts = samplesWritten;
    samplesWritten += buffer.frameCount();

//    qDebug() << "sending audio frame" << buffer.byteCount() << frame->pts << ((double)buffer.frameCount()/frame->sample_rate);
    int ret = avcodec_send_frame(codec, frame);
    if (ret < 0) {
        char errStr[1024];
        av_strerror(ret, errStr, 1024);
//        qDebug() << "error sending frame" << ret << errStr;
    }
}

VideoEncoder::VideoEncoder(Encoder *encoder, QPlatformCamera *camera, const QMediaEncoderSettings &settings)
{
    this->encoder = encoder;

    setObjectName(QLatin1String("VideoEncoder"));
    qDebug() << "VideoEncoder" << settings.videoCodec();

    auto format = camera->cameraFormat();
    auto codecID = QFFmpegMediaFormatInfo::codecIdForVideoCodec(settings.videoCodec());
    Q_ASSERT(avformat_query_codec(encoder->formatContext->oformat, codecID, FF_COMPLIANCE_NORMAL));

    auto *avCodec = avcodec_find_encoder(codecID);
    if (!avCodec) {
        qWarning() << "Could not find encoder for codecId" << codecID;
        return;
    }

    auto cameraFormat = QFFmpegVideoBuffer::toAVPixelFormat(format.pixelFormat());
    auto encoderFormat = cameraFormat;

    auto supportsFormat = [&](AVPixelFormat fmt) {
        auto *f = avCodec->pix_fmts;
        while (*f != -1) {
            if (*f == fmt)
                return true;
            ++f;
        }
        return false;
    };

    if (!supportsFormat(cameraFormat)) {
        // Take first format the encoder supports. Might want to improve upon this
        encoderFormat = *avCodec->pix_fmts;
    }

    QSize resolution = format.resolution();

    stream = avformat_new_stream(encoder->formatContext, nullptr);
    stream->id = encoder->formatContext->nb_streams - 1;
    //qDebug() << "Video stream: index" << stream->id;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    stream->codecpar->codec_id = codecID;
    // ### Fix hardcoded values
    stream->codecpar->format = encoderFormat;
    stream->codecpar->width = resolution.width();
    stream->codecpar->height = resolution.height();
    stream->codecpar->sample_aspect_ratio = AVRational{1, 1};
    float requestedRate = format.maxFrameRate();
    stream->time_base = AVRational{ 1, (int)(requestedRate*1000) };

    float delta = 1e10;
    if (avCodec->supported_framerates) {
        // codec only supports fixed frame rates
        auto *f = avCodec->supported_framerates;
        auto *best = f;
        qDebug() << "Finding fixed rate:";
        while (f->num != 0) {
            float rate = float(f->num)/float(f->den);
            float d = qAbs(rate - requestedRate);
            qDebug() << "    " << f->num << f->den << d;
            if (d < delta) {
                best = f;
                delta = d;
            }
            ++f;
        }
        qDebug() << "Fixed frame rate required. Requested:" << requestedRate << "Using:" << best->num << "/" << best->den;
        stream->time_base = { best->den, best->num };
    }

    Q_ASSERT(avCodec);
    codec = avcodec_alloc_context3(avCodec);
    avcodec_parameters_to_context(codec, stream->codecpar);
    codec->time_base = stream->time_base;
    int res = avcodec_open2(codec, avCodec, nullptr);
    if (res < 0) {
        avcodec_free_context(&codec);
        qWarning() << "Couldn't open codec for writing";
        return;
    }
//    qDebug() << "video codec opened" << res << codec->time_base.num << codec->time_base.den;
    stream->time_base = codec->time_base;

    if (cameraFormat != encoderFormat)
        converter = sws_getContext(resolution.width(), resolution.height(), cameraFormat,
                                   resolution.width(), resolution.height(), encoderFormat,
                                   SWS_BICUBIC, nullptr, nullptr, nullptr);
}

VideoEncoder::~VideoEncoder()
{
    sws_freeContext(converter);
    converter = nullptr;
}

void VideoEncoder::addFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&queueMutex);
    videoFrameQueue.enqueue(frame);
    wake();
}
inline qint64 timeStamp(qint64 ts, AVRational base)
{
    return (1000*ts*base.num + 500)/base.den;
}

QVideoFrame VideoEncoder::takeFrame()
{
    QMutexLocker locker(&queueMutex);
    if (videoFrameQueue.isEmpty())
        return QVideoFrame();
    return videoFrameQueue.dequeue();
}

void VideoEncoder::init()
{
    qDebug() << "VideoEncoder::init started video device thread.";
}

void VideoEncoder::cleanup()
{
    while (!videoFrameQueue.isEmpty())
        loop();
    EncoderThread::cleanup();
}

bool VideoEncoder::shouldWait() const
{
    QMutexLocker locker(&queueMutex);
    return videoFrameQueue.isEmpty();
}

void VideoEncoder::loop()
{
    auto frame = takeFrame();
    if (!frame.isValid())
        return;

    if (baseTime < 0)
        baseTime = frame.startTime();

//    qDebug() << "new video buffer" << frame.surfaceFormat() << frame.size();
    retrievePackets();

    frame.map(QVideoFrame::ReadOnly);
    auto size = frame.size();
    AVFrame *avFrame = av_frame_alloc();
    avFrame->format = codec->pix_fmt;
    avFrame->width = size.width();
    avFrame->height = size.height();
    av_frame_get_buffer(avFrame, 0);

    const uint8_t *data[4] = { frame.bits(0), frame.bits(1), frame.bits(2), 0 };
    int strides[4] = { frame.bytesPerLine(0), frame.bytesPerLine(1), frame.bytesPerLine(2), 0 };

    QImage img;
    if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
        // the QImage is cached inside the video frame, so we can take the pointer to the image data here
        img = frame.toImage();
        data[0] = (const uint8_t *)img.bits();
        strides[0] = img.bytesPerLine();
    }

    // #### make sure bytesPerline agree, support planar formats
    Q_ASSERT(avFrame->data[0]);
    if (!converter) {
        memcpy(avFrame->data[0], data[0], strides[0]*frame.height());
    } else {
        sws_scale(converter, data, strides, 0, frame.height(), avFrame->data, avFrame->linesize);
    }

    qint64 time = frame.startTime() - baseTime;
    avFrame->pts = (time*stream->time_base.den + (stream->time_base.num >> 1))/(1000*stream->time_base.num);

//    qDebug() << "sending frame" << avFrame->pts << time << stream->time_base.num << stream->time_base.den;
    int ret = avcodec_send_frame(codec, avFrame);
    if (ret < 0) {
        char errStr[1024];
        av_strerror(ret, errStr, 1024);
        qDebug() << "error sending frame" << ret << errStr;
    }
}

}

QT_END_NAMESPACE
