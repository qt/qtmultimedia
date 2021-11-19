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

#include "qffmpegdecoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegmediadevices_p.h"
#include "private/qiso639_2_p.h"
#include "qffmpeg_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegvideobuffer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qffmpeghwaccel_p.h"
#include "qvideosink.h"
#include "qaudiosink.h"
#include "qaudiooutput.h"

#include <qlocale.h>
#include <qtimer.h>

#include <qloggingcategory.h>

extern "C" {
#include <libavutil/hwcontext.h>
}

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

Q_LOGGING_CATEGORY(qLcDemuxer, "qt.multimedia.ffmpeg.demuxer")
Q_LOGGING_CATEGORY(qLcDecoder, "qt.multimedia.ffmpeg.decoder")
Q_LOGGING_CATEGORY(qLcVideoRenderer, "qt.multimedia.ffmpeg.videoRenderer")
Q_LOGGING_CATEGORY(qLcAudioRenderer, "qt.multimedia.ffmpeg.audioRenderer")

Codec::Data::Data(AVCodecContext *context, AVStream *stream, const HWAccel &hwAccel)
    : context(context)
    , stream(stream)
    , hwAccel(hwAccel)
{
}

Codec::Data::~Data()
{
    if (!context)
        return;
    avcodec_close(context);
    avcodec_free_context(&context);
}

Codec::Codec(AVFormatContext *format, int streamIndex, QRhi *rhi)
{
    qCDebug(qLcDecoder) << "Codec::Codec" << streamIndex;
    Q_ASSERT(streamIndex >= 0 && streamIndex < (int)format->nb_streams);

    AVStream *stream = format->streams[streamIndex];
    AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        qCDebug(qLcDecoder) << "Failed to find a valid FFmpeg decoder";
        return;
    }

    QFFmpeg::HWAccel hwAccel;
    if (decoder->type == AVMEDIA_TYPE_VIDEO) {
        hwAccel = QFFmpeg::HWAccel(decoder);
        hwAccel.setRhi(rhi);
    }

    auto *context = avcodec_alloc_context3(decoder);
    if (!context) {
        qCDebug(qLcDecoder) << "Failed to allocate a FFmpeg codec context";
        return;
    }

    int ret = avcodec_parameters_to_context(context, stream->codecpar);
    if (ret < 0) {
        qCDebug(qLcDecoder) << "Failed to set FFmpeg codec parameters";
        return;
    }

    context->hw_device_ctx = hwAccel.hwContext();
    // ### This still gives errors about wrong HW formats (as we accept all of them)
    // But it would be good to get so we can filter out pixel format we don't support natively
    //    context->get_format = QFFmpeg::getFormat;

    /* Init the decoder, with reference counting and threading */
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    av_dict_set(&opts, "threads", "auto", 0);
    ret = avcodec_open2(context, decoder, &opts);
    if (ret < 0) {
        qCDebug(qLcDecoder) << "Failed to open FFmpeg codec context.";
        avcodec_free_context(&context);
        return;
    }

    d = new Data(context, stream, hwAccel);
}

void Thread::kill()
{
    exit.storeRelaxed(true);
    wake();
    deleteLater();
}

void Thread::maybePause()
{
    while (timeOut >= 0 || shouldWait()) {
        QElapsedTimer timer;
        timer.start();
//        qDebug() << this << "maybePause, waiting" << timeOut;
        if (condition.wait(&mutex, QDeadlineTimer(timeOut, Qt::PreciseTimer))) {
            if (timeOut >= 0)
                timeOut -= timer.elapsed();
            if (timeOut < 0)
                timeOut = -1;
        } else {
            timeOut = -1;
        }
//        qDebug() << this << "    done waiting" << timeOut;
    }
}

void Thread::run()
{
    init();
    QMutexLocker locker(&mutex);
    while (!exit.loadRelaxed()) {
        maybePause();
        if (exit.loadAcquire())
            break;
        loop();
    }
    cleanup();
}


Demuxer::Demuxer(Decoder *decoder)
    : Thread()
    , decoder(decoder)
{
    QString objectName = QLatin1String("Demuxer");
    setObjectName(objectName);

    streamDecoders.resize(decoder->context->nb_streams);
}

StreamDecoder *Demuxer::addStream(int streamIndex, QRhi *rhi)
{
    if (streamIndex < 0)
        return nullptr;
    Codec codec(decoder->context, streamIndex, rhi);
    Q_ASSERT(codec.context()->codec_type == AVMEDIA_TYPE_AUDIO ||
             codec.context()->codec_type == AVMEDIA_TYPE_VIDEO ||
             codec.context()->codec_type == AVMEDIA_TYPE_SUBTITLE);
    auto *stream = new StreamDecoder(decoder, codec);
    QMutexLocker locker(&mutex);
    Q_ASSERT(!streamDecoders.at(streamIndex));
    streamDecoders[streamIndex] = stream;
    stream->start();
    updateEnabledStreams();
    return stream;
}

void Demuxer::removeStream(int streamIndex)
{
    Q_ASSERT(streamIndex >= 0 && streamIndex < (int)decoder->context->nb_streams);
    Q_ASSERT(streamDecoders.at(streamIndex) != nullptr);
    streamDecoders[streamIndex]->kill();
    streamDecoders[streamIndex] = nullptr;
    updateEnabledStreams();
}

void Demuxer::updateEnabledStreams()
{
    for (uint i = 0; i < decoder->context->nb_streams; ++i) {
        AVDiscard discard = AVDISCARD_DEFAULT;
        if (!streamDecoders.at(i))
            discard = AVDISCARD_ALL;
        decoder->context->streams[i]->discard = discard;
    }
}

void Demuxer::init()
{
    qCDebug(qLcDemuxer) << "Demuxer started";
}

void Demuxer::cleanup()
{
    Thread::cleanup();
}

bool Demuxer::shouldWait() const
{
//    qCDebug(qLcDemuxer) << "XXXX Demuxer::shouldWait" << this << data->seek_pos.loadRelaxed();
    if (decoder->seek_pos.loadRelaxed() >= 0)
        return false;
    // require a minimum of 200ms of data
    qint64 queueSize = 0;
    for (auto *d : streamDecoders) {
        if (!d)
            continue;
        if (d->queuedDuration() < 200)
            return false;
        queueSize += d->queuedPacketSize();
    }
//    qCDebug(qLcDemuxer) << "    queue size" << queueSize << MaxQueueSize;
    if (queueSize > MaxQueueSize)
        return true;
//    qCDebug(qLcDemuxer) << "    waiting!";
    return true;

}

void Demuxer::loop()
{
    seekPos = decoder->seek_pos.loadRelaxed();
    if (seekPos >= 0)
        doSeek(seekPos, seekOffset);

    AVPacket *packet = av_packet_alloc();
    if (av_read_frame(decoder->context, packet) < 0) {
        eos = true;
        return;
    }
    if (seekPos >= 0 && packet->pts != AV_NOPTS_VALUE) {
        auto *stream = decoder->context->streams[packet->stream_index];
        qint64 pts = timeStamp(packet->pts, stream->time_base);
        qCDebug(qLcDemuxer) << ">>> Demuxer: after seek, got pts:" << pts;
        if (pts > seekPos && seekPos + seekOffset >= 0) {
            seekOffset -= 50;
            qCDebug(qLcDemuxer) << "    retrying seek:" << seekPos << seekOffset;
            return;
        }
        // Found a decent pos
        seekPos = -1;
        seekOffset = 0;
        eos = false;
        decoder->seek_pos.storeRelaxed(-1);
        decoder->condition.wakeAll();
        decoder->triggerStep();
    }

    auto *decoder = streamDecoders.at(packet->stream_index);
    if (!decoder) {
        av_packet_free(&packet);
        return;
    }
    decoder->addPacket(packet, serial);
}

void Demuxer::doSeek(qint64 pos, qint64 offset)
{
    qint64 seekPos = (pos + offset)*AV_TIME_BASE/1000;
    qCDebug(qLcDemuxer) << "Demuxer: executing seek" << pos << offset;
    av_seek_frame(decoder->context, -1, seekPos, AVSEEK_FLAG_BACKWARD);
    for (StreamDecoder *d : qAsConst(streamDecoders)) {
        if (d)
            d->flush();
    }
    decoder->currentTime = pos;
    ++serial;
    qCDebug(qLcDemuxer) << "all queues flushed";
}

StreamDecoder::StreamDecoder(Decoder *decoder, const Codec &codec)
    : Thread()
    , decoder(decoder)
    , codec(codec)
{
    Q_ASSERT(codec.context()->codec_type == AVMEDIA_TYPE_AUDIO ||
             codec.context()->codec_type == AVMEDIA_TYPE_VIDEO ||
             codec.context()->codec_type == AVMEDIA_TYPE_SUBTITLE);

    QString objectName;
    switch (codec.context()->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        objectName = QLatin1String("AudioDecoderThread");
        // Queue size: 3 frames for video/subtitle, 9 for audio
        frameQueue.maxSize = 9;
        break;
    case AVMEDIA_TYPE_VIDEO:
        objectName = QLatin1String("VideoDecoderThread");
        break;
    case AVMEDIA_TYPE_SUBTITLE:
        objectName = QLatin1String("SubtitleDecoderThread");
        break;
    default:
        Q_UNREACHABLE();
    }
    setObjectName(objectName);
}

void StreamDecoder::addPacket(AVPacket *packet, int serial)
{
    Q_ASSERT(packet);
    {
        QMutexLocker locker(&packetQueue.mutex);
//        qCDebug(qLcDecoder) << "enqueuing packet of type" << type() << "with serial" << serial << "current serial" << this->serial
//                            << "size" << packet->size
//                            << "pts" << codec.toMs(packet->pts)
//                            << "duration" << codec.toMs(packet->duration);
        packetQueue.queue.enqueue(Packet(packet, serial));
        packetQueue.size += packet->size;
        packetQueue.duration += codec.toMs(packet->duration);
    }
    condition.wakeAll();
}

void StreamDecoder::flush()
{
    {
        QMutexLocker locker(&packetQueue.mutex);
    //    qCDebug(qLcDecoder) << ">>>> flushing packet queue" << type() << serial;
        packetQueue.queue.clear();
        packetQueue.size = 0;
        packetQueue.duration = 0;
    }

    QMutexLocker locker(&frameQueue.mutex);
    frameQueue.queue.clear();
    //        timeOut = -1;
    if (renderer)
        renderer->wake();
}

void StreamDecoder::setRenderer(Renderer *r)
{
    QMutexLocker locker(&mutex);
    renderer = r;
    if (renderer)
        renderer->wake();
}

Packet StreamDecoder::takePacket()
{
    QMutexLocker locker(&packetQueue.mutex);
    if (packetQueue.queue.isEmpty())
        return {};
    auto packet = packetQueue.queue.dequeue();
    packetQueue.size -= packet.avPacket()->size;
    packetQueue.duration -= codec.toMs(packet.avPacket()->duration);
//    qCDebug(qLcDecoder) << "<<<< dequeuing packet of type" << type() << "with serial" << packet.serial() << "current serial" << serial
//             << "pts" << codec.toMs(packet.avPacket()->pts)
//             << "duration" << codec.toMs(packet.avPacket()->duration)
//             << "ts" << decoder->baseTimer.elapsed();
    if (packet.serial() != serial) {
        avcodec_flush_buffers(codec.context());
        serial = packet.serial();
    }
    decoder->demuxer->wake();
    return packet;
}

void StreamDecoder::addFrame(const Frame &f)
{
    Q_ASSERT(f.isValid());
    QMutexLocker locker(&frameQueue.mutex);
    frameQueue.queue.append(std::move(f));
    if (renderer)
        renderer->wake();
}

Frame StreamDecoder::takeFrame()
{
    QMutexLocker locker(&frameQueue.mutex);
    // wake up the decoder so it delivers more frames
    wake();
    if (frameQueue.queue.isEmpty())
        return {};
    return frameQueue.queue.dequeue();
}

void StreamDecoder::init()
{
    qCDebug(qLcDecoder) << "Starting decoder";
}

bool StreamDecoder::shouldWait() const
{
    if (hasNoPackets() || hasEnoughFrames())
        return true;
    return false;
}

void StreamDecoder::loop()
{
    if (codec.context()->codec->type == AVMEDIA_TYPE_SUBTITLE)
        decodeSubtitle();
    else
        decode();
}

void StreamDecoder::decode()
{
    Q_ASSERT(codec.context());
    AVFrame *frame = av_frame_alloc();
    int res = avcodec_receive_frame(codec.context(), frame);
    if (res >= 0) {
        qint64 pts = codec.toMs(frame->pts);
//        qCDebug(qLcDecoder) << "received frame" << type << timeStamp(frame->pts, stream->time_base) << seek;
        if (pts < decoder->pts_base.loadRelaxed()) {
            // too early, discard
//            qCDebug(qLcDecoder) << "    discard";
            av_frame_free(&frame);
            return;
        }
        addFrame(Frame{frame, codec, pts});
    } else if (res == AVERROR(EOF)) {
        eos = true;
        return;
    } else if (res != AVERROR(EAGAIN)) {
        qWarning() << "error in decoder" << res;
        return;
    }

    Packet packet = takePacket();
    if (!packet.isValid())
        return;

    // send the frame to the data
    avcodec_send_packet(codec.context(), packet.avPacket());
    //        qCDebug(cat) << "packet sent to AV decoder";
}

void StreamDecoder::decodeSubtitle()
{
    //    qCDebug(qLcDecoder) << "    decoding subtitle" << "has delay:" << (codec->codec->capabilities & AV_CODEC_CAP_DELAY);
    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    int gotSubtitle = 0;
    Packet packet = takePacket();
    if (!packet.isValid())
        return;

    int res = avcodec_decode_subtitle2(codec.context(), &subtitle, &gotSubtitle, packet.avPacket());
    //    qCDebug(qLcDecoder) << "       subtitle got:" << res << gotSubtitle << subtitle.format << Qt::hex << (quint64)subtitle.pts;
    if (res >= 0 && gotSubtitle) {
        // apparently the timestamps in the AVSubtitle structure are not always filled in
        // if they are missing, use the packets pts and duration values instead
        qint64 start, end;
        if (subtitle.pts == AV_NOPTS_VALUE) {
            start = codec.toMs(packet.avPacket()->pts);
            end = start + codec.toMs(packet.avPacket()->duration);
        } else {
            qint64 pts = timeStamp(subtitle.pts, AVRational{1, AV_TIME_BASE});
            start = pts + subtitle.start_display_time;
            end = pts + subtitle.end_display_time;
        }
        //        qCDebug(qLcDecoder) << "    got subtitle (" << start << "--" << end << "):";
        QString text;
        for (uint i = 0; i < subtitle.num_rects; ++i) {
            const auto *r = subtitle.rects[i];
            //            qCDebug(qLcDecoder) << "    subtitletext:" << r->text << "/" << r->ass;
            if (i)
                text += QLatin1Char('\n');
            if (r->text)
                text += QString::fromUtf8(r->text);
            else {
                const char *ass = r->ass;
                int nCommas = 0;
                while (*ass) {
                    if (nCommas == 9)
                        break;
                    if (*ass == ',')
                        ++nCommas;
                    ++ass;
                }
                text += QString::fromUtf8(ass);
            }
        }
        text.replace(QLatin1String("\\N"), QLatin1String("\n"));
        text.replace(QLatin1String("\\n"), QLatin1String("\n"));
        text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        if (text.endsWith(QLatin1Char('\n')))
            text.chop(1);

        //        qCDebug(qLcDecoder) << "    >>> subtitle adding" << text << start << end;
        Frame sub{text, start, end};
        addFrame(sub);
    }
}

QPlatformMediaPlayer::TrackType StreamDecoder::type() const
{
    switch (codec.stream()->codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        return QPlatformMediaPlayer::AudioStream;
    case AVMEDIA_TYPE_VIDEO:
        return QPlatformMediaPlayer::VideoStream;
    case AVMEDIA_TYPE_SUBTITLE:
        return QPlatformMediaPlayer::SubtitleStream;
    default:
        return QPlatformMediaPlayer::NTrackTypes;
    }
}

Renderer::Renderer(Decoder *decoder, QPlatformMediaPlayer::TrackType type)
    : Thread()
    , decoder(decoder)
    , type(type)
{
    QString objectName;
    if (type == QPlatformMediaPlayer::AudioStream)
        objectName = QLatin1String("AudioRenderThread");
    else
        objectName = QLatin1String("VideoRenderThread");
    setObjectName(objectName);
}

void Renderer::setStream(StreamDecoder *stream)
{
    QMutexLocker locker(&mutex);
    if (streamDecoder == stream)
        return;
    if (streamDecoder)
        streamDecoder->setRenderer(nullptr);
    streamDecoder = stream;
    if (streamDecoder)
        streamDecoder->setRenderer(this);
    wake();
}

void Renderer::kill()
{
    QMutexLocker locker(&mutex);
    if (streamDecoder)
        streamDecoder->setRenderer(nullptr);
    Thread::kill();
}

bool Renderer::shouldWait() const
{
    if (!paused)
        return false;
    if (step)
        return false;
    return true;
}

VideoRenderer::VideoRenderer(Decoder *decoder, QVideoSink *sink)
    : Renderer(decoder, QPlatformMediaPlayer::VideoStream)
    , sink(sink)
{}

void VideoRenderer::kill()
{
    QMutexLocker locker(&mutex);
    if (subtitleStreamDecoder)
        subtitleStreamDecoder->setRenderer(nullptr);
    if (streamDecoder)
        streamDecoder->setRenderer(nullptr);
    Thread::kill();
}

void VideoRenderer::setSubtitleStream(StreamDecoder *stream)
{
    QMutexLocker locker(&mutex);
    if (stream == subtitleStreamDecoder)
        return;
    if (subtitleStreamDecoder)
        subtitleStreamDecoder->setRenderer(nullptr);
    subtitleStreamDecoder = stream;
    currentSubtitle = {};
    if (subtitleStreamDecoder)
        subtitleStreamDecoder->setRenderer(this);
    wake();
}

void VideoRenderer::init()
{
    qCDebug(qLcVideoRenderer) << "starting video renderer";
}

void VideoRenderer::loop()
{
    if (!streamDecoder) {
        timeOut = 10; // ### Fixme, this is to avoid 100% CPU load before play()
        return;
    }

    Frame frame = streamDecoder->takeFrame();
    if (!frame.isValid()) {
        timeOut = 10;
        return;
    }
    qCDebug(qLcVideoRenderer) << "waiting for video frame" << sink;
    qint64 pts_base = decoder->pts_base.loadRelaxed();
    if (frame.avFrame()->pts < pts_base)
        return;

    qCDebug(qLcVideoRenderer) << "received video frame";

    AVStream *stream = frame.codec()->stream();
    qint64 startTime = frame.pts();
    qint64 duration = (1000*stream->avg_frame_rate.den + (stream->avg_frame_rate.num>>1))
                      /stream->avg_frame_rate.num;

    if (sink) {
        qint64 startTime = frame.pts();
        QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(frame.takeAVFrame(), frame.codec()->hwAccel());
        QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
        QVideoFrame videoFrame(buffer, format);
        videoFrame.setStartTime(startTime);
        videoFrame.setEndTime(startTime + duration);
        qCDebug(qLcVideoRenderer) << "Creating video frame" << startTime << (startTime + duration);

        // add in subtitles
        if (!currentSubtitle.isValid() && subtitleStreamDecoder)
            currentSubtitle = subtitleStreamDecoder->takeFrame();

//        qCDebug(qLcVideoRenderer) << "frame: subtitle" << sub;
        if (currentSubtitle.isValid()) {
            qCDebug(qLcVideoRenderer) << "    " << currentSubtitle.pts() << currentSubtitle.duration() << currentSubtitle.text();
            if (currentSubtitle.pts() <= startTime && currentSubtitle.end() > startTime) {
//                qCDebug(qLcVideoRenderer) << "        setting text";
                sink->setSubtitleText(currentSubtitle.text());
            }
            if (currentSubtitle.end() < startTime) {
//                qCDebug(qLcVideoRenderer) << "        removing subtitle item";
                currentSubtitle = {};
                sink->setSubtitleText({});
            }
        }

//        qCDebug(qLcVideoRenderer) << "    sending a video frame" << startTime << duration << decoder->baseTimer.elapsed();
        sink->setVideoFrame(videoFrame);
        doneStep();
    }
    const Frame *nextFrame = streamDecoder->peekFrame();
    qint64 nextFrameTime = 0;
    if (nextFrame)
        nextFrameTime = nextFrame->pts();
    else
        nextFrameTime = startTime + duration;
//    qCDebug(qLcVideoRenderer) << "    calculating next frame time" << nextFrame << nextFrameTime << startTime << duration
//             << pts_base << decoder->baseTimer.elapsed();
    timeOut = nextFrameTime - (decoder->baseTimer.elapsed() + pts_base);
    if (timeOut < 0)
        timeOut = -1;
//    qCDebug(qLcVideoRenderer) << "    next video frame in" << timeOut;
    decoder->currentTime = startTime;
    decoder->player->positionChanged(startTime);
}

AudioRenderer::AudioRenderer(Decoder *decoder, QAudioOutput *output)
    : Renderer(decoder, QPlatformMediaPlayer::AudioStream)
    , output(output)
{
    connect(output, &QAudioOutput::deviceChanged, this, &AudioRenderer::outputDeviceChanged);
}

void AudioRenderer::updateOutput(const Codec *codec)
{
    freeOutput();
    qCDebug(qLcAudioRenderer) << "updateOutput";

    AVStream *audioStream = codec->stream();

    QAudioFormat format;
    format.setSampleFormat(QFFmpegMediaFormatInfo::sampleFormat(AVSampleFormat(audioStream->codecpar->format)));
    format.setSampleRate(audioStream->codecpar->sample_rate);
    format.setChannelCount(2); // #### FIXME
    // ### add channel layout
    qCDebug(qLcAudioRenderer) << "creating new audio sink with format" << format;
    audioSink = new QAudioSink(output->device(), format);
    audioDevice = audioSink->start();
    qCDebug(qLcAudioRenderer) << "   -> have an audio sink" << audioDevice;

    // init resampling if needed
    AVSampleFormat requiredFormat = QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat());
    if (requiredFormat == audioStream->codecpar->format &&
        audioStream->codecpar->channels == 2)
        return;
    qCDebug(qLcAudioRenderer) << "init resampler" << requiredFormat << audioStream->codecpar->channels;
    resampler = swr_alloc_set_opts(nullptr,  // we're allocating a new context
                                   AV_CH_LAYOUT_STEREO,  // out_ch_layout
                                   requiredFormat,    // out_sample_fmt
                                   audioStream->codecpar->sample_rate,                // out_sample_rate
                                   audioStream->codecpar->channel_layout, // in_ch_layout
                                   AVSampleFormat(audioStream->codecpar->format),   // in_sample_fmt
                                   audioStream->codecpar->sample_rate,                // in_sample_rate
                                   0,                    // log_offset
                                   nullptr);
    swr_init(resampler);
}
void AudioRenderer::freeOutput()
{
    if (audioSink) {
        audioSink->reset();
        delete audioSink;
        audioSink = nullptr;
    }
    if (resampler) {
        swr_free(&resampler);
        resampler = nullptr;
    }
}

void AudioRenderer::init()
{
    qCDebug(qLcAudioRenderer) << "Starting audio renderer";
}

void AudioRenderer::cleanup()
{
    freeOutput();
}

void AudioRenderer::loop()
{
    if (!streamDecoder) {
        timeOut = 10; // ### Fixme, this is to avoid 100% CPU load before play()
        return;
    }

    if (deviceChanged)
        freeOutput();
    deviceChanged = false;
    doneStep();

    Frame frame = streamDecoder->takeFrame();
    if (!frame.isValid()) {
        timeOut = 10;
        return;
    }

    if (!audioSink)
        updateOutput(frame.codec());

    qint64 pts_base = decoder->pts_base.loadRelaxed();
    qint64 startTime = frame.pts();
    if (!audioSink || startTime < pts_base) {
//        qCDebug(qLcAudioRenderer) << "    audio frame to early, discarding";
        return;
    }

    QAudioFormat format = audioSink->format();
    qint64 duration = format.durationForBytes(frame.avFrame()->linesize[0]);
    qCDebug(qLcAudioRenderer) << "sending" << frame.avFrame()->linesize[0] << "bytes to audio sink, startTime/duration=" << startTime << duration;
    if (!paused) {
        if (!resampler) {
            audioDevice->write((char *)frame.avFrame()->data[0], frame.avFrame()->linesize[0]);
        } else {
             uint8_t *output;
             av_samples_alloc(&output, nullptr, 2, frame.avFrame()->nb_samples,
                              QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat()), 0);
             const uint8_t **in = (const uint8_t **)frame.avFrame()->extended_data;
             int out_samples = swr_convert(resampler, &output, frame.avFrame()->nb_samples,
                                           in, frame.avFrame()->nb_samples);
             int size = av_samples_get_buffer_size(nullptr, 2, out_samples,
                                                   QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat()), 0);
             audioDevice->write((char *)output, size);
             av_freep(&output);
        }
    }

    const Frame *nextFrame = streamDecoder->peekFrame();
    qint64 nextFrameTime = startTime + duration/1000;
    if (nextFrame)
        nextFrameTime = nextFrame->pts();
    // always write 40ms ahead
    timeOut = nextFrameTime - 80 - (decoder->baseTimer.elapsed() + pts_base);
    if (timeOut < 0)
        timeOut = -1;
//    qCDebug(qLcAudioRenderer) << "next audio frame at" << nextFrameTime << "sleeping" << timeOut << "ms. pts_base=" << pts_base
//             << "base time" << decoder->baseTimer.elapsed();
}

void AudioRenderer::outputDeviceChanged()
{
    QMutexLocker locker(&mutex);
    deviceChanged = true;
}

Decoder::Decoder(QFFmpegMediaPlayer *p)
    : player(p)
{
}

Decoder::~Decoder()
{
    pause();
    if (videoRenderer)
        videoRenderer->kill();
    if (audioRenderer)
        audioRenderer->kill();
    if (demuxer)
        demuxer->kill();
}

void Decoder::init()
{
    if (demuxer)
        return;
    for (int i = 0; i < 3; ++i)
        changeTrack(QPlatformMediaPlayer::TrackType(i), player->m_requestedStreams[i]);

    demuxer = new Demuxer(this);
    updateAudio();
    updateVideo();
    syncClocks();
    startDemuxer();
}

void Decoder::setPaused(bool b)
{
    if (paused == b)
        return;
    paused = b;
//    qCDebug(qLcDecoder) << "setPaused" << b;
    if (b) {
        if (audioRenderer)
            audioRenderer->pause();
        if (videoRenderer)
            videoRenderer->pause();
    } else {
        pts_base = currentTime;
        syncClocks();
        if (audioRenderer)
            audioRenderer->unPause();
        if (videoRenderer)
            videoRenderer->unPause();
    }
}

void Decoder::triggerStep()
{
    audioRenderer->singleStep();
    videoRenderer->singleStep();
}

void Decoder::syncClocks()
{
    qCDebug(qLcDecoder) << "syncing clocks";
    baseTimer.restart();
}

void Decoder::setVideoSink(QVideoSink *sink)
{
    qCDebug(qLcDecoder) << "setVideoSink" << sink;
    if (sink == videoSink)
        return;
    videoSink = sink;
    if (sink && !videoRenderer) {
        videoRenderer = new VideoRenderer(this, sink);
        videoRenderer->start();
    } else if (!videoSink && videoRenderer) {
        videoRenderer->kill();
        videoRenderer = nullptr;
        // ### disable corresponding video stream
    }
    if (demuxer)
        updateVideo();
}

void Decoder::updateVideo()
{
    if (!demuxer || !videoRenderer)
        return;

    StreamDecoder *stream = demuxer->addStream(m_currentStream[QPlatformMediaPlayer::VideoStream], videoSink->rhi());
    videoRenderer->setStream(stream);
    stream = demuxer->addStream(m_currentStream[QPlatformMediaPlayer::SubtitleStream]);
    videoRenderer->setSubtitleStream(stream);
}

void Decoder::setAudioSink(QPlatformAudioOutput *output)
{
    if (audioOutput == output)
        return;

    qCDebug(qLcDecoder) << "setAudioSink" << audioOutput;
    audioOutput = output;
    if (output && !audioRenderer) {
        audioRenderer = new AudioRenderer(this, output->q);
        audioRenderer->start();
    } else if (!output && audioRenderer) {
        audioRenderer->kill();
        audioRenderer = nullptr;
        // ### unregister from decoder and remove it
    }
    updateAudio();
}

void Decoder::updateAudio()
{
    if (!demuxer || !audioRenderer)
        return;

    auto *stream = demuxer->addStream(m_currentStream[QPlatformMediaPlayer::AudioStream]);
    audioRenderer->setStream(stream);
}

void Decoder::changeTrack(QPlatformMediaPlayer::TrackType type, int index)
{
    int streamIndex = player->m_streamMap[type].value(index).avStreamIndex;
    if (m_currentStream[type] == streamIndex)
        return;
    m_currentStream[type] = streamIndex;
    qDebug() << ">>>>> change track" << type << index << streamIndex;
    if (!demuxer)
        return;
    if (type == QPlatformMediaPlayer::AudioStream)
        updateAudio();
    else
        updateVideo();
}


void Decoder::startDemuxer()
{
    demuxer->start();
}

int Decoder::getDefaultStream(QPlatformMediaPlayer::TrackType type)
{
    const auto &map = player->m_streamMap[type];
    for (int i = 0; i < map.size(); ++i) {
        auto *s = context->streams[map.at(i).avStreamIndex];
        if (s->disposition & AV_DISPOSITION_DEFAULT)
            return i;
    }
    return 0;
}

void Decoder::seek(qint64 pos)
{
    QMutexLocker locker(&mutex);
    seek_pos.storeRelaxed(pos);
    qCDebug(qLcDecoder) << ">>>>>> seeking to pos" << pos;
    pts_base = pos;
    syncClocks();
    demuxer->wake();
    while (seek_pos.loadRelaxed() >= 0)
        condition.wait(&mutex);
    triggerStep();
}

QT_END_NAMESPACE
