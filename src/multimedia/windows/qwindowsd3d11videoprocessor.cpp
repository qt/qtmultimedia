// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsd3d11videoprocessor_p.h"

#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qsystemerror_p.h>

#include <dxgi1_2.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcD3D11VideoProcessor, "qt.multimedia.windows.d3d11videoprocessor");

namespace QtMultimediaPrivate {

namespace {

DXGI_COLOR_SPACE_TYPE toDxgiColorSpace(QVideoFrameFormat::ColorSpace colorSpace,
                                       QVideoFrameFormat::ColorRange colorRange,
                                       QVideoFrameFormat::ColorTransfer colorTransfer)
{
    const bool fullRange = colorRange == QVideoFrameFormat::ColorRange_Full;

    switch (colorSpace) {
    case QVideoFrameFormat::ColorSpace_BT2020:
        switch (colorTransfer) {
        case QVideoFrameFormat::ColorTransfer_ST2084:
            // DXGI defines no full-range YCbCr/PQ/2020 space; HDR10 input is always
            // studio-range, tone-mapped to the SDR output space by the processor.
            return DXGI_COLOR_SPACE_YCBCR_STUDIO_G2084_LEFT_P2020;
        case QVideoFrameFormat::ColorTransfer_STD_B67:
            return fullRange ? DXGI_COLOR_SPACE_YCBCR_FULL_GHLG_TOPLEFT_P2020
                             : DXGI_COLOR_SPACE_YCBCR_STUDIO_GHLG_TOPLEFT_P2020;
        case QVideoFrameFormat::ColorTransfer_Unknown:
        case QVideoFrameFormat::ColorTransfer_BT709:
        case QVideoFrameFormat::ColorTransfer_BT601:
        case QVideoFrameFormat::ColorTransfer_Linear:
        case QVideoFrameFormat::ColorTransfer_Gamma22:
        case QVideoFrameFormat::ColorTransfer_Gamma28:
            // BT.2020 primaries with a gamma transfer are wide-gamut SDR, not HDR.
            return fullRange ? DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P2020
                             : DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020;
        }
        Q_UNREACHABLE_RETURN(DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P2020);
    case QVideoFrameFormat::ColorSpace_BT601:
        return fullRange ? DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P601
                         : DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P601;
    case QVideoFrameFormat::ColorSpace_BT709:
    case QVideoFrameFormat::ColorSpace_AdobeRgb:
    case QVideoFrameFormat::ColorSpace_Undefined:
        // Fallback for color spaces with no DXGI equivalent (e.g. AdobeRgb).
        return fullRange ? DXGI_COLOR_SPACE_YCBCR_FULL_G22_LEFT_P709
                         : DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
    }
    Q_UNREACHABLE_RETURN(DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709);
}

} // namespace

QWindowsD3D11VideoProcessor::QWindowsD3D11VideoProcessor() = default;
QWindowsD3D11VideoProcessor::~QWindowsD3D11VideoProcessor() = default;

bool QWindowsD3D11VideoProcessor::isPixelFormatSupported(QVideoFrameFormat::PixelFormat pixelFormat)
{
    // Matches the D3D11VA surface formats in FFmpeg's hwcontext_d3d11va supported_formats[].
    switch (pixelFormat) {
    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_P010:
        return true;
    default:
        return false;
    }
}

bool QWindowsD3D11VideoProcessor::ensureVideoProcessor(ID3D11Device *device, const QSize &frameSize)
{
    if (m_videoProcessor && m_processorSize == frameSize)
        return true;

    m_videoProcessor.Reset();
    m_enumerator.Reset();
    m_processorSize = {};

    if (!m_videoDevice) {
        if (FAILED(device->QueryInterface(IID_PPV_ARGS(&m_videoDevice))))
            return false;
    }

    if (!m_videoContext) {
        ComPtr<ID3D11DeviceContext> ctx;
        device->GetImmediateContext(&ctx);
        if (FAILED(ctx.As(&m_videoContext)))
            return false;
    }

    D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc{};
    contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
    contentDesc.InputWidth = static_cast<UINT>(frameSize.width());
    contentDesc.InputHeight = static_cast<UINT>(frameSize.height());
    contentDesc.OutputWidth = static_cast<UINT>(frameSize.width());
    contentDesc.OutputHeight = static_cast<UINT>(frameSize.height());
    contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

    HRESULT hr = m_videoDevice->CreateVideoProcessorEnumerator(&contentDesc, &m_enumerator);
    if (FAILED(hr)) {
        qCWarning(lcD3D11VideoProcessor)
                << "CreateVideoProcessorEnumerator:" << QSystemError::windowsComString(hr);
        return false;
    }

    hr = m_videoDevice->CreateVideoProcessor(m_enumerator.Get(), 0, &m_videoProcessor);
    if (FAILED(hr)) {
        qCWarning(lcD3D11VideoProcessor)
                << "CreateVideoProcessor:" << QSystemError::windowsComString(hr);
        m_enumerator.Reset();
        return false;
    }

    m_processorSize = frameSize;
    return true;
}

bool QWindowsD3D11VideoProcessor::ensureOutputTexture(ID3D11Device *device, const QSize &frameSize)
{
    if (m_outputTex) {
        D3D11_TEXTURE2D_DESC desc{};
        m_outputTex->GetDesc(&desc);
        if (desc.Width == static_cast<UINT>(frameSize.width())
            && desc.Height == static_cast<UINT>(frameSize.height()))
            return true;
    }

    m_outputView.Reset();
    m_outputTex.Reset();

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(frameSize.width());
    desc.Height = static_cast<UINT>(frameSize.height());
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc = { 1, 0 };
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

    if (FAILED(device->CreateTexture2D(&desc, nullptr, &m_outputTex)))
        return false;

    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputViewDesc{};
    outputViewDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
    outputViewDesc.Texture2D.MipSlice = 0;

    HRESULT hr = m_videoDevice->CreateVideoProcessorOutputView(
            m_outputTex.Get(), m_enumerator.Get(), &outputViewDesc, &m_outputView);
    if (FAILED(hr)) {
        qCWarning(lcD3D11VideoProcessor)
                << "CreateVideoProcessorOutputView:" << QSystemError::windowsComString(hr);
        m_outputTex.Reset();
        return false;
    }

    return true;
}

ComPtr<ID3D11Texture2D> QWindowsD3D11VideoProcessor::convertToBGRA(
        ID3D11Device *device, ID3D11DeviceContext *context, const ComPtr<ID3D11Texture2D> &texture,
        const QSize &frameSize, QVideoFrameFormat::ColorSpace colorSpace,
        QVideoFrameFormat::ColorRange colorRange, QVideoFrameFormat::ColorTransfer colorTransfer,
        UINT subresourceIndex)
{
    Q_ASSERT(device);
    Q_ASSERT(context);
    Q_ASSERT(texture);

    if (frameSize.isEmpty())
        return nullptr;

    if (!ensureVideoProcessor(device, frameSize))
        return nullptr;

    if (!ensureOutputTexture(device, frameSize))
        return nullptr;

    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputViewDesc{};
    inputViewDesc.FourCC = 0;
    inputViewDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
    inputViewDesc.Texture2D.MipSlice = 0;
    inputViewDesc.Texture2D.ArraySlice = subresourceIndex;

    ComPtr<ID3D11VideoProcessorInputView> inputView;
    HRESULT hr = m_videoDevice->CreateVideoProcessorInputView(texture.Get(), m_enumerator.Get(),
                                                              &inputViewDesc, &inputView);
    if (FAILED(hr)) {
        qCWarning(lcD3D11VideoProcessor)
                << "CreateVideoProcessorInputView:" << QSystemError::windowsComString(hr);
        return nullptr;
    }

    m_videoContext->VideoProcessorSetStreamColorSpace1(
            m_videoProcessor.Get(), 0, toDxgiColorSpace(colorSpace, colorRange, colorTransfer));
    m_videoContext->VideoProcessorSetOutputColorSpace1(m_videoProcessor.Get(),
                                                       DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709);

    D3D11_VIDEO_PROCESSOR_STREAM stream{};
    stream.Enable = TRUE;
    stream.pInputSurface = inputView.Get();

    // ID3D11VideoContext is not thread-safe; the caller must hold the D3D11 device lock.
    hr = m_videoContext->VideoProcessorBlt(m_videoProcessor.Get(), m_outputView.Get(), 0, 1,
                                           &stream);
    if (FAILED(hr)) {
        qCWarning(lcD3D11VideoProcessor)
                << "VideoProcessorBlt:" << QSystemError::windowsComString(hr);
        return nullptr;
    }

    context->Flush();

    return m_outputTex;
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
