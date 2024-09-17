// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegvideoframeencoder_p.h"
#include "qffmpegmediaformatinfo_p.h"
#include "qffmpegencoderoptions_p.h"
#include "qffmpegvideoencoderutils_p.h"
#include "qffmpegcodecstorage_p.h"
#include <qloggingcategory.h>
#include <QtMultimedia/private/qmaybe_p.h>

extern "C" {
#include "libavutil/display.h"
}

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLcVideoFrameEncoder, "qt.multimedia.ffmpeg.videoencoder");

namespace QFFmpeg {

namespace {

AVCodecID avCodecID(const QMediaEncoderSettings &settings)
{
    const QMediaFormat::VideoCodec qVideoCodec = settings.videoCodec();
    return QFFmpegMediaFormatInfo::codecIdForVideoCodec(qVideoCodec);
}

} // namespace

VideoFrameEncoderUPtr VideoFrameEncoder::create(const QMediaEncoderSettings &encoderSettings,
                                                const SourceParams &sourceParams,
                                                AVFormatContext *formatContext)
{
    Q_ASSERT(isSwPixelFormat(sourceParams.swFormat));
    Q_ASSERT(isHwPixelFormat(sourceParams.format) || sourceParams.swFormat == sourceParams.format);

    AVStream *stream = createStream(sourceParams, formatContext);

    if (!stream)
        return nullptr;

    VideoFrameEncoderUPtr result;

    {
        const auto &deviceTypes = HWAccel::encodingDeviceTypes();

        auto findDeviceType = [&](const AVCodec *codec) {
            AVPixelFormat pixelFormat = findAVPixelFormat(codec, &isHwPixelFormat);
            if (pixelFormat == AV_PIX_FMT_NONE)
                return deviceTypes.end();

            return std::find_if(deviceTypes.begin(), deviceTypes.end(),
                                [pixelFormat](AVHWDeviceType deviceType) {
                                    return pixelFormatForHwDevice(deviceType) == pixelFormat;
                                });
        };

        findAndOpenAVEncoder(
                avCodecID(encoderSettings),
                [&](const AVCodec *codec) {
                    const auto found = findDeviceType(codec);
                    if (found == deviceTypes.end())
                        return NotSuitableAVScore;

                    return DefaultAVScore - static_cast<AVScore>(found - deviceTypes.begin());
                },
                [&](const AVCodec *codec) {
                    HWAccelUPtr hwAccel = HWAccel::create(*findDeviceType(codec));
                    if (!hwAccel)
                        return false;
                    if (!hwAccel->matchesSizeContraints(encoderSettings.videoResolution()))
                        return false;
                    result = create(stream, codec, std::move(hwAccel), sourceParams,
                                    encoderSettings);
                    return result != nullptr;
                });
    }

    if (!result) {
        findAndOpenAVEncoder(
                avCodecID(encoderSettings),
                [&](const AVCodec *codec) {
                    return findSWFormatScores(codec, sourceParams.swFormat);
                },
                [&](const AVCodec *codec) {
                    result = create(stream, codec, nullptr, sourceParams, encoderSettings);
                    return result != nullptr;
                });
    }

    if (result)
        qCDebug(qLcVideoFrameEncoder) << "found" << (result->m_accel ? "hw" : "sw") << "encoder"
                                      << result->m_codec->name << "for id" << result->m_codec->id;
    else
        qCWarning(qLcVideoFrameEncoder) << "No valid video codecs found";

    return result;
}

VideoFrameEncoder::VideoFrameEncoder(AVStream *stream, const AVCodec *codec, HWAccelUPtr hwAccel,
                                     const SourceParams &sourceParams,
                                     const QMediaEncoderSettings &encoderSettings)
    : m_settings(encoderSettings),
      m_stream(stream),
      m_codec(codec),
      m_accel(std::move(hwAccel)),
      m_sourceSize(sourceParams.size),
      m_sourceFormat(sourceParams.format),
      m_sourceSWFormat(sourceParams.swFormat)
{
}

AVStream *VideoFrameEncoder::createStream(const SourceParams &sourceParams,
                                          AVFormatContext *formatContext)
{
    AVStream *stream = avformat_new_stream(formatContext, nullptr);

    if (!stream)
        return stream;

    stream->id = formatContext->nb_streams - 1;
    stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;

    stream->codecpar->color_trc = sourceParams.colorTransfer;
    stream->codecpar->color_space = sourceParams.colorSpace;
    stream->codecpar->color_range = sourceParams.colorRange;

    if (sourceParams.transform.rotation != QtVideo::Rotation::None || sourceParams.transform.xMirrorredAfterRotation) {
        constexpr auto displayMatrixSize = sizeof(int32_t) * 9;
        AVPacketSideData sideData = { reinterpret_cast<uint8_t *>(av_malloc(displayMatrixSize)),
                                      displayMatrixSize, AV_PKT_DATA_DISPLAYMATRIX };
        int32_t *matrix = reinterpret_cast<int32_t *>(sideData.data);
        av_display_rotation_set(matrix, static_cast<double>(sourceParams.transform.rotation));
        if (sourceParams.transform.xMirrorredAfterRotation)
            av_display_matrix_flip(matrix, sourceParams.transform.xMirrorredAfterRotation, false);

        addStreamSideData(stream, sideData);
    }

    return stream;
}

VideoFrameEncoderUPtr VideoFrameEncoder::create(AVStream *stream, const AVCodec *codec,
                                                HWAccelUPtr hwAccel,
                                                const SourceParams &sourceParams,
                                                const QMediaEncoderSettings &encoderSettings)
{
    VideoFrameEncoderUPtr frameEncoder(new VideoFrameEncoder(stream, codec, std::move(hwAccel),
                                                             sourceParams, encoderSettings));
    frameEncoder->initTargetSize();

    frameEncoder->initCodecFrameRate();

    if (!frameEncoder->initTargetFormats())
        return nullptr;

    frameEncoder->initStream();

    if (!frameEncoder->initCodecContext())
        return nullptr;

    if (!frameEncoder->open())
        return nullptr;

    frameEncoder->updateConversions();
    return frameEncoder;
}

void VideoFrameEncoder::initTargetSize()
{
    m_targetSize = adjustVideoResolution(m_codec, m_settings.videoResolution());

#ifdef Q_OS_WINDOWS
    // TODO: investigate, there might be more encoders not supporting odd resolution
    if (strcmp(m_codec->name, "h264_mf") == 0) {
        auto makeEven = [](int size) { return size & ~1; };
        const QSize fixedSize(makeEven(m_targetSize.width()), makeEven(m_targetSize.height()));
        if (fixedSize != m_targetSize) {
            qCDebug(qLcVideoFrameEncoder) << "Fix odd video resolution for codec" << m_codec->name
                                          << ":" << m_targetSize << "->" << fixedSize;
            m_targetSize = fixedSize;
        }
    }
#endif
}

void VideoFrameEncoder::initCodecFrameRate()
{
    if (m_codec->supported_framerates && qLcVideoFrameEncoder().isEnabled(QtDebugMsg))
        for (auto rate = m_codec->supported_framerates; rate->num && rate->den; ++rate)
            qCDebug(qLcVideoFrameEncoder) << "supported frame rate:" << *rate;

    m_codecFrameRate = adjustFrameRate(m_codec->supported_framerates, m_settings.videoFrameRate());
    qCDebug(qLcVideoFrameEncoder) << "Adjusted frame rate:" << m_codecFrameRate;
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

        m_accel->createFramesContext(m_targetSWFormat, m_targetSize);
        if (!m_accel->hwFramesContextAsBuffer())
            return false;
    } else {
        m_targetSWFormat = m_targetFormat;
    }

    return true;
}

VideoFrameEncoder::~VideoFrameEncoder() = default;

void VideoFrameEncoder::initStream()
{
    Q_ASSERT(m_codec);

    m_stream->codecpar->codec_id = m_codec->id;

    // Apples HEVC decoders don't like the hev1 tag ffmpeg uses by default, use hvc1 as the more
    // commonly accepted tag
    if (m_codec->id == AV_CODEC_ID_HEVC)
        m_stream->codecpar->codec_tag = MKTAG('h', 'v', 'c', '1');
    else
        m_stream->codecpar->codec_tag = 0;

    // ### Fix hardcoded values
    m_stream->codecpar->format = m_targetFormat;
    m_stream->codecpar->width = m_targetSize.width();
    m_stream->codecpar->height = m_targetSize.height();
    m_stream->codecpar->sample_aspect_ratio = AVRational{ 1, 1 };
#if QT_CODEC_PARAMETERS_HAVE_FRAMERATE
    m_stream->codecpar->framerate = m_codecFrameRate;
#endif

    m_stream->time_base = adjustFrameTimeBase(m_codec->supported_framerates, m_codecFrameRate);
}

bool VideoFrameEncoder::initCodecContext()
{
    Q_ASSERT(m_codec);
    Q_ASSERT(m_stream->codecpar->codec_id);

    m_codecContext.reset(avcodec_alloc_context3(m_codec));
    if (!m_codecContext) {
        qWarning() << "Could not allocate codec context";
        return false;
    }

    // copies format, size, color params, framerate
    avcodec_parameters_to_context(m_codecContext.get(), m_stream->codecpar);
#if !QT_CODEC_PARAMETERS_HAVE_FRAMERATE
    m_codecContext->framerate = m_codecFrameRate;
#endif
    m_codecContext->time_base = m_stream->time_base;
    qCDebug(qLcVideoFrameEncoder) << "codecContext time base" << m_codecContext->time_base.num
                                  << m_codecContext->time_base.den;

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
    Q_ASSERT(m_codecContext);

    AVDictionaryHolder opts;
    applyVideoEncoderOptions(m_settings, m_codec->name, m_codecContext.get(), opts);
    applyExperimentalCodecOptions(m_codec, opts);

    const int res = avcodec_open2(m_codecContext.get(), m_codec, opts);
    if (res < 0) {
        qCWarning(qLcVideoFrameEncoder)
                << "Couldn't open video encoder" << m_codec->name << "; result:" << err2str(res);
        return false;
    }
    qCDebug(qLcVideoFrameEncoder) << "video codec opened" << res << "time base"
                                  << m_codecContext->time_base;
    return true;
}

qreal VideoFrameEncoder::codecFrameRate() const
{
    return m_codecFrameRate.den ? qreal(m_codecFrameRate.num) / m_codecFrameRate.den : 0.;
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

namespace {
struct FrameConverter
{
    FrameConverter(AVFrameUPtr inputFrame) : m_inputFrame{ std::move(inputFrame) } { }

    int downloadFromHw()
    {
        AVFrameUPtr cpuFrame = makeAVFrame();

        int err = av_hwframe_transfer_data(cpuFrame.get(), currentFrame(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder)
                    << "Error transferring frame data to surface." << err2str(err);
            return err;
        }

        setFrame(std::move(cpuFrame));
        return 0;
    }

    void convert(SwsContext *converter, AVPixelFormat format, const QSize &size)
    {
        AVFrameUPtr scaledFrame = makeAVFrame();

        scaledFrame->format = format;
        scaledFrame->width = size.width();
        scaledFrame->height = size.height();

        av_frame_get_buffer(scaledFrame.get(), 0);
        const auto scaledHeight =
                sws_scale(converter, currentFrame()->data, currentFrame()->linesize, 0, currentFrame()->height,
                          scaledFrame->data, scaledFrame->linesize);

        if (scaledHeight != scaledFrame->height)
            qCWarning(qLcVideoFrameEncoder)
                    << "Scaled height" << scaledHeight << "!=" << scaledFrame->height;

        setFrame(std::move(scaledFrame));
    }

    int uploadToHw(HWAccel *accel)
    {
        auto *hwFramesContext = accel->hwFramesContextAsBuffer();
        Q_ASSERT(hwFramesContext);
        AVFrameUPtr hwFrame = makeAVFrame();
        if (!hwFrame)
            return AVERROR(ENOMEM);

        int err = av_hwframe_get_buffer(hwFramesContext, hwFrame.get(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder) << "Error getting HW buffer" << err2str(err);
            return err;
        } else {
            qCDebug(qLcVideoFrameEncoder) << "got HW buffer";
        }
        if (!hwFrame->hw_frames_ctx) {
            qCDebug(qLcVideoFrameEncoder) << "no hw frames context";
            return AVERROR(ENOMEM);
        }
        err = av_hwframe_transfer_data(hwFrame.get(), currentFrame(), 0);
        if (err < 0) {
            qCDebug(qLcVideoFrameEncoder)
                    << "Error transferring frame data to surface." << err2str(err);
            return err;
        }

        setFrame(std::move(hwFrame));

        return 0;
    }

    QMaybe<AVFrameUPtr, int> takeResultFrame()
    {
        // Ensure that object is reset to empty state
        AVFrameUPtr converted = std::move(m_convertedFrame);
        AVFrameUPtr input = std::move(m_inputFrame);

        if (!converted)
            return input;

        // Copy metadata except size and format from input frame
        const int status = av_frame_copy_props(converted.get(), input.get());
        if (status != 0)
            return status;

        return converted;
    }

private:
    void setFrame(AVFrameUPtr frame) { m_convertedFrame = std::move(frame); }

    AVFrame *currentFrame() const
    {
        if (m_convertedFrame)
            return m_convertedFrame.get();
        return m_inputFrame.get();
    }

    AVFrameUPtr m_inputFrame;
    AVFrameUPtr m_convertedFrame;
};
}

int VideoFrameEncoder::sendFrame(AVFrameUPtr inputFrame)
{
    if (!m_codecContext) {
        qWarning() << "codec context is not initialized!";
        return AVERROR(EINVAL);
    }

    if (!inputFrame)
        return avcodec_send_frame(m_codecContext.get(), nullptr); // Flush

    if (!updateSourceFormatAndSize(inputFrame.get()))
        return AVERROR(EINVAL);

    FrameConverter converter{ std::move(inputFrame) };

    if (m_downloadFromHW) {
        const int status = converter.downloadFromHw();
        if (status != 0)
            return status;
    }

    if (m_converter)
        converter.convert(m_converter.get(), m_targetSWFormat, m_targetSize);

    if (m_uploadToHW) {
        const int status = converter.uploadToHw(m_accel.get());
        if (status != 0)
            return status;
    }

    const QMaybe<AVFrameUPtr, int> resultFrame = converter.takeResultFrame();
    if (!resultFrame)
        return resultFrame.error();

    AVRational timeBase{};
    int64_t pts{};
    getAVFrameTime(*resultFrame.value(), pts, timeBase);
    qCDebug(qLcVideoFrameEncoder) << "sending frame" << pts << "*" << timeBase;

    return avcodec_send_frame(m_codecContext.get(), resultFrame.value().get());
}

qint64 VideoFrameEncoder::estimateDuration(const AVPacket &packet, bool isFirstPacket)
{
    qint64 duration = 0; // In stream units, multiply by time_base to get seconds

    if (isFirstPacket) {
        // First packet - Estimate duration from frame rate. Duration must
        // be set for single-frame videos, otherwise they won't open in
        // media player.
        const AVRational frameDuration = av_inv_q(m_codecContext->framerate);
        duration = av_rescale_q(1, frameDuration, m_stream->time_base);
    } else {
        // Duration is calculated from actual packet times. TODO: Handle discontinuities
        duration = packet.pts - m_lastPacketTime;
    }

    return duration;
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

        if (packet->duration == 0) {
            const bool firstFrame = m_lastPacketTime == AV_NOPTS_VALUE;
            packet->duration = estimateDuration(*packet, firstFrame);
        }

        m_lastPacketTime = packet->pts;

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

bool VideoFrameEncoder::updateSourceFormatAndSize(const AVFrame *frame)
{
    Q_ASSERT(frame);

    const QSize frameSize(frame->width, frame->height);
    const AVPixelFormat frameFormat = static_cast<AVPixelFormat>(frame->format);

    if (frameSize == m_sourceSize && frameFormat == m_sourceFormat)
        return true;

    auto applySourceFormatAndSize = [&](AVPixelFormat swFormat) {
        m_sourceSize = frameSize;
        m_sourceFormat = frameFormat;
        m_sourceSWFormat = swFormat;
        updateConversions();
        return true;
    };

    if (frameFormat == m_sourceFormat)
        return applySourceFormatAndSize(m_sourceSWFormat);

    if (frameFormat == AV_PIX_FMT_NONE) {
        qWarning() << "Got a frame with invalid pixel format";
        return false;
    }

    if (isSwPixelFormat(frameFormat))
        return applySourceFormatAndSize(frameFormat);

    auto framesCtx = reinterpret_cast<const AVHWFramesContext *>(frame->hw_frames_ctx->data);
    if (!framesCtx || framesCtx->sw_format == AV_PIX_FMT_NONE) {
        qWarning() << "Cannot update conversions as hw frame has invalid framesCtx" << framesCtx;
        return false;
    }

    return applySourceFormatAndSize(framesCtx->sw_format);
}

void VideoFrameEncoder::updateConversions()
{
    const bool needToScale = m_sourceSize != m_targetSize;
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
        qCDebug(qLcVideoFrameEncoder)
                << "video source and encoder use different formats:" << m_sourceSWFormat
                << m_targetSWFormat << "or sizes:" << m_sourceSize << m_targetSize;

        m_converter.reset(sws_getContext(m_sourceSize.width(), m_sourceSize.height(),
                                         m_sourceSWFormat, m_targetSize.width(),
                                         m_targetSize.height(), m_targetSWFormat, SWS_FAST_BILINEAR,
                                         nullptr, nullptr, nullptr));
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
