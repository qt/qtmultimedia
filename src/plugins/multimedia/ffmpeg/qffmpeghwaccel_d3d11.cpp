// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_d3d11_p.h"

#include "qffmpegvideobuffer_p.h"

#include <QtMultimedia/qvideoframeformat.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/private/qcomptr_p.h>

#include <libavutil/hwcontext_d3d11va.h>
#include <d3d11_1.h>

#include <mutex>

namespace std {

template <>
class lock_guard<AVD3D11VADeviceContext>
{
public:
    explicit lock_guard(const AVD3D11VADeviceContext &ctx) : m_ctx(ctx)
    {
        m_ctx.lock(m_ctx.lock_ctx);
    }

    ~lock_guard() { m_ctx.unlock(m_ctx.lock_ctx); }

    lock_guard(const lock_guard &) = delete;
    lock_guard &operator=(const lock_guard &) = delete;

private:
    const AVD3D11VADeviceContext &m_ctx;
};

} // namespace std

QT_BEGIN_NAMESPACE

namespace QFFmpeg {

namespace {

Q_LOGGING_CATEGORY(qLcMediaFFmpegHWAccel, "qt.multimedia.hwaccel");

ComPtr<ID3D11Texture2D> getAvFrameTexture(const AVFrame *frame)
{
    return reinterpret_cast<ID3D11Texture2D *>(frame->data[0]);
}

int getAvFramePoolIndex(const AVFrame *frame)
{
    return static_cast<int>(reinterpret_cast<intptr_t>(frame->data[1]));
}

const AVD3D11VADeviceContext *getHwDeviceContext(const AVHWDeviceContext *ctx)
{
    return static_cast<AVD3D11VADeviceContext *>(ctx->hwctx);
}

AVBufferRef *wrapTextureAsBuffer(const ComPtr<ID3D11Texture2D> &tex)
{
    Q_ASSERT(tex);

    AVD3D11FrameDescriptor *avFrameDesc =
            static_cast<AVD3D11FrameDescriptor *>(av_mallocz(sizeof(AVD3D11FrameDescriptor)));
    avFrameDesc->index = 0;
    avFrameDesc->texture = tex.Get();

    return av_buffer_create(reinterpret_cast<uint8_t *>(avFrameDesc),
                            sizeof(AVD3D11FrameDescriptor *), [](void *opaque, uint8_t *data) {
        static_cast<ID3D11Texture2D *>(opaque)->Release();
        av_free(data);
    }, tex.Get(), 0);
}

ComPtr<ID3D11Texture2D> copyTexture(const AVD3D11VADeviceContext *hwDevCtx, const AVFrame *src)
{
    const int poolIndex = getAvFramePoolIndex(src);
    const ComPtr<ID3D11Texture2D> poolTex = getAvFrameTexture(src);

    D3D11_TEXTURE2D_DESC texDesc{};
    poolTex->GetDesc(&texDesc);

    texDesc.ArraySize = 1;
    texDesc.MiscFlags = 0;
    texDesc.BindFlags = 0;

    ComPtr<ID3D11Texture2D> destTex;
    if (hwDevCtx->device->CreateTexture2D(&texDesc, nullptr, &destTex) != S_OK) {
        qCCritical(qLcMediaFFmpegHWAccel) << "Unable to copy frame from decoder pool";
        return {};
    }

    hwDevCtx->device_context->CopySubresourceRegion(destTex.Get(), 0, 0, 0, 0, poolTex.Get(),
                                                    poolIndex, nullptr);

    return destTex;
}

class ImportedTextureHandles : public QVideoFrameTexturesHandles
{
public:
    ImportedTextureHandles(TextureConverterBackendPtr &&converterBackend,
                           QVideoFrameTexturesHandlesUPtr &&handles)
        : m_parentConverterBackend(std::move(converterBackend)), m_handles(std::move(handles))
    {
    }

    quint64 textureHandle(QRhi &rhi, int plane) override
    {
        return m_handles ? m_handles->textureHandle(rhi, plane) : 0;
    }

private:
    TextureConverterBackendPtr
            m_parentConverterBackend; // ensures the backend is deleted after the texture
    QVideoFrameTexturesHandlesUPtr m_handles;
};

} // namespace

std::shared_ptr<D3D11TextureConverter> D3D11TextureConverter::create(QRhi *rhi)
{
    if (!rhi || !QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi))
        return nullptr;

    return std::shared_ptr<D3D11TextureConverter>(new D3D11TextureConverter(rhi));
}

D3D11TextureConverter::D3D11TextureConverter(QRhi *rhi) : TextureConverterBackend(rhi) { }

QVideoFrameTexturesHandlesUPtr
D3D11TextureConverter::createTextureHandles(AVFrame *frame,
                                            QVideoFrameTexturesHandlesUPtr /*oldHandles*/)
{
    if (!frame || !frame->hw_frames_ctx || frame->format != AV_PIX_FMT_D3D11)
        return nullptr;

    const auto *ctx = avFrameDeviceContext(frame);

    if (!ctx || ctx->type != AV_HWDEVICE_TYPE_D3D11VA)
        return nullptr;

    const auto *avDeviceCtx = getHwDeviceContext(ctx);
    if (!avDeviceCtx)
        return nullptr;

    Q_ASSERT(QWindowsD3D11RhiTextureImporter::isRhiBackendSupported(*rhi));

    const ComPtr<ID3D11Texture2D> ffmpegTex = getAvFrameTexture(frame);
    const int index = getAvFramePoolIndex(frame);

    QVideoFrameTexturesHandlesUPtr handles;
    {
        // Lock the FFmpeg device context while we copy from FFmpeg's
        // frame pool into a shared texture because the underlying ID3D11DeviceContext
        // is not thread safe.
        std::lock_guard<AVD3D11VADeviceContext> guard(*avDeviceCtx);

        // Import a copy of the slice from the frame pool, cropping away extra surface
        // alignment areas that FFmpeg adds to the textures.
        handles = m_importer.importTexture(*rhi, avDeviceCtx->device, avDeviceCtx->device_context,
                                           ffmpegTex, QSize{ frame->width, frame->height }, index);
    }

    if (!handles)
        return nullptr;

    return std::make_unique<ImportedTextureHandles>(shared_from_this(), std::move(handles));
}

void D3D11TextureConverter::SetupDecoderTextures(AVCodecContext *s)
{
    int ret = avcodec_get_hw_frames_parameters(s, s->hw_device_ctx, AV_PIX_FMT_D3D11,
                                               &s->hw_frames_ctx);
    if (ret < 0) {
        qCDebug(qLcMediaFFmpegHWAccel) << "Failed to allocate HW frames context" << AVError{ ret };
        return;
    }

    const auto *frames_ctx = reinterpret_cast<const AVHWFramesContext *>(s->hw_frames_ctx->data);
    auto *hwctx = static_cast<AVD3D11VAFramesContext *>(frames_ctx->hwctx);
    hwctx->MiscFlags = D3D11_RESOURCE_MISC_SHARED;
    hwctx->BindFlags = D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE;
    ret = av_hwframe_ctx_init(s->hw_frames_ctx);
    if (ret < 0) {
        qCDebug(qLcMediaFFmpegHWAccel)
                << "Failed to initialize HW frames context" << AVError{ ret };
        av_buffer_unref(&s->hw_frames_ctx);
    }
}

AVFrameUPtr copyFromHwPoolD3D11(AVFrameUPtr src)
{
    if (!src || !src->hw_frames_ctx || src->format != AV_PIX_FMT_D3D11)
        return src;

    const AVHWDeviceContext *avDevCtx = avFrameDeviceContext(src.get());
    if (!avDevCtx || avDevCtx->type != AV_HWDEVICE_TYPE_D3D11VA)
        return src;

    AVFrameUPtr dest = makeAVFrame();
    if (int status = av_frame_copy_props(dest.get(), src.get()); status != 0) {
        qCCritical(qLcMediaFFmpegHWAccel)
                << "Unable to copy frame from decoder pool" << AVError{ status };
        return src;
    }

    const AVD3D11VADeviceContext *hwDevCtx = getHwDeviceContext(avDevCtx);
    ComPtr<ID3D11Texture2D> destTex = [&] {
        std::lock_guard<AVD3D11VADeviceContext> guard(*hwDevCtx);
        return copyTexture(hwDevCtx, src.get());
    }();

    if (!destTex)
        return src;

    dest->buf[0] = wrapTextureAsBuffer(destTex);
    dest->data[0] = reinterpret_cast<uint8_t *>(destTex.Detach());
    dest->data[1] = reinterpret_cast<uint8_t *>(0); // This texture is not a texture array

    dest->width = src->width;
    dest->height = src->height;
    dest->format = src->format;
    dest->hw_frames_ctx = av_buffer_ref(src->hw_frames_ctx);

    return dest;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
