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

#include "qffmpeghwaccel_d3d11_p.h"

#include <qvideoframeformat.h>
#include "qffmpegvideobuffer_p.h"


#include <private/qvideotexturehelper_p.h>
#include <private/qwindowsiupointer_p.h>
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
    D3D11TextureSet(QRhi *rhi, QVideoFrameFormat::PixelFormat format, QWindowsIUPointer<ID3D11Texture2D> &&tex)
        : m_rhi(rhi)
        , m_format(format)
        , m_tex(tex)
    {}

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
            if (!tex->createFrom({quint64(m_tex.get()), 0}))
                tex.reset();
        }
        return tex;
    }

private:
    QRhi *m_rhi = nullptr;
    QVideoFrameFormat::PixelFormat m_format;
    QWindowsIUPointer<ID3D11Texture2D> m_tex;
};


D3D11TextureConverter::D3D11TextureConverter(QRhi *rhi)
    : TextureConverterBackend(rhi)
{
}

static QWindowsIUPointer<ID3D11Texture2D> getSharedTextureForDevice(ID3D11Device *dev, ID3D11Texture2D *tex)
{
    QWindowsIUPointer<IDXGIResource> dxgiResource;
    HRESULT hr = tex->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void **>(dxgiResource.address()));
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

    QWindowsIUPointer<ID3D11Texture2D> sharedTex;
    hr = dev->OpenSharedResource(shared, __uuidof(ID3D11Texture2D), reinterpret_cast<void **>(sharedTex.address()));
    if (FAILED(hr))
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to share FFmpeg texture" << hr;
    return sharedTex;
}

static QWindowsIUPointer<ID3D11Texture2D> copyTextureFromArray(ID3D11Device *dev, ID3D11Texture2D *array, int index)
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

    QWindowsIUPointer<ID3D11Texture2D> texCopy;
    HRESULT hr = dev->CreateTexture2D(&texDesc, nullptr, texCopy.address());
    if (FAILED(hr)) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to create texture" << hr;
        return {};
    }

    QWindowsIUPointer<ID3D11DeviceContext> ctx;
    dev->GetImmediateContext(ctx.address());
    ctx->CopySubresourceRegion(texCopy.get(), 0, 0, 0, 0, array, index, nullptr);

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

    auto dev = reinterpret_cast<ID3D11Device *>(nh->dev);
    if (!dev)
        return nullptr;

    auto ffmpegTex = (ID3D11Texture2D *)frame->data[0];
    int index = (intptr_t)frame->data[1];

    auto sharedTex = getSharedTextureForDevice(dev, ffmpegTex);
    if (sharedTex) {
        auto tex = copyTextureFromArray(dev, sharedTex.get(), index);
        if (tex) {
            if (rhi->backend() == QRhi::D3D11) {
                QVideoFrameFormat::PixelFormat format = QFFmpegVideoBuffer::toQtPixelFormat(AVPixelFormat(fCtx->sw_format));
                return new D3D11TextureSet(rhi, format, std::move(tex));
            } else if (rhi->backend() == QRhi::OpenGLES2) {

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
