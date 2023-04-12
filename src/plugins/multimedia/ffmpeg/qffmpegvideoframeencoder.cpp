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

static Q_LOGGING_CATEGORY(qLcVideoFrameEncoder, "qt.multimedia.ffmpeg.videoencoder")

namespace QFFmpeg {

VideoFrameEncoder::Data::~Data()
{
    if (converter)
        sws_freeContext(converter);
}

VideoFrameEncoder::VideoFrameEncoder(const QMediaEncoderSettings &encoderSettings,
                                     const QSize &sourceSize, float frameRate,
                                     AVPixelFormat sourceFormat, AVPixelFormat sourceSWFormat)
    : d(new Data)
{
    d->settings = encoderSettings;
    d->frameRate = frameRate;
    d->sourceSize = sourceSize;

    if (!d->settings.videoResolution().isValid())
        d->settings.setVideoResolution(d->sourceSize);

    d->sourceFormat = sourceFormat;
    d->sourceSWFormat = sourceSWFormat;

    Q_ASSERT(isSwPixelFormat(sourceSWFormat));

    const auto qVideoCodec = encoderSettings.videoCodec();
    const auto codecID = QFFmpegMediaFormatInfo::codecIdForVideoCodec(qVideoCodec);

    std::tie(d->codec, d->accel) = findHwEncoder(codecID, sourceSize);

    if (!d->codec)
        d->codec = findSwEncoder(codecID, sourceFormat, sourceSWFormat);

    if (!d->codec) {
        qWarning() << "Could not find encoder for codecId" << codecID;
        d = {};
        return;
    }

    qCDebug(qLcVideoFrameEncoder) << "found encoder" << d->codec->name << "for id" << d->codec->id;

    d->targetFormat = findTargetFormat(sourceFormat, sourceSWFormat, d->codec, d->accel.get());

    if (d->targetFormat == AV_PIX_FMT_NONE) {
        qWarning() << "Could not find target format for codecId" << codecID;
        d = {};
        return;
    }

    const bool needToScale = d->sourceSize != d->settings.videoResolution();
    const bool zeroCopy = d->sourceFormat == d->targetFormat && !needToScale;

    if (zeroCopy) {
        qCDebug(qLcVideoFrameEncoder) << "zero copy encoding, format" << d->targetFormat;
        // no need to initialize any converters
        return;
    }

    if (isHwPixelFormat(d->sourceFormat))
        d->downloadFromHW = true;
    else
        d->sourceSWFormat = d->sourceFormat;

    if (isHwPixelFormat(d->targetFormat)) {
        Q_ASSERT(d->accel);
        // if source and target formats don't agree, but the target is a HW format, we need to upload
        d->uploadToHW = true;
        d->targetSWFormat = findTargetSWFormat(d->sourceSWFormat, *d->accel);

        if (d->targetSWFormat == AV_PIX_FMT_NONE) {
            qWarning() << "Cannot find software target format. sourceSWFormat:" << d->sourceSWFormat
                       << "targetFormat:" << d->targetFormat;
            d = {};
            return;
        }

        qCDebug(qLcVideoFrameEncoder)
                << "using sw format" << d->targetSWFormat << "as transfer format.";

        // need to create a frames context to convert the input data
        d->accel->createFramesContext(d->targetSWFormat, d->settings.videoResolution());
    } else {
        d->targetSWFormat = d->targetFormat;
    }

    if (d->sourceSWFormat != d->targetSWFormat || needToScale) {
        const auto targetSize = d->settings.videoResolution();
        qCDebug(qLcVideoFrameEncoder)
                << "camera and encoder use different formats:" << d->sourceSWFormat
                << d->targetSWFormat << "or sizes:" << d->sourceSize << targetSize;

        d->converter =
                sws_getContext(d->sourceSize.width(), d->sourceSize.height(), d->sourceSWFormat,
                               targetSize.width(), targetSize.height(), d->targetSWFormat,
                               SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }

    qCDebug(qLcVideoFrameEncoder) << "VideoFrameEncoder conversions initialized:"
                                  << "sourceFormat:" << d->sourceFormat
                                  << (isHwPixelFormat(d->sourceFormat) ? "(hw)" : "(sw)")
                                  << "targetFormat:" << d->targetFormat
                                  << (isHwPixelFormat(d->targetFormat) ? "(hw)" : "(sw)")
                                  << "sourceSWFormat:" << d->sourceSWFormat
                                  << "targetSWFormat:" << d->targetSWFormat;
}

VideoFrameEncoder::~VideoFrameEncoder()
{
}

void QFFmpeg::VideoFrameEncoder::initWithFormatContext(AVFormatContext *formatContext)
{
    if (!d)
        return;

    d->stream = avformat_new_stream(formatContext, nullptr);
    d->stream->id = formatContext->nb_streams - 1;
    //qCDebug(qLcVideoFrameEncoder) << "Video stream: index" << d->stream->id;
    d->stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    d->stream->codecpar->codec_id = d->codec->id;

    // Apples HEVC decoders don't like the hev1 tag ffmpeg uses by default, use hvc1 as the more commonly accepted tag
    if (d->codec->id == AV_CODEC_ID_HEVC)
        d->stream->codecpar->codec_tag = MKTAG('h','v','c','1');

    // ### Fix hardcoded values
    d->stream->codecpar->format = d->targetFormat;
    d->stream->codecpar->width = d->settings.videoResolution().width();
    d->stream->codecpar->height = d->settings.videoResolution().height();
    d->stream->codecpar->sample_aspect_ratio = AVRational{1, 1};
    float requestedRate = d->frameRate;
    constexpr int TimeScaleFactor = 1000; // Allows not to follow fixed rate
    d->stream->time_base = AVRational{ 1, static_cast<int>(requestedRate * TimeScaleFactor) };

    float delta = 1e10;
    if (d->codec->supported_framerates) {
        // codec only supports fixed frame rates
        auto *best = d->codec->supported_framerates;
        qCDebug(qLcVideoFrameEncoder) << "Finding fixed rate:";
        for (auto *f = d->codec->supported_framerates; f->num != 0; f++) {
            auto maybeRate = toFloat(*f);
            if (!maybeRate)
                continue;
            float d = qAbs(*maybeRate - requestedRate);
            qCDebug(qLcVideoFrameEncoder) << "    " << f->num << f->den << d;
            if (d < delta) {
                best = f;
                delta = d;
            }
        }
        qCDebug(qLcVideoFrameEncoder) << "Fixed frame rate required. Requested:" << requestedRate << "Using:" << best->num << "/" << best->den;
        d->stream->time_base = *best;
        requestedRate = toFloat(*best).value_or(0.f);
    }

    Q_ASSERT(d->codec);
    d->codecContext.reset(avcodec_alloc_context3(d->codec));
    if (!d->codecContext) {
        qWarning() << "Could not allocate codec context";
        d = {};
        return;
    }

    avcodec_parameters_to_context(d->codecContext.get(), d->stream->codecpar);
    d->codecContext->time_base = d->stream->time_base;
    qCDebug(qLcVideoFrameEncoder) << "requesting time base" << d->codecContext->time_base.num << d->codecContext->time_base.den;
    auto [num, den] = qRealToFraction(requestedRate);
    d->codecContext->framerate = { num, den };
    if (d->accel) {
        auto deviceContext = d->accel->hwDeviceContextAsBuffer();
        if (deviceContext)
            d->codecContext->hw_device_ctx = av_buffer_ref(deviceContext);
        auto framesContext = d->accel->hwFramesContextAsBuffer();
        if (framesContext)
            d->codecContext->hw_frames_ctx = av_buffer_ref(framesContext);
    }
}

bool VideoFrameEncoder::open()
{
    if (!d) {
        qWarning() << "Cannot open null VideoFrameEncoder";
        return false;
    }
    AVDictionaryHolder opts;
    applyVideoEncoderOptions(d->settings, d->codec->name, d->codecContext.get(), opts);
    int res = avcodec_open2(d->codecContext.get(), d->codec, opts);
    if (res < 0) {
        d->codecContext.reset();
        qWarning() << "Couldn't open codec for writing" << err2str(res);
        return false;
    }
    qCDebug(qLcVideoFrameEncoder) << "video codec opened" << res << "time base" << d->codecContext->time_base.num << d->codecContext->time_base.den;
    d->stream->time_base = d->stream->time_base;
    return true;
}

qint64 VideoFrameEncoder::getPts(qint64 us) const
{
    Q_ASSERT(d);
    qint64 div = 1'000'000 * d->stream->time_base.num;
    return div != 0 ? (us * d->stream->time_base.den + div / 2) / div : 0;
}

const AVRational &VideoFrameEncoder::getTimeBase() const
{
    return d->stream->time_base;
}

int VideoFrameEncoder::sendFrame(AVFrameUPtr frame)
{
    if (!d->codecContext) {
        qWarning() << "codec context is not initialized!";
        return AVERROR(EINVAL);
    }

    if (!frame)
        return avcodec_send_frame(d->codecContext.get(), frame.get());

    int64_t pts = 0;
    AVRational timeBase = {};
    getAVFrameTime(*frame, pts, timeBase);

    if (d->downloadFromHW) {
        auto f = makeAVFrame();

        int err = av_hwframe_transfer_data(f.get(), frame.get(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder) << "Error transferring frame data to surface." << err2str(err);
            return err;
        }

        frame = std::move(f);
    }

    if (d->converter) {
        auto f = makeAVFrame();

        f->format = d->targetSWFormat;
        f->width = d->settings.videoResolution().width();
        f->height = d->settings.videoResolution().height();

        av_frame_get_buffer(f.get(), 0);
        const auto scaledHeight = sws_scale(d->converter, frame->data, frame->linesize, 0,
                                            frame->height, f->data, f->linesize);

        if (scaledHeight != f->height)
            qCWarning(qLcVideoFrameEncoder) << "Scaled height" << scaledHeight << "!=" << f->height;

        frame = std::move(f);
    }

    if (d->uploadToHW) {
        auto *hwFramesContext = d->accel->hwFramesContextAsBuffer();
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

    qCDebug(qLcVideoFrameEncoder) << "sending frame" << pts << "*" << timeBase.num << "/"
                                  << timeBase.den;

    setAVFrameTime(*frame, pts, timeBase);
    return avcodec_send_frame(d->codecContext.get(), frame.get());
}

AVPacket *VideoFrameEncoder::retrievePacket()
{
    if (!d || !d->codecContext)
        return nullptr;
    AVPacket *packet = av_packet_alloc();
    int ret = avcodec_receive_packet(d->codecContext.get(), packet);
    if (ret < 0) {
        av_packet_free(&packet);
        if (ret != AVERROR(EOF) && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
            qCDebug(qLcVideoFrameEncoder) << "Error receiving packet" << ret << err2str(ret);
        return nullptr;
    }
    auto ts = timeStampMs(packet->pts, d->stream->time_base);

    qCDebug(qLcVideoFrameEncoder) << "got a packet" << packet->pts << packet->dts << (ts ? *ts : 0);

    if (packet->dts != AV_NOPTS_VALUE && packet->pts < packet->dts) {
        // the case seems to be an ffmpeg bug
        packet->dts = AV_NOPTS_VALUE;
    }

    packet->stream_index = d->stream->id;
    return packet;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
