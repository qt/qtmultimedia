// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qffmpegencoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegvideoframeencoder_p.h"
#include "private/qmultimediautils_p.h"

#include <qdebug.h>
#include <qiodevice.h>
#include <qaudiosource.h>
#include <qaudiobuffer.h>
#include "qffmpegaudioinput_p.h"
#include <private/qplatformcamera_p.h>
#include <private/qplatformvideosource_p.h>
#include "qffmpegvideobuffer_p.h"
#include "qffmpegmediametadata_p.h"
#include "qffmpegencoderoptions_p.h"

#include <qloggingcategory.h>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/common.h>
}

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcFFmpegEncoder, "qt.multimedia.ffmpeg.encoder");

namespace QFFmpeg
{

namespace {

template<typename T>
T dequeueIfPossible(std::queue<T> &queue)
{
    if (queue.empty())
        return T{};

    auto result = std::move(queue.front());
    queue.pop();
    return result;
}

} // namespace

Encoder::Encoder(const QMediaEncoderSettings &settings, const QString &filePath)
    : settings(settings)
{
    const AVOutputFormat *avFormat = QFFmpegMediaFormatInfo::outputFormatForFileFormat(settings.fileFormat());
    formatContext = avformat_alloc_context();
    formatContext->oformat = const_cast<AVOutputFormat *>(avFormat); // constness varies

    QByteArray filePathUtf8 = filePath.toUtf8();
    formatContext->url = (char *)av_malloc(filePathUtf8.size() + 1);
    memcpy(formatContext->url, filePathUtf8.constData(), filePathUtf8.size() + 1);
    formatContext->pb = nullptr;

    // Initialize the AVIOContext for accessing the resource indicated by the url
    auto result = avio_open2(&formatContext->pb, formatContext->url, AVIO_FLAG_WRITE, nullptr, nullptr);
    qCDebug(qLcFFmpegEncoder) << "opened" << result << formatContext->url;

    muxer = new Muxer(this);
}

Encoder::~Encoder()
{
}

void Encoder::addAudioInput(QFFmpegAudioInput *input)
{
    audioEncode = new AudioEncoder(this, input, settings);
    addMediaFrameHandler(input, &QFFmpegAudioInput::newAudioBuffer, audioEncode,
                         &AudioEncoder::addBuffer);
    input->setRunning(true);
}

void Encoder::addVideoSource(QPlatformVideoSource * source)
{
    auto frameFormat = source->frameFormat();

    if (!frameFormat.isValid()) {
        qCWarning(qLcFFmpegEncoder) << "Cannot add source; invalid vide frame format";
        emit error(QMediaRecorder::ResourceError,
                   QLatin1StringView("Cannot get video source format"));
        return;
    }

    std::optional<AVPixelFormat> hwPixelFormat = source->ffmpegHWPixelFormat()
            ? AVPixelFormat(*source->ffmpegHWPixelFormat())
            : std::optional<AVPixelFormat>{};

    qCDebug(qLcFFmpegEncoder) << "adding video source" << source->metaObject()->className() << ":"
                              << "pixelFormat=" << frameFormat.pixelFormat()
                              << "frameSize=" << frameFormat.frameSize()
                              << "frameRate=" << frameFormat.frameRate() << "ffmpegHWPixelFormat="
                              << (hwPixelFormat ? *hwPixelFormat : AV_PIX_FMT_NONE);

    auto veUPtr = std::make_unique<VideoEncoder>(this, settings, frameFormat, hwPixelFormat);
    if (!veUPtr->isValid()) {
        emit error(QMediaRecorder::FormatError, QLatin1StringView("Cannot initialize encoder"));
        return;
    }

    auto ve = veUPtr.release();
    addMediaFrameHandler(source, &QPlatformVideoSource::newVideoFrame, ve, &VideoEncoder::addFrame);
    videoEncoders.append(ve);
}

void Encoder::start()
{
    qCDebug(qLcFFmpegEncoder) << "Encoder::start!";

    formatContext->metadata = QFFmpegMetaData::toAVMetaData(metaData);

    Q_ASSERT(!isHeaderWritten);

    int res = avformat_write_header(formatContext, nullptr);
    if (res < 0) {
        qWarning() << "could not write header, error:" << res << err2str(res);
        emit error(QMediaRecorder::ResourceError, "Cannot start writing the stream");
        return;
    }

    isHeaderWritten = true;

    qCDebug(qLcFFmpegEncoder) << "stream header is successfully written";

    muxer->start();
    if (audioEncode)
        audioEncode->start();
    for (auto *videoEncoder : videoEncoders)
        if (videoEncoder->isValid())
            videoEncoder->start();
}

EncodingFinalizer::EncodingFinalizer(Encoder *e) : encoder(e) {
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void EncodingFinalizer::run()
{
    if (encoder->audioEncode)
        encoder->audioEncode->stopAndDelete();
    for (auto &videoEncoder : encoder->videoEncoders)
        videoEncoder->stopAndDelete();
    encoder->muxer->stopAndDelete();

    if (encoder->isHeaderWritten) {
        const int res = av_write_trailer(encoder->formatContext);
        if (res < 0)
            qWarning() << "could not write trailer" << res;
    }
    // else ffmpeg might crash

    // Close the AVIOContext and release any file handles
    const int res = avio_close(encoder->formatContext->pb);
    Q_ASSERT(res == 0);

    avformat_free_context(encoder->formatContext);
    qCDebug(qLcFFmpegEncoder) << "    done finalizing.";
    emit encoder->finalizationDone();
    delete encoder;
}

void Encoder::finalize()
{
    qCDebug(qLcFFmpegEncoder) << ">>>>>>>>>>>>>>> finalize";

    for (auto &conn : connections)
        disconnect(conn);

    auto *finalizer = new EncodingFinalizer(this);
    finalizer->start();
}

void Encoder::setPaused(bool p)
{
    if (audioEncode)
       audioEncode->setPaused(p);
    for (auto &videoEncoder : videoEncoders)
       videoEncoder->setPaused(p);
}

void Encoder::setMetaData(const QMediaMetaData &metaData)
{
    this->metaData = metaData;
}

void Encoder::newTimeStamp(qint64 time)
{
    QMutexLocker locker(&timeMutex);
    if (time > timeRecorded) {
        timeRecorded = time;
        emit durationChanged(time);
    }
}

template<typename... Args>
void Encoder::addMediaFrameHandler(Args &&...args)
{
    auto connection = connect(std::forward<Args>(args)..., Qt::DirectConnection);
    connections.append(connection);
}

Muxer::Muxer(Encoder *encoder)
    : encoder(encoder)
{
    setObjectName(QLatin1String("Muxer"));
}

void Muxer::addPacket(AVPacketUPtr packet)
{
    {
        QMutexLocker locker(&queueMutex);
        packetQueue.push(std::move(packet));
    }

    //    qCDebug(qLcFFmpegEncoder) << "Muxer::addPacket" << packet->pts << packet->stream_index;
    dataReady();
}

AVPacketUPtr Muxer::takePacket()
{
    QMutexLocker locker(&queueMutex);
    return dequeueIfPossible(packetQueue);
}

void Muxer::init()
{
    qCDebug(qLcFFmpegEncoder) << "Muxer::init started thread.";
}

void Muxer::cleanup()
{
}

bool QFFmpeg::Muxer::hasData() const
{
    QMutexLocker locker(&queueMutex);
    return !packetQueue.empty();
}

void Muxer::processOne()
{
    auto packet = takePacket();
    //   qCDebug(qLcFFmpegEncoder) << "writing packet to file" << packet->pts << packet->duration <<
    //   packet->stream_index;

    // the function takes ownership for the packet
    av_interleaved_write_frame(encoder->formatContext, packet.release());
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
        qCDebug(qLcFFmpegEncoder) << "format:" << *f;
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

AudioEncoder::AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input, const QMediaEncoderSettings &settings)
    : input(input)
    , settings(settings)
{
    this->encoder = encoder;

    setObjectName(QLatin1String("AudioEncoder"));
    qCDebug(qLcFFmpegEncoder) << "AudioEncoder" << settings.audioCodec();

    format = input->device.preferredFormat();
    auto codecID = QFFmpegMediaFormatInfo::codecIdForAudioCodec(settings.audioCodec());
    Q_ASSERT(avformat_query_codec(encoder->formatContext->oformat, codecID, FF_COMPLIANCE_NORMAL));

    AVSampleFormat requested = QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat());

    avCodec = QFFmpeg::findAVEncoder(codecID, {}, requested);

    if (!avCodec)
        avCodec = QFFmpeg::findAVEncoder(codecID);

    qCDebug(qLcFFmpegEncoder) << "found audio codec" << avCodec->name;

    Q_ASSERT(avCodec);

    AVSampleFormat bestSampleFormat = bestMatchingSampleFormat(requested, avCodec->sample_fmts);

    stream = avformat_new_stream(encoder->formatContext, nullptr);
    stream->id = encoder->formatContext->nb_streams - 1;
    stream->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    stream->codecpar->codec_id = codecID;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    stream->codecpar->channel_layout = av_get_default_channel_layout(format.channelCount());
    stream->codecpar->channels = format.channelCount();
#else
    av_channel_layout_default(&stream->codecpar->ch_layout, format.channelCount());
#endif
    stream->codecpar->sample_rate = format.sampleRate();
    stream->codecpar->frame_size = 1024;
    stream->codecpar->format = bestSampleFormat;
    stream->time_base = AVRational{ 1, format.sampleRate() };

    qCDebug(qLcFFmpegEncoder) << "set stream time_base" << stream->time_base.num << "/"
                              << stream->time_base.den;
}

void AudioEncoder::open()
{
    AVSampleFormat requested = QFFmpegMediaFormatInfo::avSampleFormat(format.sampleFormat());

    codecContext.reset(avcodec_alloc_context3(avCodec));

    if (stream->time_base.num != 1 || stream->time_base.den != format.sampleRate()) {
        qCDebug(qLcFFmpegEncoder) << "Most likely, av_format_write_header changed time base from"
                                  << 1 << "/" << format.sampleRate() << "to" << stream->time_base;
    }

    codecContext->time_base = stream->time_base;

    avcodec_parameters_to_context(codecContext.get(), stream->codecpar);

    AVDictionaryHolder opts;
    applyAudioEncoderOptions(settings, avCodec->name, codecContext.get(), opts);

    int res = avcodec_open2(codecContext.get(), avCodec, opts);
    qCDebug(qLcFFmpegEncoder) << "audio codec opened" << res;
    qCDebug(qLcFFmpegEncoder) << "audio codec params: fmt=" << codecContext->sample_fmt
                              << "rate=" << codecContext->sample_rate;

    if (codecContext->sample_fmt != requested) {
        SwrContext *resampler = nullptr;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
        resampler = swr_alloc_set_opts(
                nullptr, // we're allocating a new context
                codecContext->channel_layout, // out_ch_layout
                codecContext->sample_fmt, // out_sample_fmt
                codecContext->sample_rate, // out_sample_rate
                av_get_default_channel_layout(format.channelCount()), // in_ch_layout
                requested, // in_sample_fmt
                format.sampleRate(), // in_sample_rate
                0, // log_offset
                nullptr);
#else
        AVChannelLayout in_ch_layout = {};
        av_channel_layout_default(&in_ch_layout, format.channelCount());
        swr_alloc_set_opts2(&resampler, // we're allocating a new context
                            &codecContext->ch_layout, codecContext->sample_fmt,
                            codecContext->sample_rate, &in_ch_layout, requested,
                            format.sampleRate(), 0, nullptr);
#endif

        swr_init(resampler);

        this->resampler.reset(resampler);
    }
}

void AudioEncoder::addBuffer(const QAudioBuffer &buffer)
{
    QMutexLocker locker(&queueMutex);
    if (!paused.loadRelaxed()) {
        audioBufferQueue.push(buffer);
        locker.unlock();
        dataReady();
    }
}

QAudioBuffer AudioEncoder::takeBuffer()
{
    QMutexLocker locker(&queueMutex);
    return dequeueIfPossible(audioBufferQueue);
}

void AudioEncoder::init()
{
    open();
    if (input) {
        input->setFrameSize(codecContext->frame_size);
    }
    qCDebug(qLcFFmpegEncoder) << "AudioEncoder::init started audio device thread.";
}

void AudioEncoder::cleanup()
{
    while (!audioBufferQueue.empty())
        processOne();
    while (avcodec_send_frame(codecContext.get(), nullptr) == AVERROR(EAGAIN))
        retrievePackets();
    retrievePackets();
}

bool AudioEncoder::hasData() const
{
    QMutexLocker locker(&queueMutex);
    return !audioBufferQueue.empty();
}

void AudioEncoder::retrievePackets()
{
    while (1) {
        AVPacketUPtr packet(av_packet_alloc());
        int ret = avcodec_receive_packet(codecContext.get(), packet.get());
        if (ret < 0) {
            if (ret != AVERROR(EOF))
                break;
            if (ret != AVERROR(EAGAIN)) {
                char errStr[1024];
                av_strerror(ret, errStr, 1024);
                qCDebug(qLcFFmpegEncoder) << "receive packet" << ret << errStr;
            }
            break;
        }

        // qCDebug(qLcFFmpegEncoder) << "writing audio packet" << packet->size << packet->pts << packet->dts;
        packet->stream_index = stream->id;
        encoder->muxer->addPacket(std::move(packet));
    }
}

void AudioEncoder::processOne()
{
    QAudioBuffer buffer = takeBuffer();
    if (!buffer.isValid() || paused.loadAcquire())
        return;

//    qCDebug(qLcFFmpegEncoder) << "new audio buffer" << buffer.byteCount() << buffer.format() << buffer.frameCount() << codec->frame_size;
    retrievePackets();

    auto frame = makeAVFrame();
    frame->format = codecContext->sample_fmt;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    frame->channel_layout = codecContext->channel_layout;
    frame->channels = codecContext->channels;
#else
    frame->ch_layout = codecContext->ch_layout;
#endif
    frame->sample_rate = codecContext->sample_rate;
    frame->nb_samples = buffer.frameCount();
    if (frame->nb_samples)
        av_frame_get_buffer(frame.get(), 0);

    if (resampler) {
        const uint8_t *data = buffer.constData<uint8_t>();
        swr_convert(resampler.get(), frame->extended_data, frame->nb_samples, &data,
                    frame->nb_samples);
    } else {
        memcpy(frame->buf[0]->data, buffer.constData<uint8_t>(), buffer.byteCount());
    }

    const auto &timeBase = stream->time_base;
    const auto pts = timeBase.den && timeBase.num
            ? timeBase.den * samplesWritten / (codecContext->sample_rate * timeBase.num)
            : samplesWritten;
    setAVFrameTime(*frame, pts, timeBase);
    samplesWritten += buffer.frameCount();

    qint64 time = format.durationForFrames(samplesWritten);
    encoder->newTimeStamp(time/1000);

    //    qCDebug(qLcFFmpegEncoder) << "sending audio frame" << buffer.byteCount() << frame->pts <<
    //    ((double)buffer.frameCount()/frame->sample_rate);

    int ret = avcodec_send_frame(codecContext.get(), frame.get());
    if (ret < 0) {
        char errStr[1024];
        av_strerror(ret, errStr, 1024);
//        qCDebug(qLcFFmpegEncoder) << "error sending frame" << ret << errStr;
    }
}

VideoEncoder::VideoEncoder(Encoder *encoder, const QMediaEncoderSettings &settings,
                           const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat)
{
    this->encoder = encoder;

    setObjectName(QLatin1String("VideoEncoder"));

    AVPixelFormat swFormat = QFFmpegVideoBuffer::toAVPixelFormat(format.pixelFormat());
    AVPixelFormat ffmpegPixelFormat =
            hwFormat && *hwFormat != AV_PIX_FMT_NONE ? *hwFormat : swFormat;
    auto frameRate = format.frameRate();
    if (frameRate <= 0.) {
        qWarning() << "Invalid frameRate" << frameRate << "; Using the default instead";

        // set some default frame rate since ffmpeg has UB if it's 0.
        frameRate = 30.;
    }

    frameEncoder = VideoFrameEncoder::create(settings, format.frameSize(), frameRate,
                                             ffmpegPixelFormat, swFormat, encoder->formatContext);
}

VideoEncoder::~VideoEncoder() = default;

bool VideoEncoder::isValid() const
{
    return frameEncoder != nullptr;
}

void VideoEncoder::addFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&queueMutex);

    // Drop frames if encoder can not keep up with the video source data rate
    const bool queueFull = videoFrameQueue.size() >= maxQueueSize;

    if (queueFull) {
        qCDebug(qLcFFmpegEncoder) << "Encoder frame queue full. Frame lost.";
    } else if (!paused.loadRelaxed()) {
        videoFrameQueue.push(frame);

        locker.unlock(); // Avoid context switch on wake wake-up

        dataReady();
    }
}

QVideoFrame VideoEncoder::takeFrame()
{
    QMutexLocker locker(&queueMutex);
    return dequeueIfPossible(videoFrameQueue);
}

void VideoEncoder::retrievePackets()
{
    if (!frameEncoder)
        return;
    while (auto packet = frameEncoder->retrievePacket())
        encoder->muxer->addPacket(std::move(packet));
}

void VideoEncoder::init()
{
    qCDebug(qLcFFmpegEncoder) << "VideoEncoder::init started video device thread.";
    bool ok = frameEncoder->open();
    if (!ok)
        emit encoder->error(QMediaRecorder::ResourceError, "Could not initialize encoder");
}

void VideoEncoder::cleanup()
{
    while (!videoFrameQueue.empty())
        processOne();
    if (frameEncoder) {
        while (frameEncoder->sendFrame(nullptr) == AVERROR(EAGAIN))
            retrievePackets();
        retrievePackets();
    }
}

bool VideoEncoder::hasData() const
{
    QMutexLocker locker(&queueMutex);
    return !videoFrameQueue.empty();
}

struct QVideoFrameHolder
{
    QVideoFrame f;
    QImage i;
};

static void freeQVideoFrame(void *opaque, uint8_t *)
{
    delete reinterpret_cast<QVideoFrameHolder *>(opaque);
}

void VideoEncoder::processOne()
{
    if (paused.loadAcquire())
        return;

    retrievePackets();

    auto frame = takeFrame();
    if (!frame.isValid())
        return;

    if (!isValid())
        return;

//    qCDebug(qLcFFmpegEncoder) << "new video buffer" << frame.startTime();

    AVFrameUPtr avFrame;

    auto *videoBuffer = dynamic_cast<QFFmpegVideoBuffer *>(frame.videoBuffer());
    if (videoBuffer) {
        // ffmpeg video buffer, let's use the native AVFrame stored in there
        auto *hwFrame = videoBuffer->getHWFrame();
        if (hwFrame && hwFrame->format == frameEncoder->sourceFormat())
            avFrame.reset(av_frame_clone(hwFrame));
    }

    if (!avFrame) {
        frame.map(QVideoFrame::ReadOnly);
        auto size = frame.size();
        avFrame = makeAVFrame();
        avFrame->format = frameEncoder->sourceFormat();
        avFrame->width = size.width();
        avFrame->height = size.height();

        for (int i = 0; i < 4; ++i) {
            avFrame->data[i] = const_cast<uint8_t *>(frame.bits(i));
            avFrame->linesize[i] = frame.bytesPerLine(i);
        }

        QImage img;
        if (frame.pixelFormat() == QVideoFrameFormat::Format_Jpeg) {
            // the QImage is cached inside the video frame, so we can take the pointer to the image data here
            img = frame.toImage();
            avFrame->data[0] = (uint8_t *)img.bits();
            avFrame->linesize[0] = img.bytesPerLine();
        }

        Q_ASSERT(avFrame->data[0]);
        // ensure the video frame and it's data is alive as long as it's being used in the encoder
        avFrame->opaque_ref = av_buffer_create(nullptr, 0, freeQVideoFrame, new QVideoFrameHolder{frame, img}, 0);
    }

    if (baseTime.loadAcquire() == std::numeric_limits<qint64>::min()) {
        baseTime.storeRelease(frame.startTime() - lastFrameTime);
        qCDebug(qLcFFmpegEncoder) << ">>>> adjusting base time to" << baseTime.loadAcquire()
                                  << frame.startTime() << lastFrameTime;
    }

    qint64 time = frame.startTime() - baseTime.loadAcquire();
    lastFrameTime = frame.endTime() - baseTime.loadAcquire();

    setAVFrameTime(*avFrame, frameEncoder->getPts(time), frameEncoder->getTimeBase());

    encoder->newTimeStamp(time/1000);

    qCDebug(qLcFFmpegEncoder) << ">>> sending frame" << avFrame->pts << time << lastFrameTime;
    int ret = frameEncoder->sendFrame(std::move(avFrame));
    if (ret < 0) {
        qCDebug(qLcFFmpegEncoder) << "error sending frame" << ret << err2str(ret);
        encoder->error(QMediaRecorder::ResourceError, err2str(ret));
    }
}

}

QT_END_NAMESPACE

#include "moc_qffmpegencoder_p.cpp"
