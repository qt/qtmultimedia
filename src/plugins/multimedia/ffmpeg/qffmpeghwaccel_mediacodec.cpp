// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_mediacodec_p.h"

#include "androidsurfacetexture_p.h"
#include <rhi/qrhi.h>

extern "C" {
#include <libavcodec/mediacodec.h>
#include <libavutil/hwcontext_mediacodec.h>
}

#if !defined(Q_OS_ANDROID)
#    error "Configuration error"
#endif

namespace QFFmpeg {

namespace {

class MediaCodecTextureHandles : public QVideoFrameTexturesHandles
{
public:
    MediaCodecTextureHandles(TextureConverterBackendPtr &&converterBackend, quint64 textureHandle)
        : m_parentConverterBackend(std::move(converterBackend)), m_handle(textureHandle)
    {
    }

    quint64 textureHandle(QRhi &, int plane) override { return (plane == 0) ? m_handle : 0; }

private:
    TextureConverterBackendPtr m_parentConverterBackend; // ensures the backend is kept
    quint64 m_handle;
};

void deleteSurface(AVHWDeviceContext *ctx)
{
    AndroidSurfaceTexture* s = reinterpret_cast<AndroidSurfaceTexture *>(ctx->user_opaque);
    delete s;
}

AndroidSurfaceTexture* getTextureSurface(AVFrame *frame)
{
    if (!frame || !frame->hw_frames_ctx)
        return nullptr;

    auto *frameContext = reinterpret_cast<AVHWFramesContext *>(frame->hw_frames_ctx->data);

    if (!frameContext || !frameContext->device_ctx)
        return nullptr;

    AVHWDeviceContext *deviceContext = frameContext->device_ctx;

    return reinterpret_cast<AndroidSurfaceTexture *>(deviceContext->user_opaque);
}
} // namespace

void MediaCodecTextureConverter::setupDecoderSurface(AVCodecContext *avCodecContext)
{
    std::unique_ptr<AndroidSurfaceTexture> androidSurfaceTexture(new AndroidSurfaceTexture(0));
    AVMediaCodecContext *mediacodecContext = av_mediacodec_alloc_context();
    av_mediacodec_default_init(avCodecContext, mediacodecContext, androidSurfaceTexture->surface());

    if (!avCodecContext->hw_device_ctx || !avCodecContext->hw_device_ctx->data)
        return;

    AVHWDeviceContext *deviceContext =
            reinterpret_cast<AVHWDeviceContext *>(avCodecContext->hw_device_ctx->data);

    if (!deviceContext->hwctx)
        return;

    AVMediaCodecDeviceContext *mediaDeviceContext =
            reinterpret_cast<AVMediaCodecDeviceContext *>(deviceContext->hwctx);

    if (!mediaDeviceContext)
        return;

    mediaDeviceContext->surface = androidSurfaceTexture->surface();

    Q_ASSERT(deviceContext->user_opaque == nullptr);
    deviceContext->user_opaque = androidSurfaceTexture.release();
    deviceContext->free = deleteSurface;
}

QVideoFrameTexturesUPtr
MediaCodecTextureConverter::createTextures(AVFrame * /*frame*/,
                                           QVideoFrameTexturesUPtr & /*oldTextures*/)
{
    // Most likely, this method should be used instead of createTextureHandles, QRhiTexture is
    // already created, and we don't need to convert it to handle and back. This work should be done
    // in the scope of QTBUG-132174
    return nullptr;
}

QVideoFrameTexturesHandlesUPtr
MediaCodecTextureConverter::createTextureHandles(AVFrame *frame,
                                                 QVideoFrameTexturesHandlesUPtr /*oldHandles*/)
{
    // TODO: reuse oldHandles: the underlying QRhiTexture must be taken from it instead of
    // keeping it in AndroidTextureConverter. See QTBUG-132174
    AndroidSurfaceTexture * androidSurfaceTexture = getTextureSurface(frame);

    if (!androidSurfaceTexture || !androidSurfaceTexture->isValid())
        return {};

    if (!externalTexture || m_currentSurfaceIndex != androidSurfaceTexture->index()) {
        m_currentSurfaceIndex = androidSurfaceTexture->index();
        androidSurfaceTexture->detachFromGLContext();
        externalTexture = std::unique_ptr<QRhiTexture>(
                rhi->newTexture(QRhiTexture::Format::RGBA8, { frame->width, frame->height }, 1,
                                QRhiTexture::ExternalOES));

        if (!externalTexture->create()) {
            qWarning() << "Failed to create the external texture!";
            return {};
        }

        quint64 textureHandle = externalTexture->nativeTexture().object;
        androidSurfaceTexture->attachToGLContext(textureHandle);
    }

    // release a MediaCodec buffer and render it to the surface
    AVMediaCodecBuffer *buffer = (AVMediaCodecBuffer *)frame->data[3];

    if (!buffer) {
        qWarning() << "Received a frame without AVMediaCodecBuffer.";
    } else if (av_mediacodec_release_buffer(buffer, 1) < 0) {
        qWarning() << "Failed to render buffer to surface.";
        return {};
    }

    androidSurfaceTexture->updateTexImage();

    return std::make_unique<MediaCodecTextureHandles>(shared_from_this(),
                                                      externalTexture->nativeTexture().object);
}
}
