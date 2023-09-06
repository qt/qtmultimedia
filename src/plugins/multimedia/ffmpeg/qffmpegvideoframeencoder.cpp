// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoframeencoder_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegencoderoptions_p.h"
#include "qffmpegvideoencoderutils_p.h"
#include "private/qplatformmediarecorder_p.h"
#include "private/qmultimediautils_p.h"
#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcVideoFrameEncoder, "qt.multimedia.ffmpeg.videoencoder");

namespace QFFmpeg {

std::unique_ptr<VideoFrameEncoder>
VideoFrameEncoder::create(const QMediaEncoderSettings &encoderSettings, const QSize &sourceSize,
                          qreal sourceFrameRate, AVPixelFormat sourceFormat,
                          AVPixelFormat sourceSWFormat, AVFormatContext *formatContext)
{
    Q_ASSERT(isSwPixelFormat(sourceSWFormat));
    Q_ASSERT(isHwPixelFormat(sourceFormat) || sourceSWFormat == sourceFormat);

    std::unique_ptr<VideoFrameEncoder> result(new VideoFrameEncoder);

    result->m_settings = encoderSettings;
    result->m_sourceSize = sourceSize;
    result->m_sourceFormat = sourceFormat;

    // Temporary: check isSwPixelFormat because of android issue (QTBUG-116836)
    result->m_sourceSWFormat = isSwPixelFormat(sourceFormat) ? sourceFormat : sourceSWFormat;

    if (!result->m_settings.videoResolution().isValid())
        result->m_settings.setVideoResolution(sourceSize);

    if (result->m_settings.videoFrameRate() <= 0.)
        result->m_settings.setVideoFrameRate(sourceFrameRate);

    if (!result->initCodec() || !result->initTargetFormats()
        || !result->initCodecContext(formatContext)) {
        return nullptr;
    }

    // TODO: make VideoFrameEncoder::private and do openning here
    // if (!open()) {
    //    m_error = QMediaRecorder::FormatError;
    //    m_errorStr = QLatin1StringView("Cannot open codec");
    //    return;
    // }

    result->updateConversions();

    return result;
}

bool VideoFrameEncoder::initCodec()
{
    const auto qVideoCodec = m_settings.videoCodec();
    const auto codecID = QFFmpegMediaFormatInfo::codecIdForVideoCodec(qVideoCodec);
    const auto resolution = m_settings.videoResolution();

    std::tie(m_codec, m_accel) = findHwEncoder(codecID, resolution);

    if (!m_codec)
        m_codec = findSwEncoder(codecID, m_sourceSWFormat);

    if (!m_codec) {
        qWarning() << "Could not find encoder for codecId" << codecID;
        return false;
    }

    qCDebug(qLcVideoFrameEncoder) << "found encoder" << m_codec->name << "for id" << m_codec->id;

#ifdef Q_OS_WINDOWS
    // TODO: investigate, there might be more encoders not supporting odd resolution
    if (strcmp(m_codec->name, "h264_mf") == 0) {
        auto makeEven = [](int size) { return size & ~1; };
        const QSize fixedResolution(makeEven(resolution.width()), makeEven(resolution.height()));
        if (fixedResolution != resolution) {
            qCDebug(qLcVideoFrameEncoder) << "Fix odd video resolution for codec" << m_codec->name
                                          << ":" << resolution << "->" << fixedResolution;
            m_settings.setVideoResolution(fixedResolution);
        }
    }
#endif

    auto fixedResolution = adjustVideoResolution(m_codec, m_settings.videoResolution());
    if (resolution != fixedResolution) {
        qCDebug(qLcVideoFrameEncoder) << "Fix odd video resolution for codec" << m_codec->name
                                      << ":" << resolution << "->" << fixedResolution;

        m_settings.setVideoResolution(fixedResolution);
    }

    if (m_codec->supported_framerates && qLcVideoFrameEncoder().isEnabled(QtDebugMsg))
        for (auto rate = m_codec->supported_framerates; rate->num && rate->den; ++rate)
            qCDebug(qLcVideoFrameEncoder) << "supported frame rate:" << *rate;

    m_codecFrameRate = adjustFrameRate(m_codec->supported_framerates, m_settings.videoFrameRate());
    qCDebug(qLcVideoFrameEncoder) << "Adjusted frame rate:" << m_codecFrameRate;

    return true;
}

bool VideoFrameEncoder::initTargetFormats()
{
    m_targetFormat = findTargetFormat(m_sourceFormat, m_sourceSWFormat, m_codec, m_accel.get());

    if (m_targetFormat == AV_PIX_FMT_NONE) {
        qWarning() << "Could not find target format for codecId" << m_codec->id;
        return false;
    }

    if (isHwPixelFormat(m_targetFormat)) {
        Q_ASSERT(m_accel);

        m_targetSWFormat = findTargetSWFormat(m_sourceSWFormat, m_codec, *m_accel);

        if (m_targetSWFormat == AV_PIX_FMT_NONE) {
            qWarning() << "Cannot find software target format. sourceSWFormat:" << m_sourceSWFormat
                       << "targetFormat:" << m_targetFormat;
            return false;
        }

        m_accel->createFramesContext(m_targetSWFormat, m_settings.videoResolution());
        if (!m_accel->hwFramesContextAsBuffer())
            return false;
    } else {
        m_targetSWFormat = m_targetFormat;
    }

    return true;
}

VideoFrameEncoder::~VideoFrameEncoder() = default;

bool QFFmpeg::VideoFrameEncoder::initCodecContext(AVFormatContext *formatContext)
{
    m_stream = avformat_new_stream(formatContext, nullptr);
    m_stream->id = formatContext->nb_streams - 1;
    //qCDebug(qLcVideoFrameEncoder) << "Video stream: index" << d->stream->id;
    m_stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    m_stream->codecpar->codec_id = m_codec->id;

    // Apples HEVC decoders don't like the hev1 tag ffmpeg uses by default, use hvc1 as the more commonly accepted tag
    if (m_codec->id == AV_CODEC_ID_HEVC)
        m_stream->codecpar->codec_tag = MKTAG('h', 'v', 'c', '1');

    const auto resolution = m_settings.videoResolution();

    // ### Fix hardcoded values
    m_stream->codecpar->format = m_targetFormat;
    m_stream->codecpar->width = resolution.width();
    m_stream->codecpar->height = resolution.height();
    m_stream->codecpar->sample_aspect_ratio = AVRational{ 1, 1 };

    Q_ASSERT(m_codec);

    m_stream->time_base = adjustFrameTimeBase(m_codec->supported_framerates, m_codecFrameRate);
    m_codecContext.reset(avcodec_alloc_context3(m_codec));
    if (!m_codecContext) {
        qWarning() << "Could not allocate codec context";
        return false;
    }

    avcodec_parameters_to_context(m_codecContext.get(), m_stream->codecpar);
    m_codecContext->time_base = m_stream->time_base;
    qCDebug(qLcVideoFrameEncoder) << "codecContext time base" << m_codecContext->time_base.num
                                  << m_codecContext->time_base.den;

    m_codecContext->framerate = m_codecFrameRate;
    m_codecContext->pix_fmt = m_targetFormat;
    m_codecContext->width = resolution.width();
    m_codecContext->height = resolution.height();

    if (m_accel) {
        auto deviceContext = m_accel->hwDeviceContextAsBuffer();
        Q_ASSERT(deviceContext);
        m_codecContext->hw_device_ctx = av_buffer_ref(deviceContext);

        if (auto framesContext = m_accel->hwFramesContextAsBuffer())
            m_codecContext->hw_frames_ctx = av_buffer_ref(framesContext);
    }

    return true;
}

bool VideoFrameEncoder::open()
{
    if (!m_codecContext)
        return false;

    AVDictionaryHolder opts;
    applyVideoEncoderOptions(m_settings, m_codec->name, m_codecContext.get(), opts);
    int res = avcodec_open2(m_codecContext.get(), m_codec, opts);
    if (res < 0) {
        m_codecContext.reset();
        qWarning() << "Couldn't open codec for writing" << err2str(res);
        return false;
    }
    qCDebug(qLcVideoFrameEncoder) << "video codec opened" << res << "time base"
                                  << m_codecContext->time_base;
    return true;
}

qint64 VideoFrameEncoder::getPts(qint64 us) const
{
    qint64 div = 1'000'000 * m_stream->time_base.num;
    return div != 0 ? (us * m_stream->time_base.den + div / 2) / div : 0;
}

const AVRational &VideoFrameEncoder::getTimeBase() const
{
    return m_stream->time_base;
}

int VideoFrameEncoder::sendFrame(AVFrameUPtr frame)
{
    if (!m_codecContext) {
        qWarning() << "codec context is not initialized!";
        return AVERROR(EINVAL);
    }

    if (!frame)
        return avcodec_send_frame(m_codecContext.get(), frame.get());

    if (frame->format != m_sourceFormat) {
        qWarning() << "Frame format has changed:" << m_sourceFormat << "->" << frame->format;
        return AVERROR(EINVAL);
    }

    const QSize frameSize(frame->width, frame->height);
    if (frameSize != m_sourceSize) {
        qCDebug(qLcVideoFrameEncoder) << "Update conversions on the fly. Source size"
                                      << m_sourceSize << "->" << frameSize;
        m_sourceSize = frameSize;
        updateConversions();
    }

    int64_t pts = 0;
    AVRational timeBase = {};
    getAVFrameTime(*frame, pts, timeBase);

    if (m_downloadFromHW) {
        auto f = makeAVFrame();

        int err = av_hwframe_transfer_data(f.get(), frame.get(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder) << "Error transferring frame data to surface." << err2str(err);
            return err;
        }

        frame = std::move(f);
    }

    if (m_converter) {
        auto f = makeAVFrame();

        f->format = m_targetSWFormat;
        f->width = m_settings.videoResolution().width();
        f->height = m_settings.videoResolution().height();

        av_frame_get_buffer(f.get(), 0);
        const auto scaledHeight = sws_scale(m_converter.get(), frame->data, frame->linesize, 0,
                                            frame->height, f->data, f->linesize);

        if (scaledHeight != f->height)
            qCWarning(qLcVideoFrameEncoder) << "Scaled height" << scaledHeight << "!=" << f->height;

        frame = std::move(f);
    }

    if (m_uploadToHW) {
        auto *hwFramesContext = m_accel->hwFramesContextAsBuffer();
        Q_ASSERT(hwFramesContext);
        auto f = makeAVFrame();

        if (!f)
            return AVERROR(ENOMEM);
        int err = av_hwframe_get_buffer(hwFramesContext, f.get(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder) << "Error getting HW buffer" << err2str(err);
            return err;
        } else {
            qCDebug(qLcVideoFrameEncoder) << "got HW buffer";
        }
        if (!f->hw_frames_ctx) {
            qCDebug(qLcVideoFrameEncoder) << "no hw frames context";
            return AVERROR(ENOMEM);
        }
        err = av_hwframe_transfer_data(f.get(), frame.get(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder) << "Error transferring frame data to surface." << err2str(err);
            return err;
        }
        frame = std::move(f);
    }

    qCDebug(qLcVideoFrameEncoder) << "sending frame" << pts << "*" << timeBase;

    setAVFrameTime(*frame, pts, timeBase);
    return avcodec_send_frame(m_codecContext.get(), frame.get());
}

AVPacketUPtr VideoFrameEncoder::retrievePacket()
{
    if (!m_codecContext)
        return nullptr;

    auto getPacket = [&]() {
        AVPacketUPtr packet(av_packet_alloc());
        const int ret = avcodec_receive_packet(m_codecContext.get(), packet.get());
        if (ret < 0) {
            if (ret != AVERROR(EOF) && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
                qCDebug(qLcVideoFrameEncoder) << "Error receiving packet" << ret << err2str(ret);
            return AVPacketUPtr{};
        }
        auto ts = timeStampMs(packet->pts, m_stream->time_base);

        qCDebug(qLcVideoFrameEncoder)
                << "got a packet" << packet->pts << packet->dts << (ts ? *ts : 0);

        packet->stream_index = m_stream->id;
        return packet;
    };

    auto fixPacketDts = [&](AVPacket &packet) {
        // Workaround for some ffmpeg codecs bugs (e.g. nvenc)
        // Ideally, packet->pts < packet->dts is not expected

        if (packet.dts == AV_NOPTS_VALUE)
            return true;

        packet.dts -= m_packetDtsOffset;

        if (packet.pts != AV_NOPTS_VALUE && packet.pts < packet.dts) {
            m_packetDtsOffset += packet.dts - packet.pts;
            packet.dts = packet.pts;

            if (m_prevPacketDts != AV_NOPTS_VALUE && packet.dts < m_prevPacketDts) {
                qCWarning(qLcVideoFrameEncoder)
                        << "Skip packet; failed to fix dts:" << packet.dts << m_prevPacketDts;
                return false;
            }
        }

        m_prevPacketDts = packet.dts;

        return true;
    };

    while (auto packet = getPacket()) {
        if (fixPacketDts(*packet))
            return packet;
    }

    return nullptr;
}

void VideoFrameEncoder::updateConversions()
{
    const bool needToScale = m_sourceSize != m_settings.videoResolution();
    const bool zeroCopy = m_sourceFormat == m_targetFormat && !needToScale;

    m_converter.reset();

    if (zeroCopy) {
        m_downloadFromHW = false;
        m_uploadToHW = false;

        qCDebug(qLcVideoFrameEncoder) << "zero copy encoding, format" << m_targetFormat;
        // no need to initialize any converters
        return;
    }

    m_downloadFromHW = m_sourceFormat != m_sourceSWFormat;
    m_uploadToHW = m_targetFormat != m_targetSWFormat;

    if (m_sourceSWFormat != m_targetSWFormat || needToScale) {
        const auto targetSize = m_settings.videoResolution();
        qCDebug(qLcVideoFrameEncoder)
                << "video source and encoder use different formats:" << m_sourceSWFormat
                << m_targetSWFormat << "or sizes:" << m_sourceSize << targetSize;

        m_converter.reset(sws_getContext(m_sourceSize.width(), m_sourceSize.height(),
                                         m_sourceSWFormat, targetSize.width(), targetSize.height(),
                                         m_targetSWFormat, SWS_FAST_BILINEAR, nullptr, nullptr,
                                         nullptr));
    }

    qCDebug(qLcVideoFrameEncoder) << "VideoFrameEncoder conversions initialized:"
                                  << "sourceFormat:" << m_sourceFormat
                                  << (isHwPixelFormat(m_sourceFormat) ? "(hw)" : "(sw)")
                                  << "targetFormat:" << m_targetFormat
                                  << (isHwPixelFormat(m_targetFormat) ? "(hw)" : "(sw)")
                                  << "sourceSWFormat:" << m_sourceSWFormat
                                  << "targetSWFormat:" << m_targetSWFormat
                                  << "converter:" << m_converter.get();
}

} // namespace QFFmpeg

QT_END_NAMESPACE
