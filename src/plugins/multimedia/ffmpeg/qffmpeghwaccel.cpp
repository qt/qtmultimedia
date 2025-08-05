// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "libavutil/version.h"

#include "qffmpeghwaccel_p.h"

#ifdef Q_OS_WINDOWS
#  include "qffmpeghwaccel_d3d11_p.h"
#  include <QtCore/private/qsystemlibrary_p.h>
#endif

#include "qffmpeg_p.h"
#include "qffmpegcodecstorage_p.h"
#include "qffmpegmediaintegration_p.h"
#include "qffmpegvideobuffer_p.h"
#include "qscopedvaluerollback.h"

#include <QtCore/QElapsedTimer>

#ifdef Q_OS_LINUX
#  include "QtCore/qfile.h"
#  include <QLibrary>
#endif

#include <rhi/qrhi.h>
#include <qloggingcategory.h>
#include <unordered_set>

/* Infrastructure for HW acceleration goes into this file. */

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

Q_STATIC_LOGGING_CATEGORY(qLHWAccel, "qt.multimedia.ffmpeg.hwaccel");

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
    if (type == AV_HWDEVICE_TYPE_CUDA) {
        if (!QFile::exists(QLatin1String("/proc/driver/nvidia/version")))
            return false;

        // QTBUG-122199
        // CUDA backend requires libnvcuvid in libavcodec
        QLibrary lib(u"libnvcuvid.so"_s);
        if (!lib.load())
            return false;
        lib.unload();
        return true;
    }
#elif defined(Q_OS_WINDOWS)
    if (type == AV_HWDEVICE_TYPE_D3D11VA)
        return QSystemLibrary(QLatin1String("d3d11.dll")).load();

#if QT_FFMPEG_HAS_D3D12VA
    if (type == AV_HWDEVICE_TYPE_D3D12VA)
        return QSystemLibrary(QLatin1String("d3d12.dll")).load();
#endif

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
        qWarning() << "Internal FFmpeg error, unknow hw type:" << type;
        return false;
    }

    if (!precheckDriver(type)) {
        qCDebug(qLHWAccel) << "Drivers for hw device" << deviceName << "is not installed";
        return false;
    }

    if (type == AV_HWDEVICE_TYPE_MEDIACODEC ||
        type == AV_HWDEVICE_TYPE_VIDEOTOOLBOX ||
        type == AV_HWDEVICE_TYPE_D3D11VA ||
#if QT_FFMPEG_HAS_D3D12VA
        type == AV_HWDEVICE_TYPE_D3D12VA ||
#endif
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
        for (const Codec codec : CodecEnumerator()) {
            forEachAVPixelFormat(codec, [&](AVPixelFormat format) {
                if (isHwPixelFormat(format))
                    hwPixFormats.insert(format);
            });
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
    for (const auto &deviceType : definedDeviceTypesString.split(u',')) {
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

std::pair<std::optional<Codec>, HWAccelUPtr> HWAccel::findDecoderWithHwAccel(AVCodecID id)
{
    for (auto type : decodingDeviceTypes()) {
        const std::optional<Codec> codec = findAVDecoder(id, pixelFormatForHwDevice(type));

        if (!codec)
            continue;

        qCDebug(qLHWAccel) << "Found potential codec" << codec->name() << "for hw accel" << type
                           << "; Checking the hw device...";

        HWAccelUPtr hwAccel = create(type);

        if (!hwAccel)
            continue;

        qCDebug(qLHWAccel) << "HW device is OK";

        return { codec, std::move(hwAccel) };
    }

    qCDebug(qLHWAccel) << "No hw acceleration found for codec id" << id;

    return { std::nullopt, nullptr };
}

static bool isNoConversionFormat(AVPixelFormat f)
{
    bool needsConversion = true;
    QFFmpegVideoBuffer::toQtPixelFormat(f, &needsConversion);
    return !needsConversion;
};

// Used for the AVCodecContext::get_format callback
AVPixelFormat getFormat(AVCodecContext *codecContext, const AVPixelFormat *fmt)
{
    QSpan<const AVPixelFormat> suggestedFormats = makeSpan(fmt);
    // First check HW accelerated codecs, the HW device context must be set
    if (codecContext->hw_device_ctx) {
        auto *device_ctx = (AVHWDeviceContext *)codecContext->hw_device_ctx->data;
        ValueAndScore<AVPixelFormat> formatAndScore;

        // to be rewritten via findBestAVFormat
        const Codec codec{ codecContext->codec };
        for (const AVCodecHWConfig *config : codec.hwConfigs()) {
            if (!(config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX))
                continue;

            if (device_ctx->type != config->device_type)
                continue;

            const bool isDeprecated = (config->methods & AV_CODEC_HW_CONFIG_METHOD_AD_HOC) != 0;
            const bool shouldCheckCodecFormats = config->pix_fmt == AV_PIX_FMT_NONE;

            auto scoresGettor = [&](AVPixelFormat format) {
                // check in supported codec->pix_fmts (avcodec_get_supported_config with
                // AV_CODEC_CONFIG_PIX_FORMAT since n7.1); no reason to use findAVPixelFormat as
                // we're already in the hw_config loop
                const auto pixelFormats = codec.pixelFormats();
                if (shouldCheckCodecFormats && !hasValue(pixelFormats, format))
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

            const auto found = findBestAVValueWithScore(suggestedFormats, scoresGettor);

            if (found.score > formatAndScore.score)
                formatAndScore = found;
        }

        const auto format = formatAndScore.value;
        if (format) {
            TextureConverter::applyDecoderPreset(*format, *codecContext);
            qCDebug(qLHWAccel) << "Selected format" << *format << "for hw" << device_ctx->type;
            return *format;
        }
    }

    // prefer video formats we can handle directly
    const auto noConversionFormat = findIf(suggestedFormats, &isNoConversionFormat);
    if (noConversionFormat) {
        qCDebug(qLHWAccel) << "Selected format with no conversion" << *noConversionFormat;
        return *noConversionFormat;
    }

    const AVPixelFormat format = !suggestedFormats.empty() ? suggestedFormats[0] : AV_PIX_FMT_NONE;
    qCDebug(qLHWAccel) << "Selected format with conversion" << format;

    // take the native format, this will involve one additional format conversion on the CPU side
    return format;
}

HWAccel::~HWAccel() = default;

HWAccelUPtr HWAccel::create(AVHWDeviceType deviceType)
{
    if (auto ctx = loadHWContext(deviceType))
        return HWAccelUPtr(new HWAccel(std::move(ctx)));
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

bool HWAccel::matchesSizeContraints(QSize size) const
{
    const auto constraints = this->constraints();
    if (!constraints)
        return true;

    return size.width() >= constraints->min_width
            && size.height() >= constraints->min_height
            && size.width() <= constraints->max_width
            && size.height() <= constraints->max_height;
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
        qWarning() << "failed to init HW frame context" << err << AVError(err);
    else
        qCDebug(qLHWAccel) << "Initialized frames context" << size << c->format << c->sw_format;
}

AVHWFramesContext *HWAccel::hwFramesContext() const
{
    return m_hwFramesContext ? (AVHWFramesContext *)m_hwFramesContext->data : nullptr;
}

static void deleteHwFrameContextData(AVHWFramesContext *context)
{
    delete reinterpret_cast<HwFrameContextData *>(context->user_opaque);
}

HwFrameContextData &HwFrameContextData::ensure(AVFrame &hwFrame)
{
    Q_ASSERT(hwFrame.hw_frames_ctx && hwFrame.hw_frames_ctx->data);

    auto context = reinterpret_cast<AVHWFramesContext *>(hwFrame.hw_frames_ctx->data);
    if (!context->user_opaque) {
        context->user_opaque = new HwFrameContextData;
        Q_ASSERT(!context->free);
        context->free = deleteHwFrameContextData;
    } else {
        Q_ASSERT(context->free == deleteHwFrameContextData);
    }

    return *reinterpret_cast<HwFrameContextData *>(context->user_opaque);
}

AVFrameUPtr copyFromHwPool(AVFrameUPtr frame)
{
#ifdef Q_OS_WINDOWS
    return copyFromHwPoolD3D11(std::move(frame));
#else
    return frame;
#endif
}

} // namespace QFFmpeg

QT_END_NAMESPACE
