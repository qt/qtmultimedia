// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_d3d11_p.h"

#include <qvideoframeformat.h>
#include "qffmpegvideobuffer_p.h"


#include <private/qvideotexturehelper_p.h>
#include <private/qcomptr_p.h>
#include <rhi/qrhi.h>

#include <qopenglfunctions.h>
#include <qdebug.h>
#include <qloggingcategory.h>

#include <libavutil/hwcontext_d3d11va.h>

QT_BEGIN_NAMESPACE

static Q_LOGGING_CATEGORY(qLcMediaFFmpegHWAccel, "qt.multimedia.hwaccel")

namespace QFFmpeg {

class D3D11TextureSet : public TextureSet
{
public:
    D3D11TextureSet(ComPtr<ID3D11Texture2D> &&tex)
        : m_tex(tex)
    {}

    qint64 textureHandle(int /*plane*/) override
    {
        return qint64(m_tex.Get());
    }

private:
    ComPtr<ID3D11Texture2D> m_tex;
};


D3D11TextureConverter::D3D11TextureConverter(QRhi *rhi)
    : TextureConverterBackend(rhi)
{
}

static ComPtr<ID3D11Texture2D> getSharedTextureForDevice(ID3D11Device *dev, ID3D11Texture2D *tex)
{
    ComPtr<IDXGIResource> dxgiResource;
    HRESULT hr = tex->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void **>(dxgiResource.GetAddressOf()));
    if (FAILED(hr)) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to obtain resource handle from FFMpeg texture" << hr;
        return {};
    }
    HANDLE shared = nullptr;
    hr = dxgiResource->GetSharedHandle(&shared);
    if (FAILED(hr)) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to obtain shared handle for FFmpeg texture" << hr;
        return {};
    }

    ComPtr<ID3D11Texture2D> sharedTex;
    hr = dev->OpenSharedResource(shared, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(sharedTex.GetAddressOf()));
    if (FAILED(hr))
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to share FFmpeg texture" << hr;
    return sharedTex;
}

static ComPtr<ID3D11Texture2D> copyTextureFromArray(ID3D11Device *dev, ID3D11Texture2D *array, int index)
{
    D3D11_TEXTURE2D_DESC arrayDesc = {};
    array->GetDesc(&arrayDesc);

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = arrayDesc.Width;
    texDesc.Height = arrayDesc.Height;
    texDesc.Format = arrayDesc.Format;
    texDesc.ArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    texDesc.MiscFlags = 0;
    texDesc.SampleDesc = { 1, 0};

    ComPtr<ID3D11Texture2D> texCopy;
    HRESULT hr = dev->CreateTexture2D(&texDesc, nullptr, texCopy.GetAddressOf());
    if (FAILED(hr)) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to create texture" << hr;
        return {};
    }

    ComPtr<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext(ctx.GetAddressOf());
    ctx->CopySubresourceRegion(texCopy.Get(), 0, 0, 0, 0, array, index, nullptr);

    return texCopy;
}

TextureSet *D3D11TextureConverter::getTextures(AVFrame *frame)
{
    if (!frame || !frame->hw_frames_ctx || frame->format != AV_PIX_FMT_D3D11)
        return nullptr;

    auto *fCtx = (AVHWFramesContext *)frame->hw_frames_ctx->data;
    auto *ctx = fCtx->device_ctx;
    if (!ctx || ctx->type != AV_HWDEVICE_TYPE_D3D11VA)
        return nullptr;

    auto nh = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    if (!nh)
        return nullptr;

    auto ffmpegTex = (ID3D11Texture2D *)frame->data[0];
    int index = (intptr_t)frame->data[1];

    if (rhi->backend() == QRhi::D3D11) {
        auto dev = reinterpret_cast<ID3D11Device *>(nh->dev);
        if (!dev)
            return nullptr;
        auto sharedTex = getSharedTextureForDevice(dev, ffmpegTex);
        if (sharedTex) {
            auto tex = copyTextureFromArray(dev, sharedTex.Get(), index);
            if (tex) {
                return new D3D11TextureSet(std::move(tex));
            }
        }
    }

    return nullptr;
}

void D3D11TextureConverter::SetupDecoderTextures(AVCodecContext *s)
{
    int ret = avcodec_get_hw_frames_parameters(s,
                                               s->hw_device_ctx,
                                               AV_PIX_FMT_D3D11,
                                               &s->hw_frames_ctx);
    if (ret < 0) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to allocate HW frames context" << ret;
        return;
    }

    auto *frames_ctx = (AVHWFramesContext *)s->hw_frames_ctx->data;
    auto *hwctx = (AVD3D11VAFramesContext *)frames_ctx->hwctx;
    hwctx->MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    hwctx->BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
    ret = av_hwframe_ctx_init(s->hw_frames_ctx);
    if (ret < 0) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to initialize HW frames context" << ret;
        av_buffer_unref(&s->hw_frames_ctx);
    }
}

}

QT_END_NAMESPACE
