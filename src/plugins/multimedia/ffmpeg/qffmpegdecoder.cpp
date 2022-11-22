// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegdecoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpeg_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegvideobuffer_p.h"
#include "private/qplatformaudiooutput_p.h"
#include "qffmpeghwaccel_p.h"
#include "qffmpegvideosink_p.h"
#include "qvideosink.h"
#include "qaudiosink.h"
#include "qaudiooutput.h"
#include "qffmpegaudiodecoder_p.h"
#include "qffmpegresampler_p.h"

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

Codec::Data::Data(UniqueAVCodecContext &&context, AVStream *stream, std::unique_ptr<QFFmpeg::HWAccel> &&hwAccel)
    : context(std::move(context))
    , stream(stream)
    , hwAccel(std::move(hwAccel))
{
}

Codec::Data::~Data()
{
    avcodec_close(context.get());
}

QMaybe<Codec> Codec::create(AVStream *stream)
{
    if (!stream)
        return { "Invalid stream" };

    const AVCodec *decoder =
            QFFmpeg::HWAccel::hardwareDecoderForCodecId(stream->codecpar->codec_id);
    if (!decoder)
        return { "Failed to find a valid FFmpeg decoder" };

    //avcodec_free_context
    UniqueAVCodecContext context(avcodec_alloc_context3(decoder));
    if (!context)
        return { "Failed to allocate a FFmpeg codec context" };

    if (context->codec_type != AVMEDIA_TYPE_AUDIO &&
        context->codec_type != AVMEDIA_TYPE_VIDEO &&
        context->codec_type != AVMEDIA_TYPE_SUBTITLE) {
        return { "Unknown codec type" };
    }

    int ret = avcodec_parameters_to_context(context.get(), stream->codecpar);
    if (ret < 0)
        return { "Failed to set FFmpeg codec parameters" };

    std::unique_ptr<QFFmpeg::HWAccel> hwAccel;
    if (decoder->type == AVMEDIA_TYPE_VIDEO) {
        hwAccel = QFFmpeg::HWAccel::create(decoder);
        if (hwAccel)
            context->hw_device_ctx = av_buffer_ref(hwAccel->hwDeviceContextAsBuffer());
    }
    // ### This still gives errors about wrong HW formats (as we accept all of them)
    // But it would be good to get so we can filter out pixel format we don't support natively
    context->get_format = QFFmpeg::getFormat;

    /* Init the decoder, with reference counting and threading */
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    av_dict_set(&opts, "threads", "auto", 0);
    ret = avcodec_open2(context.get(), decoder, &opts);
    if (ret < 0)
        return "Failed to open FFmpeg codec context " + err2str(ret);

    return Codec(new Data(std::move(context), stream, std::move(hwAccel)));
}


Demuxer::Demuxer(Decoder *decoder, AVFormatContext *context)
    : Thread()
    , decoder(decoder)
    , context(context)
{
    QString objectName = QLatin1String("Demuxer");
    setObjectName(objectName);

    streamDecoders.resize(context->nb_streams);
}

Demuxer::~Demuxer()
{
    if (context) {
        if (context->pb) {
            avio_context_free(&context->pb);
            context->pb = nullptr;
        }
        avformat_free_context(context);
    }
}

StreamDecoder *Demuxer::addStream(int streamIndex)
{
    if (streamIndex < 0 || streamIndex >= (int)context->nb_streams)
        return nullptr;

    AVStream *avStream = context->streams[streamIndex];
    if (!avStream)
        return nullptr;

    QMutexLocker locker(&mutex);
    auto maybeCodec = Codec::create(avStream);
    if (!maybeCodec) {
        decoder->errorOccured(QMediaPlayer::FormatError, "Cannot open codec; " + maybeCodec.error());
        return nullptr;
    }
    auto *stream = new StreamDecoder(this, maybeCodec.value());
    Q_ASSERT(!streamDecoders.at(streamIndex));
    streamDecoders[streamIndex] = stream;
    stream->start();
    updateEnabledStreams();
    return stream;
}

void Demuxer::removeStream(int streamIndex)
{
    if (streamIndex < 0)
        return;
    QMutexLocker locker(&mutex);
    Q_ASSERT(streamIndex < (int)context->nb_streams);
    Q_ASSERT(streamDecoders.at(streamIndex) != nullptr);
    streamDecoders[streamIndex] = nullptr;
    updateEnabledStreams();
}

void Demuxer::stopDecoding()
{
    qCDebug(qLcDemuxer) << "StopDecoding";
    QMutexLocker locker(&mutex);
    sendFinalPacketToStreams();
}
int Demuxer::seek(qint64 pos)
{
    QMutexLocker locker(&mutex);
    for (StreamDecoder *d : std::as_const(streamDecoders)) {
        if (d)
            d->mutex.lock();
    }
    for (StreamDecoder *d : std::as_const(streamDecoders)) {
        if (d)
            d->flush();
    }
    for (StreamDecoder *d : std::as_const(streamDecoders)) {
        if (d)
            d->mutex.unlock();
    }
    qint64 seekPos = pos*AV_TIME_BASE/1000000; // usecs to AV_TIME_BASE
    av_seek_frame(context, -1, seekPos, AVSEEK_FLAG_BACKWARD);
    last_pts = -1;
    loop();
    qCDebug(qLcDemuxer) << "Demuxer::seek" << pos << last_pts;
    return last_pts;
}

void Demuxer::updateEnabledStreams()
{
    if (isStopped())
        return;
    for (uint i = 0; i < context->nb_streams; ++i) {
        AVDiscard discard = AVDISCARD_DEFAULT;
        if (!streamDecoders.at(i))
            discard = AVDISCARD_ALL;
        context->streams[i]->discard = discard;
    }
}

void Demuxer::sendFinalPacketToStreams()
{
    if (m_isStopped.loadAcquire())
        return;
    for (auto *streamDecoder : std::as_const(streamDecoders)) {
        qCDebug(qLcDemuxer) << "Demuxer: sending last packet to stream" << streamDecoder;
        if (!streamDecoder)
            continue;
        streamDecoder->addPacket(nullptr);
    }
    m_isStopped.storeRelease(true);
}

void Demuxer::init()
{
    qCDebug(qLcDemuxer) << "Demuxer started";
}

void Demuxer::cleanup()
{
    qCDebug(qLcDemuxer) << "Demuxer::cleanup";
#ifndef QT_NO_DEBUG
    for (auto *streamDecoder : std::as_const(streamDecoders)) {
        Q_ASSERT(!streamDecoder);
    }
#endif
    avformat_close_input(&context);
    Thread::cleanup();
}

bool Demuxer::shouldWait() const
{
    if (m_isStopped)
        return true;
//    qCDebug(qLcDemuxer) << "XXXX Demuxer::shouldWait" << this << data->seek_pos.loadRelaxed();
    // require a minimum of 200ms of data
    qint64 queueSize = 0;
    bool buffersFull = true;
    for (auto *d : streamDecoders) {
        if (!d)
            continue;
        if (d->queuedDuration() < 200)
            buffersFull = false;
        queueSize += d->queuedPacketSize();
    }
//    qCDebug(qLcDemuxer) << "    queue size" << queueSize << MaxQueueSize;
    if (queueSize > MaxQueueSize)
        return true;
//    qCDebug(qLcDemuxer) << "    waiting!";
    return buffersFull;

}

void Demuxer::loop()
{
    AVPacket *packet = av_packet_alloc();
    if (av_read_frame(context, packet) < 0) {
        sendFinalPacketToStreams();
        av_packet_free(&packet);
        return;
    }

    if (last_pts < 0 && packet->pts != AV_NOPTS_VALUE) {
        auto *stream = context->streams[packet->stream_index];
        auto pts = timeStampMs(packet->pts, stream->time_base);
        if (pts)
            last_pts = *pts;
    }

    auto *streamDecoder = streamDecoders.at(packet->stream_index);
    if (!streamDecoder) {
        av_packet_free(&packet);
        return;
    }
    streamDecoder->addPacket(packet);
}


StreamDecoder::StreamDecoder(Demuxer *demuxer, const Codec &codec)
    : Thread()
    , demuxer(demuxer)
    , codec(codec)
{
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

void StreamDecoder::addPacket(AVPacket *packet)
{
    {
        QMutexLocker locker(&packetQueue.mutex);
//        qCDebug(qLcDecoder) << "enqueuing packet of type" << type()
//                            << "size" << packet->size
//                            << "stream index" << packet->stream_index
//                            << "pts" << codec.toMs(packet->pts)
//                            << "duration" << codec.toMs(packet->duration);
        packetQueue.queue.enqueue(Packet(packet));
        if (packet) {
            packetQueue.size += packet->size;
            packetQueue.duration += codec.toMs(packet->duration);
        }
        eos.storeRelease(false);
    }
    wake();
}

void StreamDecoder::flush()
{
    qCDebug(qLcDecoder) << ">>>> flushing stream decoder" << type();
    avcodec_flush_buffers(codec.context());
    {
        QMutexLocker locker(&packetQueue.mutex);
        packetQueue.queue.clear();
        packetQueue.size = 0;
        packetQueue.duration = 0;
    }
    {
        QMutexLocker locker(&frameQueue.mutex);
        frameQueue.queue.clear();
    }
    qCDebug(qLcDecoder) << ">>>> done flushing stream decoder" << type();
}

void StreamDecoder::setRenderer(Renderer *r)
{
    QMutexLocker locker(&mutex);
    m_renderer = r;
    if (m_renderer)
        m_renderer->wake();
}

void StreamDecoder::killHelper()
{
    m_renderer = nullptr;
    demuxer->removeStream(codec.streamIndex());
}

Packet StreamDecoder::peekPacket()
{
    QMutexLocker locker(&packetQueue.mutex);
    if (packetQueue.queue.isEmpty()) {
        if (demuxer)
            demuxer->wake();
        return {};
    }
    auto packet = packetQueue.queue.first();

    if (demuxer)
        demuxer->wake();
    return packet;
}

Packet StreamDecoder::takePacket()
{
    QMutexLocker locker(&packetQueue.mutex);
    if (packetQueue.queue.isEmpty()) {
        if (demuxer)
            demuxer->wake();
        return {};
    }
    auto packet = packetQueue.queue.dequeue();
    if (packet.avPacket()) {
        packetQueue.size -= packet.avPacket()->size;
        packetQueue.duration -= codec.toMs(packet.avPacket()->duration);
    }
//    qCDebug(qLcDecoder) << "<<<< dequeuing packet of type" << type()
//                        << "size" << packet.avPacket()->size
//                        << "stream index" << packet.avPacket()->stream_index
//             << "pts" << codec.toMs(packet.avPacket()->pts)
//             << "duration" << codec.toMs(packet.avPacket()->duration)
//                    << "ts" << decoder->clockController.currentTime();
    if (demuxer)
        demuxer->wake();
    return packet;
}

void StreamDecoder::addFrame(const Frame &f)
{
    Q_ASSERT(f.isValid());
    QMutexLocker locker(&frameQueue.mutex);
    frameQueue.queue.append(std::move(f));
    if (m_renderer)
        m_renderer->wake();
}

Frame StreamDecoder::takeFrame()
{
    QMutexLocker locker(&frameQueue.mutex);
    // wake up the decoder so it delivers more frames
    if (frameQueue.queue.isEmpty()) {
        wake();
        return {};
    }
    auto f = frameQueue.queue.dequeue();
    wake();
    return f;
}

void StreamDecoder::init()
{
    qCDebug(qLcDecoder) << "Starting decoder";
}

bool StreamDecoder::shouldWait() const
{
    if (eos.loadAcquire() || (hasNoPackets() && decoderHasNoFrames) || hasEnoughFrames())
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

    auto frame = makeAVFrame();
    //    if (type() == 0)
    //        qCDebug(qLcDecoder) << "receiving frame";
    int res = avcodec_receive_frame(codec.context(), frame.get());
    if (res >= 0) {
        qint64 pts;
        if (frame->pts != AV_NOPTS_VALUE)
            pts = codec.toUs(frame->pts);
        else
            pts = codec.toUs(frame->best_effort_timestamp);
        addFrame(Frame{ std::move(frame), codec, pts });
    } else if (res == AVERROR(EOF) || res == AVERROR_EOF) {
        eos.storeRelease(true);
        timeOut = -1;
        return;
    } else if (res != AVERROR(EAGAIN)) {
        qWarning() << "error in decoder" << res << err2str(res);
        return;
    } else {
        // EAGAIN
        decoderHasNoFrames = true;
    }

    Packet packet = peekPacket();
    if (!packet.isValid()) {
        timeOut = -1;
        return;
    }

    res = avcodec_send_packet(codec.context(), packet.avPacket());
    if (res != AVERROR(EAGAIN)) {
        takePacket();
    }
    decoderHasNoFrames = false;
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
            start = codec.toUs(packet.avPacket()->pts);
            end = start + codec.toUs(packet.avPacket()->duration);
        } else {
            auto pts = timeStampUs(subtitle.pts, AVRational{1, AV_TIME_BASE});
            start = *pts + qint64(subtitle.start_display_time)*1000;
            end = *pts + qint64(subtitle.end_display_time)*1000;
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
        Frame sub{text, start, end - start};
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

Renderer::Renderer(QPlatformMediaPlayer::TrackType type)
    : Thread()
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
        streamDecoder->kill();
    streamDecoder = stream;
    if (streamDecoder)
        streamDecoder->setRenderer(this);
    streamChanged();
    wake();
}

void Renderer::killHelper()
{
    if (streamDecoder)
        streamDecoder->kill();
    streamDecoder = nullptr;
}

bool Renderer::shouldWait() const
{
    if (!streamDecoder)
        return true;
    if (!paused)
        return false;
    if (step)
        return false;
    return true;
}


void ClockedRenderer::setPaused(bool paused)
{
    Clock::setPaused(paused);
    Renderer::setPaused(paused);
}

VideoRenderer::VideoRenderer(Decoder *decoder, QVideoSink *sink)
    : ClockedRenderer(decoder, QPlatformMediaPlayer::VideoStream)
    , sink(sink)
{}

void VideoRenderer::killHelper()
{
    if (subtitleStreamDecoder)
        subtitleStreamDecoder->kill();
    subtitleStreamDecoder = nullptr;
    if (streamDecoder)
        streamDecoder->kill();
    streamDecoder = nullptr;
}

void VideoRenderer::setSubtitleStream(StreamDecoder *stream)
{
    QMutexLocker locker(&mutex);
    qCDebug(qLcVideoRenderer) << "setting subtitle stream to" << stream;
    if (stream == subtitleStreamDecoder)
        return;
    if (subtitleStreamDecoder)
        subtitleStreamDecoder->kill();
    subtitleStreamDecoder = stream;
    if (subtitleStreamDecoder)
        subtitleStreamDecoder->setRenderer(this);
    sink->setSubtitleText({});
    wake();
}

void VideoRenderer::init()
{
    qCDebug(qLcVideoRenderer) << "starting video renderer";
    ClockedRenderer::init();
}

void VideoRenderer::loop()
{
    if (!streamDecoder) {
        timeOut = -1; // Avoid 100% CPU load before play()
        return;
    }

    Frame frame = streamDecoder->takeFrame();
    if (!frame.isValid()) {
        if (streamDecoder->isAtEnd()) {
            timeOut = -1;
            eos.storeRelease(true);
            mutex.unlock();
            emit atEnd();
            mutex.lock();
            return;
        }
        timeOut = 1;
//        qDebug() << "no valid frame" << timer.elapsed();
        return;
    }
    eos.storeRelease(false);
//    qCDebug(qLcVideoRenderer) << "received video frame" << frame.pts();
    if (frame.pts() < seekTime()) {
        qCDebug(qLcVideoRenderer) << "  discarding" << frame.pts() << seekTime();
        return;
    }

    AVStream *stream = frame.codec()->stream();
    qint64 startTime = frame.pts();
    qint64 duration = (1000000*stream->avg_frame_rate.den + (stream->avg_frame_rate.num>>1))
                      /stream->avg_frame_rate.num;

    if (sink) {
        qint64 startTime = frame.pts();
//        qDebug() << "RHI:" << accel.isNull() << accel.rhi() << sink->rhi();

        // in practice this only happens with mediacodec
        if (frame.codec()->hwAccel() && !frame.avFrame()->hw_frames_ctx) {
            HWAccel *hwaccel = frame.codec()->hwAccel();
            AVFrame *avframe = frame.avFrame();
            if (!hwaccel->hwFramesContext())
                hwaccel->createFramesContext(AVPixelFormat(avframe->format),
                                            { avframe->width, avframe->height });

            avframe->hw_frames_ctx = av_buffer_ref(hwaccel->hwFramesContextAsBuffer());
        }

        QFFmpegVideoBuffer *buffer = new QFFmpegVideoBuffer(frame.takeAVFrame());
        QVideoFrameFormat format(buffer->size(), buffer->pixelFormat());
        format.setColorSpace(buffer->colorSpace());
        format.setColorTransfer(buffer->colorTransfer());
        format.setColorRange(buffer->colorRange());
        format.setMaxLuminance(buffer->maxNits());
        QVideoFrame videoFrame(buffer, format);
        videoFrame.setStartTime(startTime);
        videoFrame.setEndTime(startTime + duration);
//        qDebug() << "Creating video frame" << startTime << (startTime + duration) << subtitleStreamDecoder;

        // add in subtitles
        const Frame *currentSubtitle = nullptr;
        if (subtitleStreamDecoder)
            currentSubtitle = subtitleStreamDecoder->lockAndPeekFrame();

        if (currentSubtitle && currentSubtitle->isValid()) {
//            qDebug() << "frame: subtitle" << currentSubtitle->text() << currentSubtitle->pts() << currentSubtitle->duration();
            qCDebug(qLcVideoRenderer) << "    " << currentSubtitle->pts() << currentSubtitle->duration() << currentSubtitle->text();
            if (currentSubtitle->pts() <= startTime && currentSubtitle->end() > startTime) {
//                qCDebug(qLcVideoRenderer) << "        setting text";
                sink->setSubtitleText(currentSubtitle->text());
            }
            if (currentSubtitle->end() < startTime) {
//                qCDebug(qLcVideoRenderer) << "        removing subtitle item";
                sink->setSubtitleText({});
                subtitleStreamDecoder->removePeekedFrame();
            }
        } else {
            sink->setSubtitleText({});
        }
        if (subtitleStreamDecoder)
            subtitleStreamDecoder->unlockAndReleaseFrame();

//        qCDebug(qLcVideoRenderer) << "    sending a video frame" << startTime << duration << decoder->baseTimer.elapsed();
        sink->setVideoFrame(videoFrame);
        doneStep();
    }
    const Frame *nextFrame = streamDecoder->lockAndPeekFrame();
    qint64 nextFrameTime = 0;
    if (nextFrame)
        nextFrameTime = nextFrame->pts();
    else
        nextFrameTime = startTime + duration;
    streamDecoder->unlockAndReleaseFrame();
    qint64 mtime = timeUpdated(startTime);
    timeOut = usecsTo(mtime, nextFrameTime) / 1000;
    //    qCDebug(qLcVideoRenderer) << "    next video frame in" << startTime << nextFrameTime <<
    //    currentTime() << timeOut;
}

AudioRenderer::AudioRenderer(Decoder *decoder, QAudioOutput *output)
    : ClockedRenderer(decoder, QPlatformMediaPlayer::AudioStream)
    , output(output)
{
    connect(output, &QAudioOutput::deviceChanged, this, &AudioRenderer::updateAudio);
    connect(output, &QAudioOutput::volumeChanged, this, &AudioRenderer::setSoundVolume);
}

void AudioRenderer::syncTo(qint64 usecs)
{
    QMutexLocker locker(&mutex);

    Clock::syncTo(usecs);
    audioBaseTime = usecs;
    processedBase = processedUSecs;
}

void AudioRenderer::setPlaybackRate(float rate, qint64 currentTime)
{
    QMutexLocker locker(&mutex);

    audioBaseTime = currentTime;
    processedBase = processedUSecs;
    Clock::setPlaybackRate(rate, currentTime);
    deviceChanged = true;
}

void AudioRenderer::updateOutput(const Codec *codec)
{
    qCDebug(qLcAudioRenderer) << ">>>>>> updateOutput" << currentTime() << seekTime() << processedUSecs << isMaster();
    freeOutput();
    qCDebug(qLcAudioRenderer) << "    " << currentTime() << seekTime() << processedUSecs;

    AVStream *audioStream = codec->stream();

    auto dev = output->device();
    format = QFFmpegMediaFormatInfo::audioFormatFromCodecParameters(audioStream->codecpar);
    format.setChannelConfig(dev.channelConfiguration());

    initResempler(codec);

    audioSink = new QAudioSink(dev, format);
    audioSink->setVolume(output->volume());

    audioSink->setBufferSize(format.bytesForDuration(100000));
    audioDevice = audioSink->start();

    latencyUSecs = format.durationForBytes(audioSink->bufferSize()); // ### ideally get full latency
    qCDebug(qLcAudioRenderer) << "   -> have an audio sink" << audioDevice;
}

void AudioRenderer::initResempler(const Codec *codec)
{
    // init resampler. It's ok to always do this, as the resampler will be a no-op if
    // formats agree.
    AVSampleFormat requiredFormat = QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat());

#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    qCDebug(qLcAudioRenderer) << "init resampler" << requiredFormat
                              << codec->stream()->codecpar->channels;
#else
    qCDebug(qLcAudioRenderer) << "init resampler" << requiredFormat
                              << codec->stream()->codecpar->ch_layout.nb_channels;
#endif

    auto resamplerFormat = format;
    resamplerFormat.setSampleRate(qRound(format.sampleRate() / playbackRate()));
    resampler.reset(new Resampler(codec, resamplerFormat));
}

void AudioRenderer::freeOutput()
{
    if (audioSink) {
        audioSink->reset();
        delete audioSink;
        audioSink = nullptr;
        audioDevice = nullptr;
    }

    bufferedData = {};
    bufferWritten = 0;

    audioBaseTime = currentTime();
    processedBase = 0;
    processedUSecs = writtenUSecs = 0;
}

void AudioRenderer::init()
{
    qCDebug(qLcAudioRenderer) << "Starting audio renderer";
    ClockedRenderer::init();
}

void AudioRenderer::cleanup()
{
    freeOutput();
}

void AudioRenderer::loop()
{
    if (!streamDecoder) {
        timeOut = -1; // Avoid 100% CPU load before play()
        return;
    }

    if (deviceChanged)
        freeOutput();
    deviceChanged = false;
    doneStep();

    qint64 bytesWritten = 0;
    if (bufferedData.isValid()) {
        bytesWritten = audioDevice->write(bufferedData.constData<char>() + bufferWritten, bufferedData.byteCount() - bufferWritten);
        bufferWritten += bytesWritten;
        if (bufferWritten == bufferedData.byteCount()) {
            bufferedData = {};
            bufferWritten = 0;
        }
        processedUSecs = audioSink->processedUSecs();
    } else {
        Frame frame = streamDecoder->takeFrame();
        if (!frame.isValid()) {
            if (streamDecoder->isAtEnd()) {
                if (audioSink)
                    processedUSecs = audioSink->processedUSecs();
                timeOut = -1;
                eos.storeRelease(true);
                mutex.unlock();
                emit atEnd();
                mutex.lock();
                return;
            }
            timeOut = 1;
            return;
        }
        eos.storeRelease(false);
        if (!audioSink)
            updateOutput(frame.codec());

        qint64 startTime = frame.pts();
        if (startTime < seekTime())
            return;

        if (!paused) {
            auto buffer = resampler->resample(frame.avFrame());

            if (output->isMuted())
                // This is somewhat inefficient, but it'll work
                memset(buffer.data<char>(), 0, buffer.byteCount());

            bytesWritten = audioDevice->write(buffer.constData<char>(), buffer.byteCount());
            if (bytesWritten < buffer.byteCount()) {
                bufferedData = buffer;
                bufferWritten = bytesWritten;
            }

            processedUSecs = audioSink->processedUSecs();
        }
    }

    qint64 duration = format.durationForBytes(bytesWritten);
    writtenUSecs += duration;

    timeOut = (writtenUSecs - processedUSecs - latencyUSecs)/1000;
    if (timeOut < 0)
        // Don't use a zero timeout if the sink didn't want any more data, rather wait for 10ms.
        timeOut = bytesWritten > 0 ? 0 : 10;

//    if (!bufferedData.isEmpty())
//        qDebug() << ">>>>>>>>>>>>>>>>>>>>>>>> could not write all data" << (bufferedData.size() - bufferWritten);
//    qDebug() << "Audio: processed" << processedUSecs << "written" << writtenUSecs
//             << "delta" << (writtenUSecs - processedUSecs) << "timeOut" << timeOut;
//    qCDebug(qLcAudioRenderer) << "    updating time to" << currentTimeNoLock();
    timeUpdated(audioBaseTime + qRound((processedUSecs - processedBase) * playbackRate()));
}

void AudioRenderer::streamChanged()
{
    // mutex is already locked
    deviceChanged = true;
}

void AudioRenderer::updateAudio()
{
    QMutexLocker locker(&mutex);
    deviceChanged = true;
}

void AudioRenderer::setSoundVolume(float volume)
{
    QMutexLocker locker(&mutex);
    if (audioSink)
        audioSink->setVolume(volume);
}

Decoder::Decoder()
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

static int read(void *opaque, uint8_t *buf, int buf_size)
{
    auto *dev = static_cast<QIODevice *>(opaque);
    if (dev->atEnd())
        return AVERROR_EOF;
    return dev->read(reinterpret_cast<char *>(buf), buf_size);
}

static int64_t seek(void *opaque, int64_t offset, int whence)
{
    QIODevice *dev = static_cast<QIODevice *>(opaque);

    if (dev->isSequential())
        return AVERROR(EINVAL);

    if (whence & AVSEEK_SIZE)
        return dev->size();

    whence &= ~AVSEEK_FORCE;

    if (whence == SEEK_CUR)
        offset += dev->pos();
    else if (whence == SEEK_END)
        offset += dev->size();

    if (!dev->seek(offset))
        return AVERROR(EINVAL);
    return offset;
}

static void insertVideoData(QMediaMetaData &metaData, AVStream *stream)
{
    Q_ASSERT(stream);
    auto *codecPar = stream->codecpar;
    metaData.insert(QMediaMetaData::VideoBitRate, (int)codecPar->bit_rate);
    metaData.insert(QMediaMetaData::VideoCodec, QVariant::fromValue(QFFmpegMediaFormatInfo::videoCodecForAVCodecId(codecPar->codec_id)));
    metaData.insert(QMediaMetaData::Resolution, QSize(codecPar->width, codecPar->height));
    auto fr = toFloat(stream->avg_frame_rate);
    if (fr)
        metaData.insert(QMediaMetaData::VideoFrameRate, *fr);
};

static void insertAudioData(QMediaMetaData &metaData, AVStream *stream)
{
    Q_ASSERT(stream);
    auto *codecPar = stream->codecpar;
    metaData.insert(QMediaMetaData::AudioBitRate, (int)codecPar->bit_rate);
    metaData.insert(QMediaMetaData::AudioCodec,
                    QVariant::fromValue(QFFmpegMediaFormatInfo::audioCodecForAVCodecId(codecPar->codec_id)));
};

static int getDefaultStreamIndex(QList<Decoder::StreamInfo> &streams)
{
    if (streams.empty())
        return -1;
    for (qsizetype i = 0; i < streams.size(); i++)
        if (streams[i].isDefault)
            return i;
    return 0;
}

static void readStreams(const AVFormatContext *context,
                        QList<Decoder::StreamInfo> (&map)[QPlatformMediaPlayer::NTrackTypes], qint64 &maxDuration)
{
    maxDuration = 0;

    for (unsigned int i = 0; i < context->nb_streams; ++i) {
        auto *stream = context->streams[i];
        if (!stream)
            continue;

        auto *codecPar = stream->codecpar;
        if (!codecPar)
            continue;

        QMediaMetaData metaData = QFFmpegMetaData::fromAVMetaData(stream->metadata);
        bool isDefault = stream->disposition & AV_DISPOSITION_DEFAULT;
        QPlatformMediaPlayer::TrackType type = QPlatformMediaPlayer::VideoStream;

        switch (codecPar->codec_type) {
        case AVMEDIA_TYPE_UNKNOWN:
        case AVMEDIA_TYPE_DATA:          ///< Opaque data information usually continuous
        case AVMEDIA_TYPE_ATTACHMENT:    ///< Opaque data information usually sparse
        case AVMEDIA_TYPE_NB:
            continue;
        case AVMEDIA_TYPE_VIDEO:
            type = QPlatformMediaPlayer::VideoStream;
            insertVideoData(metaData, stream);
            break;
        case AVMEDIA_TYPE_AUDIO:
            type = QPlatformMediaPlayer::AudioStream;
            insertAudioData(metaData, stream);
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            type = QPlatformMediaPlayer::SubtitleStream;
            break;
        }

        map[type].append({ (int)i, isDefault, metaData });
        auto maybeDuration = mul(1'000'000ll * stream->duration, stream->time_base);
        if (maybeDuration)
            maxDuration = qMax(maxDuration, *maybeDuration);
    }
}

void Decoder::setMedia(const QUrl &media, QIODevice *stream)
{
    QByteArray url = media.toEncoded(QUrl::PreferLocalFile);

    AVFormatContext *context = nullptr;
    if (stream) {
        if (!stream->isOpen()) {
            if (!stream->open(QIODevice::ReadOnly)) {
                emit errorOccured(QMediaPlayer::ResourceError,
                                  QLatin1String("Could not open source device."));
                return;
            }
        }
        if (!stream->isSequential())
            stream->seek(0);
        context = avformat_alloc_context();
        constexpr int bufferSize = 32768;
        unsigned char *buffer = (unsigned char *)av_malloc(bufferSize);
        context->pb = avio_alloc_context(buffer, bufferSize, false, stream, ::read, nullptr, ::seek);
    }

    int ret = avformat_open_input(&context, url.constData(), nullptr, nullptr);
    if (ret < 0) {
        auto code = QMediaPlayer::ResourceError;
        if (ret == AVERROR(EACCES))
            code = QMediaPlayer::AccessDeniedError;
        else if (ret == AVERROR(EINVAL))
            code = QMediaPlayer::FormatError;

        emit errorOccured(code, QMediaPlayer::tr("Could not open file"));
        return;
    }

    ret = avformat_find_stream_info(context, nullptr);
    if (ret < 0) {
        emit errorOccured(QMediaPlayer::FormatError,
                          QMediaPlayer::tr("Could not find stream information for media file"));
        avformat_free_context(context);
        return;
    }

#ifndef QT_NO_DEBUG
    av_dump_format(context, 0, url.constData(), 0);
#endif

    readStreams(context, m_streamMap, m_duration);

    m_requestedStreams[QPlatformMediaPlayer::VideoStream] = getDefaultStreamIndex(m_streamMap[QPlatformMediaPlayer::VideoStream]);
    m_requestedStreams[QPlatformMediaPlayer::AudioStream] = getDefaultStreamIndex(m_streamMap[QPlatformMediaPlayer::AudioStream]);
    m_requestedStreams[QPlatformMediaPlayer::SubtitleStream] = -1;

    m_metaData = QFFmpegMetaData::fromAVMetaData(context->metadata);
    m_metaData.insert(QMediaMetaData::FileFormat,
                      QVariant::fromValue(QFFmpegMediaFormatInfo::fileFormatForAVInputFormat(context->iformat)));

    if (m_requestedStreams[QPlatformMediaPlayer::VideoStream] >= 0)
        insertVideoData(m_metaData, context->streams[avStreamIndex(QPlatformMediaPlayer::VideoStream)]);

    if (m_requestedStreams[QPlatformMediaPlayer::AudioStream] >= 0)
        insertAudioData(m_metaData, context->streams[avStreamIndex(QPlatformMediaPlayer::AudioStream)]);

    m_isSeekable = !(context->ctx_flags & AVFMTCTX_UNSEEKABLE);

    demuxer = new Demuxer(this, context);
    demuxer->start();
}

int Decoder::activeTrack(QPlatformMediaPlayer::TrackType type)
{
    return m_requestedStreams[type];
}

void Decoder::setActiveTrack(QPlatformMediaPlayer::TrackType type, int streamNumber)
{
    if (streamNumber < 0 || streamNumber >= m_streamMap[type].size())
        streamNumber = -1;
    if (m_requestedStreams[type] == streamNumber)
        return;
    m_requestedStreams[type] = streamNumber;
    changeAVTrack(type);
}

void Decoder::setState(QMediaPlayer::PlaybackState state)
{
    if (m_state == state)
        return;

    switch (state) {
    case QMediaPlayer::StoppedState:
        qCDebug(qLcDecoder) << "Decoder::stop";
        setPaused(true);
        if (demuxer)
            demuxer->stopDecoding();
        seek(0);
        if (videoSink)
            videoSink->setVideoFrame({});
        qCDebug(qLcDecoder) << "Decoder::stop: done";
        break;
    case QMediaPlayer::PausedState:
        qCDebug(qLcDecoder) << "Decoder::pause";
        setPaused(true);
        if (demuxer) {
            demuxer->startDecoding();
            demuxer->wake();
            if (m_state == QMediaPlayer::StoppedState)
                triggerStep();
        }
        break;
    case QMediaPlayer::PlayingState:
        qCDebug(qLcDecoder) << "Decoder::play";
        setPaused(false);
        if (demuxer)
            demuxer->startDecoding();
        break;
    }
    m_state = state;
}

void Decoder::setPaused(bool b)
{
    clockController.setPaused(b);
}

void Decoder::triggerStep()
{
    if (audioRenderer)
        audioRenderer->singleStep();
    if (videoRenderer)
        videoRenderer->singleStep();
}

void Decoder::setVideoSink(QVideoSink *sink)
{
    qCDebug(qLcDecoder) << "setVideoSink" << sink;
    if (sink == videoSink)
        return;
    videoSink = sink;
    if (!videoSink || m_requestedStreams[QPlatformMediaPlayer::VideoStream] < 0) {
        if (videoRenderer) {
            videoRenderer->kill();
            videoRenderer = nullptr;
        }
    } else if (!videoRenderer) {
        videoRenderer = new VideoRenderer(this, sink);
        connect(videoRenderer, &Renderer::atEnd, this, &Decoder::streamAtEnd);
        videoRenderer->start();
        StreamDecoder *stream = demuxer->addStream(avStreamIndex(QPlatformMediaPlayer::VideoStream));
        videoRenderer->setStream(stream);
        stream = demuxer->addStream(avStreamIndex(QPlatformMediaPlayer::SubtitleStream));
        videoRenderer->setSubtitleStream(stream);
    }
}

void Decoder::setAudioSink(QPlatformAudioOutput *output)
{
    if (audioOutput == output)
        return;

    qCDebug(qLcDecoder) << "setAudioSink" << audioOutput;
    audioOutput = output;
    if (!output || m_requestedStreams[QPlatformMediaPlayer::AudioStream] < 0) {
        if (audioRenderer) {
            audioRenderer->kill();
            audioRenderer = nullptr;
        }
    } else if (!audioRenderer) {
        audioRenderer = new AudioRenderer(this, output->q);
        connect(audioRenderer, &Renderer::atEnd, this, &Decoder::streamAtEnd);
        audioRenderer->start();
        auto *stream = demuxer->addStream(avStreamIndex(QPlatformMediaPlayer::AudioStream));
        audioRenderer->setStream(stream);
    }
}

void Decoder::changeAVTrack(QPlatformMediaPlayer::TrackType type)
{
    if (!demuxer)
        return;
    qCDebug(qLcDecoder) << "    applying to renderer.";
    if (m_state == QMediaPlayer::PlayingState)
        setPaused(true);
    auto *streamDecoder = demuxer->addStream(avStreamIndex(type));
    switch (type) {
    case QPlatformMediaPlayer::AudioStream:
        audioRenderer->setStream(streamDecoder);
        break;
    case QPlatformMediaPlayer::VideoStream:
        videoRenderer->setStream(streamDecoder);
        break;
    case QPlatformMediaPlayer::SubtitleStream:
        videoRenderer->setSubtitleStream(streamDecoder);
        break;
    default:
        Q_UNREACHABLE();
    }
    demuxer->seek(clockController.currentTime());
    if (m_state == QMediaPlayer::PlayingState)
        setPaused(false);
    else
        triggerStep();
}

void Decoder::seek(qint64 pos)
{
    if (!demuxer)
        return;
    pos = qBound(0, pos, m_duration);
    demuxer->seek(pos);
    clockController.syncTo(pos);
    demuxer->wake();
    if (m_state == QMediaPlayer::PausedState)
        triggerStep();
}

void Decoder::setPlaybackRate(float rate)
{
    clockController.setPlaybackRate(rate);
}

void Decoder::streamAtEnd()
{
    if (audioRenderer && !audioRenderer->isAtEnd())
        return;
    if (videoRenderer && !videoRenderer->isAtEnd())
        return;
    pause();

    emit endOfStream();
}

QT_END_NAMESPACE
