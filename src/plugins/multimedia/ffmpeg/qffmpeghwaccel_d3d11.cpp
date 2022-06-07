// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_d3d11_p.h"

#include <qvideoframeformat.h>
#include "qffmpegvideobuffer_p.h"


#include <private/qvideotexturehelper_p.h>
#include <private/qrhi_p.h>
#include <private/qrhid3d11_p.h>

#include <qopenglfunctions.h>
#include <qdebug.h>
#include <qloggingcategory.h>

#include <libavutil/hwcontext_d3d11va.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(qLcMediaFFmpegHWAccel, "qt.multimedia.hwaccel")

namespace QFFmpeg {

class D3D11TextureSet : public TextureSet
{
public:
    D3D11TextureSet(QRhi *rhi, QVideoFrameFormat::PixelFormat format, ID3D11Texture2D *tex, int index)
        : m_rhi(rhi)
        , m_format(format)
        , m_tex(tex)
        , m_index(index)
    {}

    ~D3D11TextureSet() override {
        if (m_tex)
            m_tex->Release();
    }

    std::unique_ptr<QRhiTexture> texture(int plane) override {
        auto desc = QVideoTextureHelper::textureDescription(m_format);
        if (!m_tex || !m_rhi || !desc || plane >= desc->nplanes)
            return {};

        D3D11_TEXTURE2D_DESC d3d11desc = {};
        m_tex->GetDesc(&d3d11desc);

        QSize planeSize(desc->widthForPlane(int(d3d11desc.Width), plane),
                        desc->heightForPlane(int(d3d11desc.Height), plane));

        std::unique_ptr<QRhiTexture> tex(m_rhi->newTextureArray(desc->textureFormat[plane],
                                                                int(d3d11desc.ArraySize),
                                                                planeSize, 1, {}));
        if (tex) {
            tex->setArrayRange(m_index, 1);
            if (!tex->createFrom({quint64(m_tex), 0}))
                tex.reset();
        }
        return tex;
    }

private:
    QRhi *m_rhi = nullptr;
    QVideoFrameFormat::PixelFormat m_format;
    ID3D11Texture2D *m_tex = nullptr;
    int m_index = 0;
};


D3D11TextureConverter::D3D11TextureConverter(QRhi *rhi)
    : TextureConverterBackend(rhi)
{
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

    auto dev = reinterpret_cast<ID3D11Device *>(nh->dev);
    if (!dev)
        return nullptr;

    auto ffmpegTex = (ID3D11Texture2D *)frame->data[0];
    int index = (intptr_t)frame->data[1];

    IDXGIResource *dxgiResource = nullptr;
    HRESULT hr = ffmpegTex->QueryInterface(__uuidof(IDXGIResource), (void **)&dxgiResource);
    if (FAILED(hr)) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to obtain resource handle from FFMpeg texture" << hr;
        return nullptr;
    }
    HANDLE shared = nullptr;
    hr = dxgiResource->GetSharedHandle(&shared);
    dxgiResource->Release();
    if (FAILED(hr)) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to obtain shared handle for FFmpeg texture" << hr;
        return nullptr;
    }

    if (rhi->backend() == QRhi::D3D11) {
        ID3D11Texture2D *sharedTex = nullptr;
        hr = dev->OpenSharedResource(shared, __uuidof(ID3D11Texture2D), (void **)(&sharedTex));
        if (FAILED(hr)) {
            qCDebug(qLcMediaFFmpegHWAccel) << "Failed to share FFmpeg texture" << hr;
            return nullptr;
        }

        QVideoFrameFormat::PixelFormat format = QFFmpegVideoBuffer::toQtPixelFormat(AVPixelFormat(fCtx->sw_format));
        return new D3D11TextureSet(rhi, format, sharedTex, index);
    } else if (rhi->backend() == QRhi::OpenGLES2) {

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
