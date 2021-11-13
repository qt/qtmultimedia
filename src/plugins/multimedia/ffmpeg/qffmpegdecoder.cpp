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
#include "qvideosink.h"
#include "qaudiosink.h"
#include "qaudiooutput.h"

#include <qlocale.h>
#include <qtimer.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

Q_LOGGING_CATEGORY(qLcDemuxer, "qt.multimedia.ffmpeg.demuxer")
Q_LOGGING_CATEGORY(qLcVideoDecoder, "qt.multimedia.ffmpeg.videoDecoder")
Q_LOGGING_CATEGORY(qLcVideoRenderer, "qt.multimedia.ffmpeg.videoRenderer")
Q_LOGGING_CATEGORY(qLcAudioDecoder, "qt.multimedia.ffmpeg.audioDecoder")
Q_LOGGING_CATEGORY(qLcAudioRenderer, "qt.multimedia.ffmpeg.audioRenderer")

bool Thread::checkPaused()
{
    if (!pauseRequested)
        return false;
    if (paused.loadAcquire() != pauseRequested) {
        qDebug() << "YYY" << this << "pause requested" << pauseRequested;
        paused.storeRelease(pauseRequested);
        data->condition.wakeAll();
    }
    paused.storeRelease(true);
    if (step) {
        step = false;
        return false;
    }
    return true;
}

void Thread::maybePause()
{
    QMutexLocker locker(&mutex);
    while (checkPaused() || shouldWait()) {
        qDebug() << "YYY" << this << "   waiting" << pauseRequested << step;
        condition.wait(&mutex);
        qDebug() << "YYY" << this << "   done waiting";
    }
}

void Thread::run()
{
    init();
    while (!exit.loadRelaxed()) {
        maybePause();
        loop();
    }
    cleanup();
}


void DemuxerThread::init()
{
    qCDebug(qLcDemuxer) << "Demuxer started";
    av_init_packet(&packet);
}

void DemuxerThread::cleanup()
{
    av_packet_unref(&packet);
    Thread::cleanup();
}

bool DemuxerThread::shouldWait()
{
    if (data->seek_pos.loadRelaxed() >= 0)
        return false;
    if (data->pipelines[0].decoder->queuedPacketSize + data->pipelines[1].decoder->queuedPacketSize > MaxQueueSize)
        return true;
    for (int i = 0; i < QPlatformMediaPlayer::SubtitleStream; ++i) {
        auto *stream = data->stream(QPlatformMediaPlayer::TrackType(i));
        if (stream && timeStamp(data->pipelines[i].decoder->queuedDuration, stream->time_base) < 2000)
            return false;
    }
    return true;

}

void DemuxerThread::loop()
{
    int seek = data->seek_pos.loadRelaxed();
    if (seek >= 0) {
        seekPos = seek;
        data->seek_pos.storeRelaxed(-1);
    }

    if (seekPos >= 0)
        doSeek(seekPos, seekOffset);

    if (av_read_frame(data->context, &packet) < 0) {
        eos = true;
        return;
    }
    if (seekPos >= 0 && packet.pts != AV_NOPTS_VALUE) {
        auto *stream = data->context->streams[packet.stream_index];
        qint64 pts = timeStamp(packet.pts, stream->time_base);
        //                qDebug() << ">>> after seek, got pts:" << pts;
        if (pts > seekPos && seekPos + seekOffset >= 0) {
            seekOffset -= 50;
            //                    qDebug() << "    retrying seek:" << seekPos << seekOffset;
            return;
        }
        // Found a decent pos
        seekPos = -1;
        seekOffset = 0;
        eos = false;
        data->triggerStep();
    }
    queuePacket();
}

void DemuxerThread::doSeek(qint64 pos, qint64 offset)
{
    auto *stream = data->stream(QPlatformMediaPlayer::VideoStream);
    if (!stream)
        stream = data->stream(QPlatformMediaPlayer::AudioStream);
    qint64 seekPos = (pos + offset)*AV_TIME_BASE/1000;
    int streamIndex = -1;
    if (stream && stream->time_base.num != 0) {
        streamIndex = stream->index;
        seekPos = (pos + offset)*stream->time_base.den/(stream->time_base.num*1000);
    }
    qDebug() << ">>>> executing seek" << pos << streamIndex << seekPos;
    av_seek_frame(data->context, streamIndex, seekPos, AVSEEK_FLAG_BACKWARD);
    for (int i = 0; i < QPlatformMediaPlayer::SubtitleStream; ++i)
        data->pipelines[i].decoder->flushAndSeek(pos);
    data->subtitleQueue.clear();
    qDebug() << "all queues flushed";
}

void DemuxerThread::queuePacket()
{
    auto *stream = data->context->streams[packet.stream_index];
    int trackType = -1;
    for (int i = 0; i < QPlatformMediaPlayer::NTrackTypes; ++i)
        if (packet.stream_index == data->m_currentStream[i])
            trackType = i;

    auto *codec = data->codecContext[trackType];
    if (!codec || trackType < 0)
        return;

    //        auto base = stream->time_base;
    //        qCDebug(qLcDemuxer) << "read a packet: track" << trackType << packet.stream_index
    //                            << timeStamp(packet.dts, base) << timeStamp(packet.pts, base) << timeStamp(packet.duration, base);

    if (trackType == QPlatformMediaPlayer::SubtitleStream)
        decodeSubtitle(stream, codec, &packet);
    else
        data->pipelines[trackType].decoder->enqueue(&packet);
}

void DemuxerThread::decodeSubtitle(AVStream *stream, AVCodecContext *codec, AVPacket *packet)
{
    auto base = stream->time_base;
    //                qDebug() << "    decoding subtitle" << "has delay:" << (codec->codec->capabilities & AV_CODEC_CAP_DELAY);
    AVSubtitle subtitle;
    memset(&subtitle, 0, sizeof(subtitle));
    int gotSubtitle = 0;
    int res = avcodec_decode_subtitle2(codec, &subtitle, &gotSubtitle, packet);
    //                qDebug() << "       subtitle got:" << res << gotSubtitle << subtitle.format << Qt::hex << (quint64)subtitle.pts;
    if (res >= 0 && gotSubtitle) {
        // apparently the timestamps in the AVSubtitle structure are not always filled in
        // if they are missing, use the packets pts and duration values instead
        qint64 start, end;
        if (subtitle.pts == AV_NOPTS_VALUE) {
            start = timeStamp(packet->pts, base);
            end = start + timeStamp(packet->duration, base);
        } else {
            qint64 pts = timeStamp(subtitle.pts, AV_TIME_BASE_Q);
            start = pts + subtitle.start_display_time;
            end = pts + subtitle.end_display_time;
        }
        //                    qDebug() << "    got subtitle (" << start << "--" << end << "):";
        QString text;
        for (uint i = 0; i < subtitle.num_rects; ++i) {
            const auto *r = subtitle.rects[i];
            //                        qDebug() << "    subtitletext:" << r->text << "/" << r->ass;
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

        //                    qDebug() << "    >>> subtitle adding" << text << start << end;
        Subtitle *sub = new Subtitle{text, start, end};
        data->subtitleQueue.enqueue(sub);
    }
}


DecoderThread::DecoderThread(QFFmpegDecoder *decoder, QPlatformMediaPlayer::TrackType t)
    : Thread(decoder)
    , type(t)
    , cat(t == QPlatformMediaPlayer::VideoStream ? qLcVideoDecoder() : qLcAudioDecoder())
{}

void DecoderThread::enqueue(AVPacket *packet)
{
    Q_ASSERT(packet);
    AVPacket *p = av_packet_alloc();
    av_packet_move_ref(p, packet);
    {
        QMutexLocker locker(&mutex);
        queue.enqueue(p);
        queuedPacketSize += p->size;
        queuedDuration += p->duration;
    }
    condition.wakeAll();
}

void DecoderThread::flushAndSeek(qint64 seek)
{
    QMutexLocker locker(&mutex);
    // ### fix mem leak here and int the other flush/clear methods
    queue.clear();
    queuedPacketSize = 0;
    queuedDuration = 0;
    seekTo = seek;
}

AVPacket *DecoderThread::dequeue(qint64 *seek)
{
    QMutexLocker locker(&mutex);
    if (queue.isEmpty())
        return nullptr;
    auto *p = queue.dequeue();
    queuedPacketSize -= p->size;
    queuedDuration -= p->duration;
    *seek = seekTo;
    seekTo = -1;
    data->demuxer->wake();
    return p;
}

void DecoderThread::init()
{
    qCDebug(cat) << "Starting decoder";
}

bool DecoderThread::shouldWait()
{
    if (queue.isEmpty())
        return true;
    if (data->pipelines[type].renderer->hasEnoughFrames())
        return true;
    return false;
}

void DecoderThread::loop()
{
    auto *codec = data->codecContext[type];
    Q_ASSERT(codec);
    AVFrame *frame = av_frame_alloc();
    int res = avcodec_receive_frame(codec, frame);
    if (res >= 0) {
        //            qCDebug(cat) << "received frame from AV decoder";
        if (seek >= 0) {
            auto *stream = data->stream(type);
            if (timeStamp(frame->pts, stream->time_base) < seekTo) {
                // too early, discard
                av_frame_free(&frame);
                return;
            }
            seek = -1;
        }
        data->pipelines[type].renderer->enqueue(frame);
    } else if (res == AVERROR(EOF)) {
        eos = true;
        return;
    } else if (res != AVERROR(EAGAIN)) {
        qWarning() << "error in decoder" << res;
        return;
    }

    AVPacket *packet = dequeue(&seek);
    while (!packet)
        return;

    if (seek >= 0) {
        qCDebug(cat) << ">>> flushing codec, seeking to" << seek;
        avcodec_flush_buffers(codec);
        data->pipelines[type].renderer->flushAndSeek(seek);
    }

    // send the frame to the data
    avcodec_send_packet(codec, packet);
    av_packet_unref(packet);
    //        qCDebug(cat) << "packet sent to AV decoder";
}


AVFrame *RendererThread::dequeue(qint64 *seek)
{
    QMutexLocker locker(&mutex);
    *seek = seekTo;
    // wake up the decoder so it delivers more frames
    data->pipelines[type].decoder->wake();
    if (queue.isEmpty())
        return nullptr;
    seekTo = -1;
    return queue.dequeue();
}


VideoRendererThread::VideoRendererThread(QFFmpegDecoder *decoder, QVideoSink *sink)
    : RendererThread(decoder, QPlatformMediaPlayer::VideoStream)
    , sink(sink)
{}

void VideoRendererThread::init()
{
    qCDebug(qLcVideoRenderer) << "starting video renderer";
}

void VideoRendererThread::loop()
{
    auto *stream = data->context->streams[data->m_currentStream[QPlatformMediaPlayer::VideoStream]];
    auto base = stream->time_base;

    qCDebug(qLcVideoRenderer) << "waiting for video frame" << sink;
    waitForFrame();
    qint64 seekTime = -1;
    AVFrame *avFrame = dequeue(&seekTime);
    if (seekTime >= 0) {
        qCDebug(qLcVideoRenderer) << "video frame queue flushed, resetting pts_base to" << seekTime;
        pts_base = seekTime;
    }
    if (!avFrame)
        return;
    qCDebug(qLcVideoRenderer) << "received video frame";

    qint64 startTime = timeStamp(avFrame->pts, base);
    qint64 duration = (1000*stream->avg_frame_rate.den + (stream->avg_frame_rate.num>>1))
                      /stream->avg_frame_rate.num;
    if (sink) {
        QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(avFrame);
        QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
        QVideoFrame frame(buffer, format);
        qint64 startTime = timeStamp(avFrame->pts, base);
        frame.setStartTime(startTime);
        frame.setEndTime(startTime + duration);
        qCDebug(qLcVideoRenderer) << ">>>> creating video frame" << startTime << (startTime + duration);

        // add in subtitles
        Subtitle *sub = data->subtitleQueue.peek();
//                qCDebug(qLcVideoRenderer) << "frame: subtitle" << sub;
        if (sub) {
            qCDebug(qLcVideoRenderer) << "    " << sub->start << sub->end << sub->text;
            if (sub->start <= startTime && sub->end > startTime) {
//                        qCDebug(qLcVideoRenderer) << "        setting text";
                sink->setSubtitleText(sub->text);
            }
            if (sub->end < startTime) {
//                        qCDebug(qLcVideoRenderer) << "        removing subtitle item";
                delete data->subtitleQueue.dequeue();
                sink->setSubtitleText({});
            }
        }

//                qCDebug(qLcVideoRenderer) << "    sending a video frame" << startTime << duration;
        sink->setVideoFrame(frame);
    }
    AVFrame *nextFrame = peek(&seekTime);
    if (seekTime >= 0) {
        // a seek happened inbetween, update pts_base
        qCDebug(qLcVideoRenderer) << "videorenderer: setting new time base" << seekTime;
        pts_base = seekTime;
    }
    qint64 nextFrameTime = 0;
    if (nextFrame)
        nextFrameTime = timeStamp(nextFrame->pts, base);
    else
        nextFrameTime = seekTime >= 0 ? seekTime : startTime + duration;
    int msToWait = nextFrameTime - (data->baseTimer.elapsed() + pts_base);
    qCDebug(qLcVideoRenderer) << "next video frame in" << msToWait;
    data->currentTime = startTime;
    data->player->positionChanged(startTime);
    if (msToWait > 0)
        msleep(msToWait);
}

AudioRendererThread::AudioRendererThread(QFFmpegDecoder *decoder, QAudioOutput *output)
    : RendererThread(decoder, QPlatformMediaPlayer::AudioStream)
    , output(output)
{
    connect(output, &QAudioOutput::deviceChanged, this, &AudioRendererThread::outputDeviceChanged);
}

void AudioRendererThread::updateOutput()
{
    freeOutput();
    qDebug() << "updateOutput";

    AVStream *audioStream = data->stream(QPlatformMediaPlayer::AudioStream);

    QAudioFormat format;
    format.setSampleFormat(QFFmpegMediaDevices::sampleFormat(AVSampleFormat(audioStream->codecpar->format)));
    format.setSampleRate(audioStream->codecpar->sample_rate);
    format.setChannelCount(2); // #### FIXME
    // ### add channel layout
    qCDebug(qLcAudioRenderer) << "creating new audio sink with format" << format;
    audioSink = new QAudioSink(output->device(), format);
    audioDevice = audioSink->start();
    qCDebug(qLcAudioRenderer) << "   -> have an audio sink" << audioDevice;

    // init resampling if needed
    AVSampleFormat requiredFormat = QFFmpegMediaDevices::avSampleFormat(format.sampleFormat());
    if (requiredFormat == audioStream->codecpar->format &&
        audioStream->codecpar->channels == 2)
        return;
    qDebug() << "init resampler" << requiredFormat << audioStream->codecpar->channels;
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
void AudioRendererThread::freeOutput()
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

void AudioRendererThread::init()
{
    updateOutput();
    qCDebug(qLcAudioRenderer) << "Starting audio renderer";
}

void AudioRendererThread::cleanup()
{
    freeOutput();
}

void AudioRendererThread::loop()
{
    {
        QMutexLocker locker(&mutex);
        if (deviceChanged)
            updateOutput();
        deviceChanged = false;
    }

    auto *stream = data->context->streams[data->m_currentStream[QPlatformMediaPlayer::AudioStream]];
    auto base = stream->time_base;

    waitForFrame();
    qint64 seekTime = 0;
    AVFrame *avFrame = dequeue(&seekTime);
    if (seekTime >= 0) {
        qCDebug(qLcAudioRenderer) << "audio frame queue flushed, resetting pts_base to" << seekTime;
        pts_base = seekTime;
    }
    if (!avFrame)
        return;

    if (!audioSink) {
        qCDebug(qLcAudioRenderer) << "no sink, skipping frame";
        av_frame_free(&avFrame);
        return;
    }

    QAudioFormat format = audioSink->format();
    qint64 startTime = timeStamp(avFrame->pts, base);
    qint64 duration = format.durationForBytes(avFrame->linesize[0]);
    qCDebug(qLcAudioRenderer) << "sending" << avFrame->linesize[0] << "bytes to audio sink, startTime/duration=" << startTime << duration;
    if (!isPauseRequested()) {
        if (!resampler) {
            audioDevice->write((char *)avFrame->data[0], avFrame->linesize[0]);
        } else {
             uint8_t *output;
             av_samples_alloc(&output, nullptr, 2, avFrame->nb_samples,
                              QFFmpegMediaDevices::avSampleFormat(format.sampleFormat()), 0);
             const uint8_t **in = (const uint8_t **)avFrame->extended_data;
             int out_samples = swr_convert(resampler, &output, avFrame->nb_samples,
                                           in, avFrame->nb_samples);
             int size = av_samples_get_buffer_size(nullptr, 2, out_samples,
                                                   QFFmpegMediaDevices::avSampleFormat(format.sampleFormat()), 0);
             audioDevice->write((char *)output, size);
             av_freep(&output);
        }
    }
    av_frame_free(&avFrame);

    AVFrame *nextFrame = peek(&seekTime);
    if (seekTime >= 0) {
        qCDebug(qLcAudioRenderer) << "audiorenderer: setting new time base" << seekTime;
        pts_base = seekTime;
    }
    qint64 nextFrameTime = seekTime >= 0 ? seekTime : startTime + duration/1000;
    if (nextFrame)
        nextFrameTime = qMin(timeStamp(nextFrame->pts, base), nextFrameTime);
    // always write 40ms ahead
    int msToWait = nextFrameTime - 40 - (data->baseTimer.elapsed() + pts_base);
    qCDebug(qLcAudioRenderer) << "next audio frame at" << nextFrameTime << "sleeping" << msToWait << "ms. pts_base=" << pts_base;
    if (msToWait > 0)
        msleep(msToWait);
}

void AudioRendererThread::outputDeviceChanged()
{
    QMutexLocker locker(&mutex);
    deviceChanged = true;
}

void Pipeline::createVideoPipeline(QFFmpegDecoder *d, QVideoSink *sink)
{
    Q_ASSERT(!decoder);
    Q_ASSERT(!renderer);
    decoder = new DecoderThread(d, QPlatformMediaPlayer::VideoStream);
    renderer = new VideoRendererThread(d, sink);
    decoder->start();
    renderer->start();
}

void Pipeline::createAudioPipeline(QFFmpegDecoder *d, QAudioOutput *sink)
{
    decoder = new DecoderThread(d, QPlatformMediaPlayer::AudioStream);
    renderer = new AudioRendererThread(d, sink);
    decoder->start();
    renderer->start();
}

void Pipeline::destroy()
{
    Q_ASSERT(decoder);
    Q_ASSERT(renderer);
    decoder->kill();
    renderer->kill();
    *this = {};
}


QFFmpegDecoder::QFFmpegDecoder(QFFmpegMediaPlayer *p)
    : player(p)
{}

void QFFmpegDecoder::init()
{
    if (demuxer)
        return;
    updateAudio();
    updateVideo();
    syncClocks();
    startDemuxer();
}

void QFFmpegDecoder::setPaused(bool b)
{
    if (paused == b)
        return;
    paused = b;
    qDebug() << "XXXXXXX setPaused" << b;
    if (b) {
        for (int i = 0; i < NPipelines; ++i) {
            if (pipelines[i].renderer)
                pipelines[i].renderer->requestPause();
        }
        auto isPaused = [&]() ->bool {
            for (int i = 0; i < NPipelines; ++i) {
                if (pipelines[i].renderer && !pipelines[i].renderer->isPaused())
                    return false;
            }
            return true;
        };

        mutex.lock();
        while (!isPaused())
            condition.wait(&mutex);
        qDebug() << "XXXXXX" << "both renderers paused";
        mutex.unlock();
    } else {
        syncClocks();
        for (int i = 0; i < NPipelines; ++i) {
            if (pipelines[i].renderer)
                pipelines[i].renderer->requestUnPause();
        }
    }
}

void QFFmpegDecoder::triggerStep()
{
    for (int i = 0; i < NPipelines; ++i) {
        if (pipelines[i].renderer)
            pipelines[i].renderer->requestSingleStep();
    }
}

void QFFmpegDecoder::syncClocks()
{
    qDebug() << "======================== syncing clocks to" << currentTime;
    for (int i = 0; i < NPipelines; ++i) {
        if (pipelines[i].renderer)
            pipelines[i].renderer->syncClock(currentTime);
    }
    baseTimer.start();
}

void QFFmpegDecoder::pausePipeline(bool b)
{
    qDebug() << "XXXXXX" << "pausePipeline" << b;
    if (b) {
        if (!paused) {
            for (int i = 0; i < NPipelines; ++i) {
                if (pipelines[i].renderer)
                    pipelines[i].renderer->requestPause();
            }
        }
        for (int i = 0; i < NPipelines; ++i) {
            if (pipelines[i].decoder)
                pipelines[i].decoder->requestPause();
        }
        if (demuxer)
            demuxer->requestPause();
        mutex.lock();
        auto isPaused = [&]() ->bool {
            if (demuxer && !demuxer->isPaused())
                return false;
            for (int i = 0; i < NPipelines; ++i) {
                if (pipelines[i].decoder && !pipelines[i].decoder->isPaused())
                    return false;
                if (pipelines[i].renderer && !pipelines[i].renderer->isPaused())
                    return false;
            }
            return true;
        };
        while (!isPaused())
            condition.wait(&mutex);
        for (int i = 0; i < QPlatformMediaPlayer::SubtitleStream; ++i) {
            pipelines[i].decoder->flushAndSeek(currentTime);
            pipelines[i].renderer->flushAndSeek(currentTime);
        }
        subtitleQueue.clear();
        qDebug() << "XXXXXX" << "pipeline stopped";
        mutex.unlock();
    } else {
        syncClocks();
        if (!paused) {
            for (int i = 0; i < NPipelines; ++i) {
                if (pipelines[i].renderer)
                    pipelines[i].renderer->requestUnPause();
            }
        }
        for (int i = 0; i < NPipelines; ++i) {
            if (pipelines[i].decoder)
                pipelines[i].decoder->requestUnPause();
        }
        if (demuxer)
            demuxer->requestUnPause();
    }

}

void QFFmpegDecoder::setVideoSink(QVideoSink *sink)
{
    qDebug() << "setVideoSink" << sink;
    if (sink == videoSink)
        return;
    videoSink = sink;
    updateVideo();
}

void QFFmpegDecoder::updateVideo()
{
    auto &p = pipelines[QPlatformMediaPlayer::VideoStream];
    qDebug() << "updateVideo" << videoSink;
    if (!videoSink) {
        closeCodec(QPlatformMediaPlayer::VideoStream);
        p.destroy();
        return;
    }
    if (!openCodec(QPlatformMediaPlayer::VideoStream, player->m_requestedStreams[QPlatformMediaPlayer::VideoStream]))
        return;
    openCodec(QPlatformMediaPlayer::SubtitleStream, player->m_requestedStreams[QPlatformMediaPlayer::SubtitleStream]);

    if (!p.decoder)
        p.createVideoPipeline(this, videoSink);
}

void QFFmpegDecoder::setAudioSink(QPlatformAudioOutput *output)
{
    if (audioOutput == output)
        return;

    audioOutput = output;
    updateAudio();
}

void QFFmpegDecoder::updateAudio()
{
    qDebug() << "setAudioSink" << audioOutput;

    auto &p = pipelines[QPlatformMediaPlayer::AudioStream];
    if (!audioOutput) {
        closeCodec(QPlatformMediaPlayer::AudioStream);
        p.destroy();
        return;
    }

    if (!openCodec(QPlatformMediaPlayer::AudioStream, player->m_requestedStreams[QPlatformMediaPlayer::AudioStream]))
        return;

    qDebug() << "    creating new audio renderer";
    if (!p.decoder)
        p.createAudioPipeline(this, audioOutput->q);
}

void QFFmpegDecoder::changeTrack(QPlatformMediaPlayer::TrackType type, int index)
{
    int streamIndex = player->m_streamMap[type].at(index).avStreamIndex;
    if (m_currentStream[type] == streamIndex)
        return;
    pausePipeline(true);
    qDebug() << ">>>>> change track" << type << index << streamIndex;
    openCodec(type, index);
    pausePipeline(false);
}


void QFFmpegDecoder::startDemuxer()
{
    if (!demuxer) {
        demuxer = new DemuxerThread(this);
        demuxer->start();
    }
}

bool QFFmpegDecoder::openCodec(QPlatformMediaPlayer::TrackType type, int index)
{
    bool wasPlaying = !paused && demuxer;
    if (wasPlaying)
        pause();
    qDebug() << "openCodec" << type << index;
    int streamIdx = player->m_streamMap[type].value(index).avStreamIndex;
    if (m_currentStream[type] == streamIdx)
        return streamIdx >= 0;

    qDebug() << "    using stream index" << streamIdx;
    if (codecContext[type])
        closeCodec(type);
    if (streamIdx < 0)
        return false;

    auto *stream = context->streams[streamIdx];
    AVCodec *decoder = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!decoder) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return false;
    }

    codecContext[type] = avcodec_alloc_context3(decoder);
    if (!codecContext[type]) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Failed to allocate a FFmpeg codec context"));
        return false;
    }
    int ret = avcodec_parameters_to_context(codecContext[type], stream->codecpar);
    if (ret < 0) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return false;
    }

    /* Init the decoders, with reference counting */
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    av_dict_set(&opts, "threads", "auto", 0);
    ret = avcodec_open2(codecContext[type], decoder, &opts);
    if (ret < 0) {
        player->error(QMediaPlayer::FormatError, QMediaPlayer::tr("Can't decode track."));
        return false;
    }
    m_currentStream[type] = streamIdx;

    if (demuxer)
        seek(currentTime);
    if (wasPlaying)
        play();
    return true;
}

void QFFmpegDecoder::closeCodec(QPlatformMediaPlayer::TrackType type)
{
    if (codecContext[type])
        avcodec_close(codecContext[type]);
    codecContext[type] = nullptr;
    m_currentStream[type] = -1;
}

int QFFmpegDecoder::getDefaultStream(QPlatformMediaPlayer::TrackType type)
{
    const auto &map = player->m_streamMap[type];
    for (int i = 0; i < map.size(); ++i) {
        auto *s = context->streams[map.at(i).avStreamIndex];
        if (s->disposition & AV_DISPOSITION_DEFAULT)
            return i;
    }
    return 0;
}

void QFFmpegDecoder::seek(qint64 pos)
{
    seek_pos.storeRelaxed(pos);
    qDebug() << ">>>>>> seeking to pos" << pos;
    triggerStep();
}

QT_END_NAMESPACE
