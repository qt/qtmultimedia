// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpegtextureconverter_p.h"
#include "qffmpeghwaccel_p.h"
#include <rhi/qrhi.h>

#include <q20type_traits.h>

#if QT_CONFIG(vaapi)
#  include "qffmpeghwaccel_vaapi_p.h"
#endif

#ifdef Q_OS_DARWIN
#  include "qffmpeghwaccel_videotoolbox_p.h"
#endif

#ifdef Q_OS_WINDOWS
#  include "qffmpeghwaccel_d3d11_p.h"
#endif

#ifdef Q_OS_ANDROID
#  include "qffmpeghwaccel_mediacodec_p.h"
#endif

QT_BEGIN_NAMESPACE

using namespace QFFmpeg;

namespace {

template <typename Converter>
using ConverterTypeIdentity = q20::type_identity<Converter>;

template <typename ConverterTypeHandler>
void applyConverterTypeByPixelFormat(AVPixelFormat fmt, const QRhi &rhi,
                                     ConverterTypeHandler &&handler)
{
    if (!TextureConverter::hwTextureConversionEnabled())
        return;

    switch (fmt) {
#if QT_CONFIG(vaapi)
    case AV_PIX_FMT_VAAPI:
        handler(ConverterTypeIdentity<VAAPITextureConverter>{});
        break;
#endif
#ifdef Q_OS_DARWIN
    case AV_PIX_FMT_VIDEOTOOLBOX:
        handler(ConverterTypeIdentity<VideoToolBoxTextureConverter>{});
        break;
#endif
#ifdef Q_OS_WINDOWS
    case AV_PIX_FMT_D3D11:
        if (rhi.backend() == QRhi::Implementation::D3D11) {
            if (rhi.driverInfo().deviceType != QRhiDriverInfo::CpuDevice)
                handler(ConverterTypeIdentity<D3D11TextureConverter>{});
        }
        break;
#endif
#ifdef Q_OS_ANDROID
    case AV_PIX_FMT_MEDIACODEC:
        handler(ConverterTypeIdentity<MediaCodecTextureConverter>{});
        break;
#endif
    default:
        Q_UNUSED(handler)
        Q_UNUSED(rhi)
        break;
    }
}

} // namespace

TextureConverterBackend::~TextureConverterBackend() = default;

TextureConverter::TextureConverter(QRhi &rhi) : m_rhi(rhi) { }

bool TextureConverter::init(AVFrame &hwFrame)
{
    Q_ASSERT(hwFrame.hw_frames_ctx);
    AVPixelFormat fmt = AVPixelFormat(hwFrame.format);
    if (fmt != m_format)
        updateBackend(fmt);
    return !isNull();
}

QVideoFrameTexturesUPtr TextureConverter::createTextures(AVFrame &hwFrame,
                                                         QVideoFrameTexturesUPtr &oldTextures)
{
    if (isNull())
        return nullptr;

    Q_ASSERT(hwFrame.format == m_format);
    return m_backend->createTextures(&hwFrame, oldTextures);
}

QVideoFrameTexturesHandlesUPtr
TextureConverter::createTextureHandles(AVFrame &hwFrame, QVideoFrameTexturesHandlesUPtr oldHandles)
{
    if (isNull())
        return nullptr;

    Q_ASSERT(hwFrame.format == m_format);
    return m_backend->createTextureHandles(&hwFrame, std::move(oldHandles));
}

void TextureConverter::updateBackend(AVPixelFormat fmt)
{
    m_backend = nullptr;
    m_format = fmt; // should be saved even if m_backend is not created

    applyConverterTypeByPixelFormat(m_format, m_rhi, [this](auto converterTypeIdentity) {
        using ConverterType = typename decltype(converterTypeIdentity)::type;
        m_backend = std::make_shared<ConverterType>(&m_rhi);
    });
}

bool TextureConverter::hwTextureConversionEnabled()
{
    // HW texture conversions are not stable in specific cases, dependent on the hardware and OS.
    // We need the env var for testing with no texture conversion on the user's side.
    static const int disableHwConversion =
            qEnvironmentVariableIntValue("QT_DISABLE_HW_TEXTURES_CONVERSION");

    return !disableHwConversion;
}

void TextureConverter::applyDecoderPreset(const AVPixelFormat format, AVCodecContext &codecContext)
{
    if (!hwTextureConversionEnabled())
        return;

    Q_ASSERT(codecContext.codec && Codec(codecContext.codec).isDecoder());

#ifdef Q_OS_WINDOWS
    if (format == AV_PIX_FMT_D3D11)
        D3D11TextureConverter::SetupDecoderTextures(&codecContext);
#elif defined Q_OS_ANDROID
    if (format == AV_PIX_FMT_MEDIACODEC)
        MediaCodecTextureConverter::setupDecoderSurface(&codecContext);
#else
    Q_UNUSED(codecContext);
    Q_UNUSED(format);
#endif
}

bool TextureConverter::isBackendAvailable(AVFrame &hwFrame, const QRhi &rhi)
{
    bool result = false;
    applyConverterTypeByPixelFormat(AVPixelFormat(hwFrame.format), rhi, [&result](auto) {
        result = true;
    });
    return result;
}

QT_END_NAMESPACE
