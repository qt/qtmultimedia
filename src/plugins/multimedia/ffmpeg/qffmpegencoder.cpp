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
#include "qffmpegaudioencoderutils_p.h"

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
    : m_settings(settings), m_formatContext(settings.fileFormat())
{
    m_formatContext.openAVIO(filePath);
    m_muxer = new Muxer(this);
}

Encoder::~Encoder()
{
}

void Encoder::addAudioInput(QFFmpegAudioInput *input)
{
    m_audioEncoder = new AudioEncoder(this, input, m_settings);
    addMediaFrameHandler(input, &QFFmpegAudioInput::newAudioBuffer, m_audioEncoder,
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

    auto veUPtr = std::make_unique<VideoEncoder>(this, m_settings, frameFormat, hwPixelFormat);
    if (!veUPtr->isValid()) {
        emit error(QMediaRecorder::FormatError, QLatin1StringView("Cannot initialize encoder"));
        return;
    }

    auto ve = veUPtr.release();
    addMediaFrameHandler(source, &QPlatformVideoSource::newVideoFrame, ve, &VideoEncoder::addFrame);
    m_videoEncoders.append(ve);
}

void Encoder::start()
{
    qCDebug(qLcFFmpegEncoder) << "Encoder::start!";

    if (!m_formatContext.isAVIOOpen()) {
        qWarning() << "AVIOContext is null";
        emit error(QMediaRecorder::ResourceError,
                   "AVIOContext initialization failed. Cannot start");
        return;
    }

    m_formatContext->metadata = QFFmpegMetaData::toAVMetaData(m_metaData);

    Q_ASSERT(!m_isHeaderWritten);

    int res = avformat_write_header(m_formatContext, nullptr);
    if (res < 0) {
        qWarning() << "could not write header, error:" << res << err2str(res);
        emit error(QMediaRecorder::ResourceError, "Cannot start writing the stream");
        return;
    }

    m_isHeaderWritten = true;

    qCDebug(qLcFFmpegEncoder) << "stream header is successfully written";

    m_muxer->start();
    if (m_audioEncoder)
        m_audioEncoder->start();
    for (auto *videoEncoder : m_videoEncoders)
        if (videoEncoder->isValid())
            videoEncoder->start();
}

EncodingFinalizer::EncodingFinalizer(Encoder *e) : m_encoder(e)
{
    connect(this, &QThread::finished, this, &QObject::deleteLater);
}

void EncodingFinalizer::run()
{
    if (m_encoder->m_audioEncoder)
        m_encoder->m_audioEncoder->stopAndDelete();
    for (auto &videoEncoder : m_encoder->m_videoEncoders)
        videoEncoder->stopAndDelete();
    m_encoder->m_muxer->stopAndDelete();

    if (m_encoder->m_isHeaderWritten) {
        const int res = av_write_trailer(m_encoder->m_formatContext);
        if (res < 0) {
            const auto errorDescription = err2str(res);
            qCWarning(qLcFFmpegEncoder) << "could not write trailer" << res << errorDescription;
            emit m_encoder->error(QMediaRecorder::FormatError,
                                  QLatin1String("Cannot write trailer: ") + errorDescription);
        }
    }
    // else ffmpeg might crash

    m_encoder->m_formatContext.closeAVIO();

    qCDebug(qLcFFmpegEncoder) << "    done finalizing.";
    emit m_encoder->finalizationDone();
    delete m_encoder;
}

void Encoder::finalize()
{
    qCDebug(qLcFFmpegEncoder) << ">>>>>>>>>>>>>>> finalize";

    for (auto &conn : m_connections)
        disconnect(conn);

    auto *finalizer = new EncodingFinalizer(this);
    finalizer->start();
}

void Encoder::setPaused(bool p)
{
    if (m_audioEncoder)
        m_audioEncoder->setPaused(p);
    for (auto &videoEncoder : m_videoEncoders)
        videoEncoder->setPaused(p);
}

void Encoder::setMetaData(const QMediaMetaData &metaData)
{
    m_metaData = metaData;
}

void Encoder::newTimeStamp(qint64 time)
{
    QMutexLocker locker(&m_timeMutex);
    if (time > m_timeRecorded) {
        m_timeRecorded = time;
        emit durationChanged(time);
    }
}

template<typename... Args>
void Encoder::addMediaFrameHandler(Args &&...args)
{
    auto connection = connect(std::forward<Args>(args)..., Qt::DirectConnection);
    m_connections.append(connection);
}

Muxer::Muxer(Encoder *encoder) : m_encoder(encoder)
{
    setObjectName(QLatin1String("Muxer"));
}

void Muxer::addPacket(AVPacketUPtr packet)
{
    {
        QMutexLocker locker(&m_queueMutex);
        m_packetQueue.push(std::move(packet));
    }

    //    qCDebug(qLcFFmpegEncoder) << "Muxer::addPacket" << packet->pts << packet->stream_index;
    dataReady();
}

AVPacketUPtr Muxer::takePacket()
{
    QMutexLocker locker(&m_queueMutex);
    return dequeueIfPossible(m_packetQueue);
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
    QMutexLocker locker(&m_queueMutex);
    return !m_packetQueue.empty();
}

void Muxer::processOne()
{
    auto packet = takePacket();
    //   qCDebug(qLcFFmpegEncoder) << "writing packet to file" << packet->pts << packet->duration <<
    //   packet->stream_index;

    // the function takes ownership for the packet
    av_interleaved_write_frame(m_encoder->m_formatContext, packet.release());
}

AudioEncoder::AudioEncoder(Encoder *encoder, QFFmpegAudioInput *input,
                           const QMediaEncoderSettings &settings)
    : EncoderThread(encoder), m_input(input), m_settings(settings)
{
    setObjectName(QLatin1String("AudioEncoder"));
    qCDebug(qLcFFmpegEncoder) << "AudioEncoder" << settings.audioCodec();

    m_format = input->device.preferredFormat();
    auto codecID = QFFmpegMediaFormatInfo::codecIdForAudioCodec(settings.audioCodec());
    Q_ASSERT(avformat_query_codec(encoder->m_formatContext->oformat, codecID, FF_COMPLIANCE_NORMAL));

    const AVAudioFormat requestedAudioFormat(m_format);

    m_avCodec = QFFmpeg::findAVEncoder(codecID, {}, requestedAudioFormat.sampleFormat);

    if (!m_avCodec)
        m_avCodec = QFFmpeg::findAVEncoder(codecID);

    qCDebug(qLcFFmpegEncoder) << "found audio codec" << m_avCodec->name;

    Q_ASSERT(m_avCodec);

    m_stream = avformat_new_stream(encoder->m_formatContext, nullptr);
    m_stream->id = encoder->m_formatContext->nb_streams - 1;
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

    qCDebug(qLcFFmpegEncoder) << "set stream time_base" << m_stream->time_base.num << "/"
                              << m_stream->time_base.den;
}

void AudioEncoder::open()
{
    m_codecContext.reset(avcodec_alloc_context3(m_avCodec));

    if (m_stream->time_base.num != 1 || m_stream->time_base.den != m_format.sampleRate()) {
        qCDebug(qLcFFmpegEncoder) << "Most likely, av_format_write_header changed time base from"
                                  << 1 << "/" << m_format.sampleRate() << "to"
                                  << m_stream->time_base;
    }

    m_codecContext->time_base = m_stream->time_base;

    avcodec_parameters_to_context(m_codecContext.get(), m_stream->codecpar);

    AVDictionaryHolder opts;
    applyAudioEncoderOptions(m_settings, m_avCodec->name, m_codecContext.get(), opts);
    applyExperimentalCodecOptions(m_avCodec, opts);

    int res = avcodec_open2(m_codecContext.get(), m_avCodec, opts);
    qCDebug(qLcFFmpegEncoder) << "audio codec opened" << res;
    qCDebug(qLcFFmpegEncoder) << "audio codec params: fmt=" << m_codecContext->sample_fmt
                              << "rate=" << m_codecContext->sample_rate;

    const AVAudioFormat requestedAudioFormat(m_format);
    const AVAudioFormat codecAudioFormat(m_codecContext.get());

    if (requestedAudioFormat != codecAudioFormat)
        m_resampler = createResampleContext(requestedAudioFormat, codecAudioFormat);
}

void AudioEncoder::addBuffer(const QAudioBuffer &buffer)
{
    QMutexLocker locker(&m_queueMutex);
    if (!m_paused.loadRelaxed()) {
        m_audioBufferQueue.push(buffer);
        locker.unlock();
        dataReady();
    }
}

QAudioBuffer AudioEncoder::takeBuffer()
{
    QMutexLocker locker(&m_queueMutex);
    return dequeueIfPossible(m_audioBufferQueue);
}

void AudioEncoder::init()
{
    open();
    if (m_input) {
        m_input->setFrameSize(m_codecContext->frame_size);
    }
    qCDebug(qLcFFmpegEncoder) << "AudioEncoder::init started audio device thread.";
}

void AudioEncoder::cleanup()
{
    while (!m_audioBufferQueue.empty())
        processOne();
    while (avcodec_send_frame(m_codecContext.get(), nullptr) == AVERROR(EAGAIN))
        retrievePackets();
    retrievePackets();
}

bool AudioEncoder::hasData() const
{
    QMutexLocker locker(&m_queueMutex);
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
                qCDebug(qLcFFmpegEncoder) << "receive packet" << ret << errStr;
            }
            break;
        }

        // qCDebug(qLcFFmpegEncoder) << "writing audio packet" << packet->size << packet->pts << packet->dts;
        packet->stream_index = m_stream->id;
        m_encoder->m_muxer->addPacket(std::move(packet));
    }
}

void AudioEncoder::processOne()
{
    QAudioBuffer buffer = takeBuffer();
    if (!buffer.isValid() || m_paused.loadAcquire())
        return;

    if (buffer.format() != m_format) {
        // should we recreate recreate resampler here?
        qWarning() << "Get invalid audio format:" << buffer.format() << ", expected:" << m_format;
        return;
    }

//    qCDebug(qLcFFmpegEncoder) << "new audio buffer" << buffer.byteCount() << buffer.format() << buffer.frameCount() << codec->frame_size;
    retrievePackets();

    auto frame = makeAVFrame();
    frame->format = m_codecContext->sample_fmt;
#if QT_FFMPEG_OLD_CHANNEL_LAYOUT
    frame->channel_layout = m_codecContext->channel_layout;
    frame->channels = m_codecContext->channels;
#else
    frame->ch_layout = m_codecContext->ch_layout;
#endif
    frame->sample_rate = m_codecContext->sample_rate;
    frame->nb_samples = buffer.frameCount();
    if (frame->nb_samples)
        av_frame_get_buffer(frame.get(), 0);

    if (m_resampler) {
        const uint8_t *data = buffer.constData<uint8_t>();
        swr_convert(m_resampler.get(), frame->extended_data, frame->nb_samples, &data,
                    frame->nb_samples);
    } else {
        memcpy(frame->buf[0]->data, buffer.constData<uint8_t>(), buffer.byteCount());
    }

    const auto &timeBase = m_stream->time_base;
    const auto pts = timeBase.den && timeBase.num
            ? timeBase.den * m_samplesWritten / (m_codecContext->sample_rate * timeBase.num)
            : m_samplesWritten;
    setAVFrameTime(*frame, pts, timeBase);
    m_samplesWritten += buffer.frameCount();

    qint64 time = m_format.durationForFrames(m_samplesWritten);
    m_encoder->newTimeStamp(time / 1000);

    //    qCDebug(qLcFFmpegEncoder) << "sending audio frame" << buffer.byteCount() << frame->pts <<
    //    ((double)buffer.frameCount()/frame->sample_rate);

    int ret = avcodec_send_frame(m_codecContext.get(), frame.get());
    if (ret < 0) {
        char errStr[1024];
        av_strerror(ret, errStr, 1024);
//        qCDebug(qLcFFmpegEncoder) << "error sending frame" << ret << errStr;
    }
}

VideoEncoder::VideoEncoder(Encoder *encoder, const QMediaEncoderSettings &settings,
                           const QVideoFrameFormat &format, std::optional<AVPixelFormat> hwFormat)
    : EncoderThread(encoder)
{
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

    m_frameEncoder =
            VideoFrameEncoder::create(settings, format.frameSize(), frameRate, ffmpegPixelFormat,
                                      swFormat, encoder->m_formatContext);
}

VideoEncoder::~VideoEncoder() = default;

bool VideoEncoder::isValid() const
{
    return m_frameEncoder != nullptr;
}

void VideoEncoder::addFrame(const QVideoFrame &frame)
{
    QMutexLocker locker(&m_queueMutex);

    // Drop frames if encoder can not keep up with the video source data rate
    const bool queueFull = m_videoFrameQueue.size() >= m_maxQueueSize;

    if (queueFull) {
        qCDebug(qLcFFmpegEncoder) << "Encoder frame queue full. Frame lost.";
    } else if (!m_paused.loadRelaxed()) {
        m_videoFrameQueue.push(frame);

        locker.unlock(); // Avoid context switch on wake wake-up

        dataReady();
    }
}

QVideoFrame VideoEncoder::takeFrame()
{
    QMutexLocker locker(&m_queueMutex);
    return dequeueIfPossible(m_videoFrameQueue);
}

void VideoEncoder::retrievePackets()
{
    if (!m_frameEncoder)
        return;
    while (auto packet = m_frameEncoder->retrievePacket())
        m_encoder->m_muxer->addPacket(std::move(packet));
}

void VideoEncoder::init()
{
    qCDebug(qLcFFmpegEncoder) << "VideoEncoder::init started video device thread.";
    bool ok = m_frameEncoder->open();
    if (!ok)
        emit m_encoder->error(QMediaRecorder::ResourceError, "Could not initialize encoder");
}

void VideoEncoder::cleanup()
{
    while (!m_videoFrameQueue.empty())
        processOne();
    if (m_frameEncoder) {
        while (m_frameEncoder->sendFrame(nullptr) == AVERROR(EAGAIN))
            retrievePackets();
        retrievePackets();
    }
}

bool VideoEncoder::hasData() const
{
    QMutexLocker locker(&m_queueMutex);
    return !m_videoFrameQueue.empty();
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
    if (m_paused.loadAcquire())
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
        if (hwFrame && hwFrame->format == m_frameEncoder->sourceFormat())
            avFrame.reset(av_frame_clone(hwFrame));
    }

    if (!avFrame) {
        frame.map(QVideoFrame::ReadOnly);
        auto size = frame.size();
        avFrame = makeAVFrame();
        avFrame->format = m_frameEncoder->sourceFormat();
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

    if (m_baseTime.loadAcquire() == std::numeric_limits<qint64>::min()) {
        m_baseTime.storeRelease(frame.startTime() - m_lastFrameTime);
        qCDebug(qLcFFmpegEncoder) << ">>>> adjusting base time to" << m_baseTime.loadAcquire()
                                  << frame.startTime() << m_lastFrameTime;
    }

    qint64 time = frame.startTime() - m_baseTime.loadAcquire();
    m_lastFrameTime = frame.endTime() - m_baseTime.loadAcquire();

    setAVFrameTime(*avFrame, m_frameEncoder->getPts(time), m_frameEncoder->getTimeBase());

    m_encoder->newTimeStamp(time / 1000);

    qCDebug(qLcFFmpegEncoder) << ">>> sending frame" << avFrame->pts << time << m_lastFrameTime;
    int ret = m_frameEncoder->sendFrame(std::move(avFrame));
    if (ret < 0) {
        qCDebug(qLcFFmpegEncoder) << "error sending frame" << ret << err2str(ret);
        emit m_encoder->error(QMediaRecorder::ResourceError, err2str(ret));
    }
}

}

QT_END_NAMESPACE

#include "moc_qffmpegencoder_p.cpp"
