// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_d3d11_p.h"
#include "playbackengine/qffmpegstreamdecoder_p.h"

#include <qvideoframeformat.h>
#include "qffmpegvideobuffer_p.h"

#include <private/qvideotexturehelper_p.h>
#include <private/qcomptr_p.h>
#include <private/quniquehandle_p.h>

#include <rhi/qrhi.h>

#include <qopenglfunctions.h>
#include <qdebug.h>
#include <qloggingcategory.h>

#include <libavutil/hwcontext_d3d11va.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>

QT_BEGIN_NAMESPACE

namespace {

Q_LOGGING_CATEGORY(qLcMediaFFmpegHWAccel, "qt.multimedia.hwaccel");

ComPtr<ID3D11Device1> GetD3DDevice(QRhi *rhi)
{
    const auto native = static_cast<const QRhiD3D11NativeHandles *>(rhi->nativeHandles());
    if (!native)
        return {};

    const ComPtr<ID3D11Device> rhiDevice = static_cast<ID3D11Device *>(native->dev);

    ComPtr<ID3D11Device1> dev1;
    if (rhiDevice.As(&dev1) != S_OK)
        return nullptr;

    return dev1;
}

} // namespace
namespace QFFmpeg {

bool TextureBridge::copyToSharedTex(ID3D11Device *dev, ID3D11DeviceContext *ctx,
                                    const ComPtr<ID3D11Texture2D> &tex, UINT index,
                                    const QSize &frameSize)
{
    if (!ensureSrcTex(dev, tex, frameSize))
        return false;

    // Flush to ensure that texture is fully updated before we share it.
    ctx->Flush();

    if (m_srcMutex->AcquireSync(m_srcKey, INFINITE) != S_OK)
        return false;

    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());

    // A crop box is needed because FFmpeg may have created textures
    // that are bigger than the frame size to account for the decoder's
    // surface alignment requirements.
    const D3D11_BOX crop{ 0, 0, 0, width, height, 1 };
    ctx->CopySubresourceRegion(m_srcTex.Get(), 0, 0, 0, 0, tex.Get(), index, &crop);

    m_srcMutex->ReleaseSync(m_destKey);
    return true;
}

ComPtr<ID3D11Texture2D> TextureBridge::copyFromSharedTex(const ComPtr<ID3D11Device1> &dev,
                                                         const ComPtr<ID3D11DeviceContext> &ctx)
{
    if (!ensureDestTex(dev))
        return {};

    if (m_destMutex->AcquireSync(m_destKey, INFINITE) != S_OK)
        return {};

    ctx->CopySubresourceRegion(m_outputTex.Get(), 0, 0, 0, 0, m_destTex.Get(), 0, nullptr);

    m_destMutex->ReleaseSync(m_srcKey);

    return m_outputTex;
}

bool TextureBridge::ensureDestTex(const ComPtr<ID3D11Device1> &dev)
{
    if (m_destDevice != dev) {
        // Destination device changed. Recreate texture.
        m_destTex = nullptr;
        m_destDevice = dev;
    }

    if (m_destTex)
        return true;

    if (m_destDevice->OpenSharedResource1(m_sharedHandle.get(), IID_PPV_ARGS(&m_destTex)) != S_OK)
        return false;

    CD3D11_TEXTURE2D_DESC desc{};
    m_destTex->GetDesc(&desc);

    desc.MiscFlags = 0;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    if (m_destDevice->CreateTexture2D(&desc, nullptr, m_outputTex.ReleaseAndGetAddressOf()) != S_OK)
        return false;

    if (m_destTex.As(&m_destMutex) != S_OK)
        return false;

    return true;
}

bool TextureBridge::ensureSrcTex(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex, const QSize &frameSize)
{
    if (!isSrcInitialized(dev, tex, frameSize))
        return recreateSrc(dev, tex, frameSize);

    return true;
}

bool TextureBridge::isSrcInitialized(const ID3D11Device *dev,
                                     const ComPtr<ID3D11Texture2D> &tex,
                                     const QSize &frameSize) const
{
    if (!m_srcTex)
        return false;

    // Check if device has changed
    ComPtr<ID3D11Device> texDevice;
    m_srcTex->GetDevice(texDevice.GetAddressOf());
    if (dev != texDevice.Get())
        return false;

    // Check if shared texture has correct size and format
    CD3D11_TEXTURE2D_DESC inputDesc{};
    tex->GetDesc(&inputDesc);

    CD3D11_TEXTURE2D_DESC currentDesc{};
    m_srcTex->GetDesc(&currentDesc);

    if (inputDesc.Format != currentDesc.Format)
        return false;

    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());

    if (currentDesc.Width != width || currentDesc.Height != height)
        return false;

    return true;
}

bool TextureBridge::recreateSrc(ID3D11Device *dev, const ComPtr<ID3D11Texture2D> &tex, const QSize &frameSize)
{
    m_sharedHandle.close();

    CD3D11_TEXTURE2D_DESC desc{};
    tex->GetDesc(&desc);

    const UINT width = static_cast<UINT>(frameSize.width());
    const UINT height = static_cast<UINT>(frameSize.height());

    CD3D11_TEXTURE2D_DESC texDesc{ desc.Format, width, height };
    texDesc.MipLevels = 1;
    texDesc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;

    if (dev->CreateTexture2D(&texDesc, nullptr, m_srcTex.ReleaseAndGetAddressOf()) != S_OK)
        return false;

    ComPtr<IDXGIResource1> res;
    if (m_srcTex.As(&res) != S_OK)
        return false;

    const HRESULT hr =
            res->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ, nullptr, &m_sharedHandle);

    if (hr != S_OK || !m_sharedHandle)
        return false;

    if (m_srcTex.As(&m_srcMutex) != S_OK || !m_srcMutex)
        return false;

    m_destTex = nullptr;
    m_destMutex = nullptr;
    return true;
}

class D3D11TextureSet : public TextureSet
{
public:
    D3D11TextureSet(QRhi *rhi, ComPtr<ID3D11Texture2D> &&tex)
        : m_owner{ rhi }, m_tex(std::move(tex))
    {
    }

    qint64 textureHandle(QRhi *rhi, int /*plane*/) override
    {
        if (rhi != m_owner)
            return 0u;
        return reinterpret_cast<qint64>(m_tex.Get());
    }

private:
    QRhi *m_owner = nullptr;
    ComPtr<ID3D11Texture2D> m_tex;
};

D3D11TextureConverter::D3D11TextureConverter(QRhi *rhi)
    : TextureConverterBackend(rhi), m_rhiDevice{ GetD3DDevice(rhi) }
{
    if (!m_rhiDevice)
        return;

    m_rhiDevice->GetImmediateContext(m_rhiCtx.GetAddressOf());
}

TextureSet *D3D11TextureConverter::getTextures(AVFrame *frame)
{
    if (!m_rhiDevice)
        return nullptr;

    if (!frame || !frame->hw_frames_ctx || frame->format != AV_PIX_FMT_D3D11)
        return nullptr;

    const auto *fCtx = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);
    const auto *ctx = fCtx->device_ctx;

    if (!ctx || ctx->type != AV_HWDEVICE_TYPE_D3D11VA)
        return nullptr;

    const ComPtr<ID3D11Texture2D> ffmpegTex = reinterpret_cast<ID3D11Texture2D *>(frame->data[0]);
    const int index = static_cast<int>(reinterpret_cast<intptr_t>(frame->data[1]));

    if (rhi->backend() == QRhi::D3D11) {
        {
            const auto *avDeviceCtx = static_cast<AVD3D11VADeviceContext *>(ctx->hwctx);

            if (!avDeviceCtx)
                return nullptr;

            // Lock the FFmpeg device context while we copy from FFmpeg's
            // frame pool into a shared texture because the underlying ID3D11DeviceContext
            // is not thread safe.
            avDeviceCtx->lock(avDeviceCtx->lock_ctx);
            QScopeGuard autoUnlock([&] { avDeviceCtx->unlock(avDeviceCtx->lock_ctx); });

            // Populate the shared texture with one slice from the frame pool, cropping away
            // extra surface alignment areas that FFmpeg adds to the textures
            QSize frameSize{ frame->width, frame->height };
            if (!m_bridge.copyToSharedTex(avDeviceCtx->device, avDeviceCtx->device_context,
                                          ffmpegTex, index, frameSize)) {
                return nullptr;
            }
        }

        // Get a copy of the texture on the RHI device
        ComPtr<ID3D11Texture2D> output = m_bridge.copyFromSharedTex(m_rhiDevice, m_rhiCtx);

        if (!output)
            return nullptr;

        return new D3D11TextureSet(rhi, std::move(output));
    }

    return nullptr;
}

void D3D11TextureConverter::SetupDecoderTextures(AVCodecContext *s)
{
    // We are holding pool frames alive for quite long, which may cause
    // codecs to run out of frames because FFmpeg has a fixed size
    // decoder frame pool. We must therefore add extra frames to the pool
    // to account for the frames we keep alive. First, we need to account
    // for the maximum number of queued frames during rendering. In
    // addition, we add one frame for the RHI rendering pipeline, and one
    // additional frame because we may hold one in the Qt event loop.

    const qint32 maxRenderQueueSize = StreamDecoder::maxQueueSize(QPlatformMediaPlayer::VideoStream);
    constexpr qint32 framesHeldByRhi = 1;
    constexpr qint32 framesHeldByQtEventLoop = 1;
    s->extra_hw_frames = maxRenderQueueSize + framesHeldByRhi + framesHeldByQtEventLoop;

    int ret = avcodec_get_hw_frames_parameters(s, s->hw_device_ctx, AV_PIX_FMT_D3D11,
                                               &s->hw_frames_ctx);
    if (ret < 0) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to allocate HW frames context" << ret;
        return;
    }

    const auto *frames_ctx = reinterpret_cast<const AVHWFramesContext *>(s->hw_frames_ctx->data);
    auto *hwctx = static_cast<AVD3D11VAFramesContext *>(frames_ctx->hwctx);
    hwctx->MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    hwctx->BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
    ret = av_hwframe_ctx_init(s->hw_frames_ctx);
    if (ret < 0) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to initialize HW frames context" << ret;
        av_buffer_unref(&s->hw_frames_ctx);
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE
