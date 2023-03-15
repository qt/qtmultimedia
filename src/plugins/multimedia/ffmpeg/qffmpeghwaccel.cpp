// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "libavutil/version.h"

#include "qffmpeghwaccel_p.h"
#if QT_CONFIG(vaapi)
#include "qffmpeghwaccel_vaapi_p.h"
#endif
#ifdef Q_OS_DARWIN
#include "qffmpeghwaccel_videotoolbox_p.h"
#endif
#if QT_CONFIG(wmf)
#include "qffmpeghwaccel_d3d11_p.h"
#endif
#ifdef Q_OS_ANDROID
#    include "qffmpeghwaccel_mediacodec_p.h"
#endif
#include "qffmpeg_p.h"
#include "qffmpegvideobuffer_p.h"

#include <private/qrhi_p.h>
#include <qloggingcategory.h>
#include <set>

/* Infrastructure for HW acceleration goes into this file. */

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLHWAccel, "qt.multimedia.ffmpeg.hwaccel");

namespace QFFmpeg {

static const std::initializer_list<AVHWDeviceType> preferredHardwareAccelerators = {
#if defined(Q_OS_ANDROID)
    AV_HWDEVICE_TYPE_MEDIACODEC,
#elif defined(Q_OS_LINUX)
    AV_HWDEVICE_TYPE_VAAPI,
    AV_HWDEVICE_TYPE_VDPAU,
    AV_HWDEVICE_TYPE_CUDA,
#elif defined (Q_OS_WIN)
    AV_HWDEVICE_TYPE_D3D11VA,
#elif defined (Q_OS_DARWIN)
    AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
#endif
};

static std::vector<AVHWDeviceType> deviceTypes(const char *envVarName)
{
    std::vector<AVHWDeviceType> result;

    const auto definedDeviceTypes = qgetenv(envVarName);
    if (!definedDeviceTypes.isNull()) {
        const auto definedDeviceTypesString = QString::fromUtf8(definedDeviceTypes).toLower();
        for (const auto &deviceType : definedDeviceTypesString.split(',')) {
            if (!deviceType.isEmpty()) {
                const auto foundType = av_hwdevice_find_type_by_name(deviceType.toUtf8().data());
                if (foundType == AV_HWDEVICE_TYPE_NONE)
                    qWarning() << "Unknown hw device type" << deviceType;
                else
                    result.emplace_back(foundType);
            }
        }

        return result;
    } else {
        std::set<AVHWDeviceType> deviceTypesSet;
        AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            deviceTypesSet.insert(type);

        for (const auto preffered : preferredHardwareAccelerators)
            if (deviceTypesSet.erase(preffered))
                result.push_back(preffered);

        result.insert(result.end(), deviceTypesSet.begin(), deviceTypesSet.end());
    }

    result.shrink_to_fit();
    return result;
}

static AVBufferRef *loadHWContext(const AVHWDeviceType type)
{
    AVBufferRef *hwContext = nullptr;
    int ret = av_hwdevice_ctx_create(&hwContext, type, nullptr, nullptr, 0);
    qCDebug(qLHWAccel) << "    Checking HW context:" << av_hwdevice_get_type_name(type);
    if (ret == 0) {
        qCDebug(qLHWAccel) << "    Using above hw context.";
        return hwContext;
    }
    qCDebug(qLHWAccel) << "    Could not create hw context:" << ret << strerror(-ret);
    return nullptr;
}

template<typename CodecFinder>
std::pair<const AVCodec *, std::unique_ptr<HWAccel>>
findCodecWithHwAccel(AVCodecID id, const std::vector<AVHWDeviceType> &deviceTypes,
                     CodecFinder codecFinder,
                     const std::function<bool(const HWAccel &)> &hwAccelPredicate)
{
    for (auto type : deviceTypes) {
        const auto codec = codecFinder(id, type, {});

        if (!codec)
            continue;

        qCDebug(qLHWAccel) << "Found potential codec" << codec->name << "for hw accel" << type
                           << "; Checking the hw device...";

        auto hwAccel = QFFmpeg::HWAccel::create(type);

        if (!hwAccel)
            continue;

        if (hwAccelPredicate && !hwAccelPredicate(*hwAccel)) {
            qCDebug(qLHWAccel) << "HW device is available but doesn't suit due to restrictions";
            continue;
        }

        qCDebug(qLHWAccel) << "HW device is OK";

        return { codec, std::move(hwAccel) };
    }

    qCDebug(qLHWAccel) << "No hw acceleration found for codec id" << id;

    return { nullptr, nullptr };
}

static bool isNoConversionFormat(AVPixelFormat f)
{
    bool needsConversion = true;
    QFFmpegVideoBuffer::toQtPixelFormat(f, &needsConversion);
    return !needsConversion;
};

// Used for the AVCodecContext::get_format callback
AVPixelFormat getFormat(AVCodecContext *codecContext, const AVPixelFormat *suggestedFormats)
{
    // First check HW accelerated codecs, the HW device context must be set
    if (codecContext->hw_device_ctx) {
        auto *device_ctx = (AVHWDeviceContext *)codecContext->hw_device_ctx->data;
        std::pair formatAndScore(AV_PIX_FMT_NONE, NotSuitableAVScore);

        // to be rewritten via findBestAVFormat
        for (int i = 0;
             const AVCodecHWConfig *config = avcodec_get_hw_config(codecContext->codec, i); i++) {
            if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
                continue;

            if (device_ctx->type != config->device_type)
                continue;

            const bool isDeprecated = (config->methods & AV_CODEC_HW_CONFIG_METHOD_AD_HOC) != 0;
            const bool shouldCheckCodecFormats = config->pix_fmt == AV_PIX_FMT_NONE;

            auto scoresGettor = [&](AVPixelFormat format) {
                if (shouldCheckCodecFormats && !isAVFormatSupported(codecContext->codec, format))
                    return NotSuitableAVScore;

                if (!shouldCheckCodecFormats && config->pix_fmt != format)
                    return NotSuitableAVScore;

                auto result = DefaultAVScore;

                if (isDeprecated)
                    result -= 10000;
                if (isHwPixelFormat(format))
                    result += 10;

                return result;
            };

            const auto found = findBestAVFormat(suggestedFormats, scoresGettor);

            if (found.second > formatAndScore.second)
                formatAndScore = found;
        }

        const auto &format = formatAndScore.first;
        if (format != AV_PIX_FMT_NONE) {
#if QT_CONFIG(wmf)
            if (format == AV_PIX_FMT_D3D11)
                QFFmpeg::D3D11TextureConverter::SetupDecoderTextures(codecContext);
#endif
#ifdef Q_OS_ANDROID
            if (format == AV_PIX_FMT_MEDIACODEC)
                QFFmpeg::MediaCodecTextureConverter::setupDecoderSurface(codecContext);
#endif
            qCDebug(qLHWAccel) << "Selected format" << format << "for hw" << device_ctx->type;
            return format;
        }
    }

    // prefer video formats we can handle directly
    const auto noConversionFormat = findAVFormat(suggestedFormats, &isNoConversionFormat);
    if (noConversionFormat != AV_PIX_FMT_NONE) {
        qCDebug(qLHWAccel) << "Selected format with no conversion" << noConversionFormat;
        return noConversionFormat;
    }

    qCDebug(qLHWAccel) << "Selected format with conversion" << *suggestedFormats;

    // take the native format, this will involve one additional format conversion on the CPU side
    return *suggestedFormats;
}

TextureConverter::Data::~Data()
{
    delete backend;
}

HWAccel::~HWAccel() = default;

std::unique_ptr<HWAccel> HWAccel::create(AVHWDeviceType deviceType)
{
    if (auto *ctx = loadHWContext(deviceType))
        return std::unique_ptr<HWAccel>(new HWAccel(ctx));
    else
        return {};
}

AVPixelFormat HWAccel::format(AVFrame *frame)
{
    if (!frame->hw_frames_ctx)
        return AVPixelFormat(frame->format);

    auto *hwFramesContext = (AVHWFramesContext *)frame->hw_frames_ctx->data;
    Q_ASSERT(hwFramesContext);
    return AVPixelFormat(hwFramesContext->sw_format);
}

const std::vector<AVHWDeviceType> &HWAccel::encodingDeviceTypes()
{
    static const auto &result = deviceTypes("QT_FFMPEG_ENCODING_HW_DEVICE_TYPES");
    return result;
}

const std::vector<AVHWDeviceType> &HWAccel::decodingDeviceTypes()
{
    static const auto &result = deviceTypes("QT_FFMPEG_DECODING_HW_DEVICE_TYPES");
    return result;
}

AVHWDeviceContext *HWAccel::hwDeviceContext() const
{
    return m_hwDeviceContext ? (AVHWDeviceContext *)m_hwDeviceContext->data : nullptr;
}

AVPixelFormat HWAccel::hwFormat() const
{
    return pixelFormatForHwDevice(deviceType());
}

AVHWFramesConstraintsUPtr HWAccel::constraints() const
{
    return AVHWFramesConstraintsUPtr(
            av_hwdevice_get_hwframe_constraints(hwDeviceContextAsBuffer(), nullptr));
}

std::pair<const AVCodec *, std::unique_ptr<HWAccel>>
HWAccel::findEncoderWithHwAccel(AVCodecID id, const std::function<bool(const HWAccel &)>& hwAccelPredicate)
{
    auto finder = qOverload<AVCodecID, const std::optional<AVHWDeviceType> &,
                            const std::optional<PixelOrSampleFormat> &>(&QFFmpeg::findAVEncoder);
    return findCodecWithHwAccel(id, encodingDeviceTypes(), finder, hwAccelPredicate);
}

std::pair<const AVCodec *, std::unique_ptr<HWAccel>>
HWAccel::findDecoderWithHwAccel(AVCodecID id, const std::function<bool(const HWAccel &)>& hwAccelPredicate)
{
    return findCodecWithHwAccel(id, decodingDeviceTypes(), &QFFmpeg::findAVDecoder,
                                hwAccelPredicate);
}

AVHWDeviceType HWAccel::deviceType() const
{
    return m_hwDeviceContext ? hwDeviceContext()->type : AV_HWDEVICE_TYPE_NONE;
}

void HWAccel::createFramesContext(AVPixelFormat swFormat, const QSize &size)
{
    if (m_hwFramesContext) {
        qWarning() << "Frames context has been already created!";
        return;
    }

    if (!m_hwDeviceContext)
        return;

    m_hwFramesContext.reset(av_hwframe_ctx_alloc(m_hwDeviceContext.get()));
    auto *c = (AVHWFramesContext *)m_hwFramesContext->data;
    c->format = hwFormat();
    c->sw_format = swFormat;
    c->width = size.width();
    c->height = size.height();
    qCDebug(qLHWAccel) << "init frames context";
    int err = av_hwframe_ctx_init(m_hwFramesContext.get());
    if (err < 0)
        qWarning() << "failed to init HW frame context" << err << err2str(err);
    else
        qCDebug(qLHWAccel) << "Initialized frames context" << size << c->format << c->sw_format;
}

AVHWFramesContext *HWAccel::hwFramesContext() const
{
    return m_hwFramesContext ? (AVHWFramesContext *)m_hwFramesContext->data : nullptr;
}


TextureConverter::TextureConverter(QRhi *rhi)
    : d(new Data)
{
    d->rhi = rhi;
}

TextureSet *TextureConverter::getTextures(AVFrame *frame)
{
    if (!frame || isNull())
        return nullptr;

    Q_ASSERT(frame->format == d->format);
    return d->backend->getTextures(frame);
}

void TextureConverter::updateBackend(AVPixelFormat fmt)
{
    d->backend = nullptr;
    if (!d->rhi)
        return;

    // HW textures conversions are not stable in specific cases, dependent on the hardware and OS.
    // We need the env var for testing with no textures conversion on the user's side.
    static const bool disableConversion =
            qEnvironmentVariableIsSet("QT_DISABLE_HW_TEXTURES_CONVERSION");

    if (disableConversion)
        return;

    switch (fmt) {
#if QT_CONFIG(vaapi)
    case AV_PIX_FMT_VAAPI:
        d->backend = new VAAPITextureConverter(d->rhi);
        break;
#endif
#ifdef Q_OS_DARWIN
    case AV_PIX_FMT_VIDEOTOOLBOX:
        d->backend = new VideoToolBoxTextureConverter(d->rhi);
        break;
#endif
#if QT_CONFIG(wmf)
    case AV_PIX_FMT_D3D11:
        d->backend = new D3D11TextureConverter(d->rhi);
        break;
#endif
#ifdef Q_OS_ANDROID
    case AV_PIX_FMT_MEDIACODEC:
        d->backend = new MediaCodecTextureConverter(d->rhi);
        break;
#endif
    default:
        break;
    }
    d->format = fmt;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
