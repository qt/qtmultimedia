// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoframeencoder_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegencoderoptions_p.h"
#include "private/qplatformmediarecorder_p.h"
#include "private/qmultimediautils_p.h"
#include <qloggingcategory.h>

extern "C" {
#include <libavutil/pixdesc.h>
}

/* Infrastructure for HW acceleration goes into this file. */

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcVideoFrameEncoder, "qt.multimedia.ffmpeg.videoencoder")

namespace QFFmpeg {

VideoFrameEncoder::Data::~Data()
{
    if (converter)
        sws_freeContext(converter);
}

VideoFrameEncoder::VideoFrameEncoder(const QMediaEncoderSettings &encoderSettings,
                                     const QSize &sourceSize, float frameRate, AVPixelFormat sourceFormat, AVPixelFormat swFormat)
    : d(new Data)
{
    d->settings = encoderSettings;
    d->frameRate = frameRate;
    d->sourceSize = sourceSize;

    if (!d->settings.videoResolution().isValid())
        d->settings.setVideoResolution(d->sourceSize);

    d->sourceFormat = sourceFormat;
    d->sourceSWFormat = swFormat;

    auto qVideoCodec = encoderSettings.videoCodec();
    auto codecID = QFFmpegMediaFormatInfo::codecIdForVideoCodec(qVideoCodec);

#ifndef QT_DISABLE_HW_ENCODING
    auto [preferredTypes, size] = HWAccel::preferredDeviceTypes();
    for (qsizetype i = 0; i < size; i++) {
        auto accel = HWAccel::create(preferredTypes[i]);
        if (!accel)
            continue;

        auto matchesSizeConstraints = [&]() -> bool {
            auto *constraints = av_hwdevice_get_hwframe_constraints(accel->hwDeviceContextAsBuffer(), nullptr);
            if (!constraints)
                return true;
                // Check size constraints
            bool result = (d->sourceSize.width() >= constraints->min_width && d->sourceSize.height() >= constraints->min_height &&
                           d->sourceSize.width() <= constraints->max_width && d->sourceSize.height() <= constraints->max_height);
            av_hwframe_constraints_free(&constraints);
            return result;
        };

        if (!matchesSizeConstraints())
            continue;

        d->codec = accel->hardwareEncoderForCodecId(codecID);
        if (!d->codec)
            continue;
        d->accel = std::move(accel);
        break;
    }
#endif

    if (!d->accel) {
        d->codec = avcodec_find_encoder(codecID);
        if (!d->codec) {
            qWarning() << "Could not find encoder for codecId" << codecID;
            d = {};
            return;
        }
    }
    auto supportsFormat = [&](AVPixelFormat fmt) {
        if (auto fmts = d->codec->pix_fmts) {
            for (; *fmts != AV_PIX_FMT_NONE; ++fmts)
                if (*fmts == fmt)
                    return true;
        }
        return false;
    };

    d->targetFormat = d->sourceFormat;

    if (!supportsFormat(d->sourceFormat)) {
        if (supportsFormat(swFormat))
            d->targetFormat = swFormat;
        else if (d->codec->pix_fmts)
            // Take first format the encoder supports. Might want to improve upon this
            d->targetFormat = *d->codec->pix_fmts;
        else
            qWarning() << "Cannot set target format";
    }

    auto desc = av_pix_fmt_desc_get(d->sourceFormat);
    d->sourceFormatIsHWFormat = desc->flags & AV_PIX_FMT_FLAG_HWACCEL;
    desc = av_pix_fmt_desc_get(d->targetFormat);
    d->targetFormatIsHWFormat = desc->flags & AV_PIX_FMT_FLAG_HWACCEL;

    bool needToScale = d->sourceSize != d->settings.videoResolution();
    bool zeroCopy = d->sourceFormatIsHWFormat && d->sourceFormat == d->targetFormat && !needToScale;

    if (zeroCopy)
        // no need to initialize any converters
        return;

    if (d->sourceFormatIsHWFormat) {
        // if source and target formats don't agree, but the source is a HW format or sizes do't agree, we need to download
        if (d->sourceFormat != d->targetFormat || needToScale)
            d->downloadFromHW = true;
    } else {
        d->sourceSWFormat = d->sourceFormat;
    }

    if (d->targetFormatIsHWFormat) {
        Q_ASSERT(d->accel);
        // if source and target formats don't agree, but the target is a HW format, we need to upload
        if (d->sourceFormat != d->targetFormat || needToScale) {
            d->uploadToHW = true;

            // determine the format used by the encoder.
            // We prefer YUV422 based formats such as NV12 or P010. Selection trues to find the best matching
            // format for the encoder depending on the bit depth of the source format
            auto desc = av_pix_fmt_desc_get(d->sourceSWFormat);
            int sourceDepth = desc->comp[0].depth;

            d->targetSWFormat = AV_PIX_FMT_NONE;

            auto *constraints = av_hwdevice_get_hwframe_constraints(d->accel->hwDeviceContextAsBuffer(), nullptr);
            auto *f = constraints->valid_sw_formats;
            int score = INT_MIN;
            while (*f != AV_PIX_FMT_NONE) {
                auto calcScore = [&](AVPixelFormat fmt) -> int {
                    auto *desc = av_pix_fmt_desc_get(fmt);
                    int s = 0;
                    if (fmt == d->sourceSWFormat)
                        // prefer exact matches
                        s += 10;
                    if (desc->comp[0].depth == sourceDepth)
                        s += 100;
                    else if (desc->comp[0].depth < sourceDepth)
                        s -= 100 + (sourceDepth - desc->comp[0].depth);
                    if (desc->log2_chroma_h == 1)
                        s += 1;
                    if (desc->log2_chroma_w == 1)
                        s += 1;
                    if (desc->flags & AV_PIX_FMT_FLAG_BE)
                        s -= 10;
                    if (desc->flags & AV_PIX_FMT_FLAG_PAL)
                        // we don't want paletted formats
                        s -= 10000;
                    if (desc->flags & AV_PIX_FMT_FLAG_RGB)
                        // we don't want RGB formats
                        s -= 1000;
                    if (desc->flags & AV_PIX_FMT_FLAG_HWACCEL)
                        // we really don't want HW accelerated formats here
                        s -= 1000000;
                    qCDebug(qLcVideoFrameEncoder) << "checking format" << fmt << Qt::hex << desc->flags << desc->comp[0].depth
                             << desc->log2_chroma_h << desc->log2_chroma_w << "score:" << s;
                    return s;
                };

                int s = calcScore(*f);
                if (s > score) {
                    d->targetSWFormat = *f;
                    score = s;
                }
                ++f;
            }
            if (d->targetSWFormat == AV_PIX_FMT_NONE) // shouldn't happen
                d->targetSWFormat = *constraints->valid_sw_formats;

            qCDebug(qLcVideoFrameEncoder) << "using format" << d->targetSWFormat << "as transfer format.";

            av_hwframe_constraints_free(&constraints);
            // need to create a frames context to convert the input data
            d->accel->createFramesContext(d->targetSWFormat, d->settings.videoResolution());
        }
    } else {
        d->targetSWFormat = d->targetFormat;
    }

    if (d->sourceSWFormat != d->targetSWFormat || needToScale) {
        auto resolution = d->settings.videoResolution();
        qCDebug(qLcVideoFrameEncoder) << "camera and encoder use different formats:" << d->sourceSWFormat << d->targetSWFormat;

        d->converter = sws_getContext(d->sourceSize.width(), d->sourceSize.height(), d->sourceSWFormat,
                                   resolution.width(), resolution.height(), d->targetSWFormat,
                                   SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }

    qCDebug(qLcVideoFrameEncoder) << "VideoFrameEncoder conversions initialized:"
                                  << "sourceFormat:" << d->sourceFormat
                                  << (d->sourceFormatIsHWFormat ? "(hw)" : "(sw)")
                                  << "targetFormat:" << d->targetFormat
                                  << (d->targetFormatIsHWFormat ? "(hw)" : "(sw)")
                                  << "sourceSWFormat:" << d->sourceSWFormat
                                  << "targetSWFormat:" << d->targetSWFormat;
}

VideoFrameEncoder::~VideoFrameEncoder()
{
}

void QFFmpeg::VideoFrameEncoder::initWithFormatContext(AVFormatContext *formatContext)
{
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
    d->stream->time_base = AVRational{ 1, (int)(requestedRate*1000) };

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
    AVDictionary *opts = nullptr;
    applyVideoEncoderOptions(d->settings, d->codec->name, d->codecContext.get(), &opts);
    int res = avcodec_open2(d->codecContext.get(), d->codec, &opts);
    if (res < 0) {
        d->codecContext.reset();
        qWarning() << "Couldn't open codec for writing" << err2str(res);
        return false;
    }
    qCDebug(qLcVideoFrameEncoder) << "video codec opened" << res << "time base" << d->codecContext->time_base.num << d->codecContext->time_base.den;
    d->stream->time_base = d->codecContext->time_base;
    return true;
}

qint64 VideoFrameEncoder::getPts(qint64 us)
{
    Q_ASSERT(d);
    qint64 div = 1'000'000 * d->stream->time_base.num;
    return div != 0 ? (us * d->stream->time_base.den + div / 2) / div : 0;
}

int VideoFrameEncoder::sendFrame(AVFrameUPtr frame)
{
    if (!d->codecContext) {
        qWarning() << "codec context is not initialized!";
        return AVERROR(EINVAL);
    }

    if (!frame)
        return avcodec_send_frame(d->codecContext.get(), frame.get());
    auto pts = frame->pts;

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

    qCDebug(qLcVideoFrameEncoder) << "sending frame" << pts;
    frame->pts = pts;
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
    qCDebug(qLcVideoFrameEncoder) << "got a packet" << packet->pts << (ts ? *ts : 0);
    packet->stream_index = d->stream->id;
    return packet;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
