/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qffmpegvideoframeencoder_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegencoderoptions_p.h"
#include "private/qplatformmediarecorder_p.h"
#include "private/qmultimediautils_p.h"
#include <qdebug.h>

extern "C" {
#include <libavutil/pixdesc.h>
}

/* Infrastructure for HW acceleration goes into this file. */

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

VideoFrameEncoder::Data::~Data()
{
    if (converter)
        sws_freeContext(converter);
    avcodec_free_context(&codecContext);
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

    const auto *accels = HWAccel::preferredDeviceTypes();
    while (*accels != AV_HWDEVICE_TYPE_NONE) {
        auto accel = HWAccel(*accels);
        ++accels;

        auto matchesSizeConstraints = [&]() -> bool {
            auto *constraints = av_hwdevice_get_hwframe_constraints(accel.hwDeviceContextAsBuffer(), nullptr);
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

        d->codec = accel.hardwareEncoderForCodecId(codecID);
        if (!d->codec)
            continue;
        d->accel = accel;
        break;
    }

    if (d->accel.isNull()) {
        d->codec = avcodec_find_encoder(codecID);
        if (!d->codec) {
            qWarning() << "Could not find encoder for codecId" << codecID;
            d = {};
            return;
        }
    }

    auto supportsFormat = [&](AVPixelFormat fmt) {
        auto *f = d->codec->pix_fmts;
        while (*f != -1) {
            if (*f == fmt)
                return true;
            ++f;
        }
        return false;
    };

    d->targetFormat = d->sourceFormat;

    if (!supportsFormat(d->sourceFormat)) {
        if (supportsFormat(swFormat))
            d->targetFormat = swFormat;
        else
            // Take first format the encoder supports. Might want to improve upon this
            d->targetFormat = *d->codec->pix_fmts;
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
        Q_ASSERT(!d->accel.isNull());
        // if source and target formats don't agree, but the target is a HW format, we need to upload
        if (d->sourceFormat != d->targetFormat || needToScale) {
            d->uploadToHW = true;
            // need to create a frames context to convert the input data
            d->accel.createFramesContext(sourceFormat, sourceSize);

            AVPixelFormat *formats;
            int res = av_hwframe_transfer_get_formats(d->accel.hwFramesContextAsBuffer(), AV_HWFRAME_TRANSFER_DIRECTION_TO,
                                                      &formats, 0);
            Q_UNUSED(res);
            Q_ASSERT(res == 0);

            // determine SW pixel format of target
            auto *f = formats;
            while (*f != AV_PIX_FMT_NONE) {
                if (*f == d->sourceSWFormat) {
                    d->targetSWFormat = *f;
                    break;
                }
                if (d->targetSWFormat == AV_PIX_FMT_NONE && !(av_pix_fmt_desc_get(*f)->flags & AV_PIX_FMT_FLAG_HWACCEL))
                    d->targetSWFormat = *f;
            }
            av_free(formats);
        }
    } else {
        d->targetSWFormat = d->targetFormat;
    }

    if (d->sourceSWFormat != d->targetSWFormat || needToScale) {
        auto resolution = d->settings.videoResolution();
        qDebug() << "camera and encoder use different formats:" << d->sourceSWFormat << d->targetSWFormat;
        d->converter = sws_getContext(d->sourceSize.width(), d->sourceSize.height(), d->sourceSWFormat,
                                   resolution.width(), resolution.height(), d->targetSWFormat,
                                   SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
    }
}

VideoFrameEncoder::~VideoFrameEncoder()
{
}

void QFFmpeg::VideoFrameEncoder::initWithFormatContext(AVFormatContext *formatContext)
{
    d->stream = avformat_new_stream(formatContext, nullptr);
    d->stream->id = formatContext->nb_streams - 1;
    //qDebug() << "Video stream: index" << d->stream->id;
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
        auto *f = d->codec->supported_framerates;
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
        d->stream->time_base = { best->den, best->num };
        requestedRate = float(best->num)/float(best->den);
    }

    Q_ASSERT(d->codec);
    d->codecContext = avcodec_alloc_context3(d->codec);
    if (!d->codecContext) {
        qWarning() << "Could not allocate codec context";
        d = {};
        return;
    }

    avcodec_parameters_to_context(d->codecContext, d->stream->codecpar);
    d->codecContext->time_base = d->stream->time_base;
    int num, den;
    qt_real_to_fraction(requestedRate, &num, &den);
    d->codecContext->framerate = { num, den };

    AVDictionary *opts = nullptr;
    applyVideoEncoderOptions(d->settings, d->codec->name, d->codecContext, &opts);
    int res = avcodec_open2(d->codecContext, d->codec, &opts);
    if (res < 0) {
        avcodec_free_context(&d->codecContext);
        qWarning() << "Couldn't open codec for writing";
        d = {};
        return;
    }
    //    qDebug() << "video codec opened" << res << codec->time_base.num << codec->time_base.den;
    d->stream->time_base = d->codecContext->time_base;
}

qint64 VideoFrameEncoder::getPts(qint64 ms)
{
    Q_ASSERT(d);
    return (ms*d->stream->time_base.den + (d->stream->time_base.num >> 1))/(1000*d->stream->time_base.num);
}

int VideoFrameEncoder::sendFrame(AVFrame *frame)
{
    if (!frame)
        return avcodec_send_frame(d->codecContext, frame);
    auto pts = frame->pts;

    if (d->downloadFromHW) {
        auto *f = av_frame_alloc();
        f->format = d->sourceSWFormat;
        int err = av_hwframe_transfer_data(f, frame, 0);
        if (err < 0) {
            qDebug() << "Error transferring frame data to surface." << av_err2str(err);
            return err;
        }
        av_frame_free(&frame);
        frame = f;
    }

    if (d->converter) {
        auto *f = av_frame_alloc();
        f->format = d->targetSWFormat;
        f->width = d->settings.videoResolution().width();
        f->height = d->settings.videoResolution().height();
        av_frame_get_buffer(f, 0);
        sws_scale(d->converter, frame->data, frame->linesize, 0, f->height, f->data, f->linesize);
        av_frame_free(&frame);
        frame = f;
    }

    if (d->uploadToHW) {
        auto *hwFramesContext = d->accel.hwFramesContextAsBuffer();
        Q_ASSERT(hwFramesContext);
        auto *f = av_frame_alloc();
        if (!f)
            return AVERROR(ENOMEM);
        int err = av_hwframe_get_buffer(hwFramesContext, f, 0);
        if (err < 0) {
            qDebug() << "Error getting HW buffer" << av_err2str(err);
            return err;
        }
        if (!f->hw_frames_ctx)
            return AVERROR(ENOMEM);
        err = av_hwframe_transfer_data(f, frame, 0);
        if (err < 0) {
            qDebug() << "Error transferring frame data to surface." << av_err2str(err);
            return err;
        }
        av_frame_free(&frame);
        frame = f;
    } else

    frame->pts = pts;
    return avcodec_send_frame(d->codecContext, frame);
}

AVPacket *VideoFrameEncoder::retrievePacket()
{
    if (!d)
        return nullptr;
    AVPacket *packet = av_packet_alloc();
    int ret = avcodec_receive_packet(d->codecContext, packet);
    if (ret < 0) {
        av_packet_unref(packet);
        if (ret != AVERROR(EOF) && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF)
            qDebug() << "Error receiving packet" << ret << av_err2str(ret);
        return nullptr;
    }
    packet->stream_index = d->stream->id;
    return packet;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
