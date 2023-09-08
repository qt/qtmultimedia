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
#include <QtCore/private/qsystemlibrary_p.h>

#endif
#ifdef Q_OS_ANDROID
#    include "qffmpeghwaccel_mediacodec_p.h"
#endif
#include "qffmpeg_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qscopedvaluerollback.h"
#include "QtCore/qfile.h"

#include <rhi/qrhi.h>
#include <qloggingcategory.h>
#include <unordered_set>

/* Infrastructure for HW acceleration goes into this file. */

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLHWAccel, "qt.multimedia.ffmpeg.hwaccel");
extern bool thread_local FFmpegLogsEnabledInThread;

namespace QFFmpeg {

static const std::initializer_list<AVHWDeviceType> preferredHardwareAccelerators = {
#if defined(Q_OS_ANDROID)
    AV_HWDEVICE_TYPE_MEDIACODEC,
#elif defined(Q_OS_LINUX)
    AV_HWDEVICE_TYPE_CUDA,
    AV_HWDEVICE_TYPE_VAAPI,

    // TODO: investigate VDPAU advantages.
    // nvenc/nvdec codecs use AV_HWDEVICE_TYPE_CUDA by default, but they can also use VDPAU
    // if it's included into the ffmpeg build and vdpau drivers are installed.
    // AV_HWDEVICE_TYPE_VDPAU
#elif defined (Q_OS_WIN)
    AV_HWDEVICE_TYPE_D3D11VA,
#elif defined (Q_OS_DARWIN)
    AV_HWDEVICE_TYPE_VIDEOTOOLBOX,
#endif
};

static AVBufferUPtr loadHWContext(AVHWDeviceType type)
{
    AVBufferRef *hwContext = nullptr;
    qCDebug(qLHWAccel) << "    Checking HW context:" << av_hwdevice_get_type_name(type);
    int ret = av_hwdevice_ctx_create(&hwContext, type, nullptr, nullptr, 0);

    if (ret == 0) {
        qCDebug(qLHWAccel) << "    Using above hw context.";
        return AVBufferUPtr(hwContext);
    }
    qCDebug(qLHWAccel) << "    Could not create hw context:" << ret << strerror(-ret);
    return nullptr;
}

// FFmpeg might crash on loading non-existing hw devices.
// Let's roughly precheck drivers/libraries.
static bool precheckDriver(AVHWDeviceType type)
{
    // precheckings might need some improvements
#if defined(Q_OS_LINUX)
    if (type == AV_HWDEVICE_TYPE_CUDA)
        return QFile::exists(QLatin1String("/proc/driver/nvidia/version"));
#elif defined(Q_OS_WINDOWS)
    if (type == AV_HWDEVICE_TYPE_D3D11VA)
        return QSystemLibrary(QLatin1String("d3d11.dll")).load();

    if (type == AV_HWDEVICE_TYPE_DXVA2)
        return QSystemLibrary(QLatin1String("d3d9.dll")).load();

    // TODO: check nvenc/nvdec and revisit the checking
    if (type == AV_HWDEVICE_TYPE_CUDA)
        return QSystemLibrary(QLatin1String("nvml.dll")).load();
#else
     Q_UNUSED(type);
#endif

    return true;
}

static bool checkHwType(AVHWDeviceType type)
{
    const auto deviceName = av_hwdevice_get_type_name(type);
    if (!deviceName) {
        qWarning() << "Internal ffmpeg error, unknow hw type:" << type;
        return false;
    }

    if (!precheckDriver(type)) {
        qCDebug(qLHWAccel) << "Drivers for hw device" << deviceName << "is not installed";
        return false;
    }

    if (type == AV_HWDEVICE_TYPE_MEDIACODEC ||
        type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX ||
        type == AV_HWDEVICE_TYPE_D3D11VA ||
        type == AV_HWDEVICE_TYPE_DXVA2)
        return true; // Don't waste time; it's expected to work fine of the precheck is OK


    QScopedValueRollback rollback(FFmpegLogsEnabledInThread);
    FFmpegLogsEnabledInThread = false;

    return loadHWContext(type) != nullptr;
}

static const std::vector<AVHWDeviceType> &deviceTypes()
{
    static const auto types = []() {
        qCDebug(qLHWAccel) << "Check device types";
        QElapsedTimer timer;
        timer.start();

        // gather hw pix formats
        std::unordered_set<AVPixelFormat> hwPixFormats;
        void *opaque = nullptr;
        while (auto codec = av_codec_iterate(&opaque)) {
            if (auto pixFmt = codec->pix_fmts)
                for (; *pixFmt != AV_PIX_FMT_NONE; ++pixFmt)
                    if (isHwPixelFormat(*pixFmt))
                        hwPixFormats.insert(*pixFmt);
        }

        // create a device types list
        std::vector<AVHWDeviceType> result;
        AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
        while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
            if (hwPixFormats.count(pixelFormatForHwDevice(type)) && checkHwType(type))
                result.push_back(type);
        result.shrink_to_fit();

        // reorder the list accordingly preferredHardwareAccelerators
        auto it = result.begin();
        for (const auto preffered : preferredHardwareAccelerators) {
            auto found = std::find(it, result.end(), preffered);
            if (found != result.end())
                std::rotate(it++, found, std::next(found));
        }

        using namespace std::chrono;
        qCDebug(qLHWAccel) << "Device types checked. Spent time:" << duration_cast<microseconds>(timer.durationElapsed());

        return result;
    }();

    return types;
}

static std::vector<AVHWDeviceType> deviceTypes(const char *envVarName)
{
    const auto definedDeviceTypes = qgetenv(envVarName);

    if (definedDeviceTypes.isNull())
        return deviceTypes();

    std::vector<AVHWDeviceType> result;
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

    result.shrink_to_fit();
    return result;
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

namespace {

bool hwTextureConversionEnabled(AVPixelFormat fmt)
{

    // HW textures conversions are not stable in specific cases, dependent on the hardware and OS.
    // We need the env var for testing with no textures conversion on the user's side.
    static bool isDisableConversionSet = false;
    static const int disableHwConversion = qEnvironmentVariableIntValue(
            "QT_DISABLE_HW_TEXTURES_CONVERSION", &isDisableConversionSet);

    if (disableHwConversion)
        return false;

#if QT_CONFIG(wmf)
    if (fmt == AV_PIX_FMT_D3D11) {
        // On Windows, HW texture conversion currently causes stuttering video display and possibly
        // crash on AMD GPUs. See for example QTBUG-113832 and QTBUG-111543. On this platform, HW
        // texture conversions have to be explicitly enabled for debugging and testing.
        if (!isDisableConversionSet)
            return false;
    }
#else
    Q_UNUSED(fmt);
#endif

    return true;
}

void setupDecoder(const AVPixelFormat format, AVCodecContext *const codecContext)
{
    if (!hwTextureConversionEnabled(format))
        return;

#if QT_CONFIG(wmf)
    if (format == AV_PIX_FMT_D3D11)
        QFFmpeg::D3D11TextureConverter::SetupDecoderTextures(codecContext);
#elif defined Q_OS_ANDROID
    if (format == AV_PIX_FMT_MEDIACODEC)
        QFFmpeg::MediaCodecTextureConverter::setupDecoderSurface(codecContext);
#else
    Q_UNUSED(codecContext);
#endif
}

} // namespace

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
            setupDecoder(format, codecContext);
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
    if (auto ctx = loadHWContext(deviceType))
        return std::unique_ptr<HWAccel>(new HWAccel(std::move(ctx)));
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

const AVHWFramesConstraints *HWAccel::constraints() const
{
    std::call_once(m_constraintsOnceFlag, [this]() {
        if (auto context = hwDeviceContextAsBuffer())
            m_constraints.reset(av_hwdevice_get_hwframe_constraints(context, nullptr));
    });

    return m_constraints.get();
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

    if (!hwTextureConversionEnabled(fmt))
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
