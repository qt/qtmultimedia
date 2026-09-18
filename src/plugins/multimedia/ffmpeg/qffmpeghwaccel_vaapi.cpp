// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_vaapi_p.h"

#include <QtFFmpegMediaPluginImpl/private/qffmpegvideobuffer_p.h>

#if QT_CONFIG(linux_dmabuf_egl)
#  include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#endif
#if QT_CONFIG(linux_dmabuf_vulkan)
#  include <QtMultimedia/private/qdmabufvulkanimporter_p.h>
#endif
#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qmultimedia_va_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtCore/qloggingcategory.h>

extern "C" {
#include <libavutil/hwcontext_vaapi.h>
}

#include <va/va.h>
#include <va/va_drmcommon.h>

#include <unistd.h>

static_assert(QT_CONFIG(vaapi));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLHWAccelVAAPI, "qt.multimedia.ffmpeg.hwaccelvaapi");

namespace QFFmpeg {

namespace {

constexpr bool VAExportUseLayers = false;

// Backend-agnostic export-and-import of an AV_PIX_FMT_VAAPI frame's VA surface, parameterized by
// the dma-buf import context/function so the actual GL/Vulkan interop stays in the caller.
template <typename Context, typename ImportFunc>
QVideoFrameTexturesHandlesUPtr
createVaapiTextureHandles(AVFrame *frame, TextureConverterBackend &converter,
                          const Context &context, ImportFunc &&importPlanes)
{
    QRhi &rhi = *converter.rhi;

    using namespace QtMultimediaPrivate;
    using VAStatus = QtMultimediaPrivate::VAStatus;

    if (frame->format != AV_PIX_FMT_VAAPI || !frame->hw_frames_ctx)
        return nullptr;

    auto *ctx = avFrameDeviceContext(frame);
    if (!ctx)
        return nullptr;

    auto *vaCtx = (AVVAAPIDeviceContext *)ctx->hwctx;
    auto vaDisplay = vaCtx->display;
    if (!vaDisplay) {
        qCDebug(qLHWAccelVAAPI) << "    no VADisplay, disabling";
        return nullptr;
    }

    VASurfaceID vaSurface = (uintptr_t)frame->data[3];

    VADRMPRIMESurfaceDescriptor prime = {};
    const auto exportStatus =
            vaExportSurfaceHandle(vaDisplay, vaSurface, VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                                  VA_EXPORT_SURFACE_READ_ONLY
                                          | (VAExportUseLayers ? VA_EXPORT_SURFACE_SEPARATE_LAYERS
                                                               : VA_EXPORT_SURFACE_COMPOSED_LAYERS),
                                  &prime);
    if (exportStatus != VA_STATUS_SUCCESS) {
        qWarning() << "vaExportSurfaceHandle failed:" << VAStatus(exportStatus);
        return nullptr;
    }

    // Make sure all fd's in 'prime' are closed when we return from this function, i.e. after
    // importPlanes() below has dup()'d the ones it needs.
    QScopeGuard closeObjectsGuard([&prime]() {
        for (uint32_t i = 0; i < prime.num_objects; ++i)
            close(prime.objects[i].fd);
    });

    // ### Check that prime.fourcc is what we expect
    vaSyncSurface(vaDisplay, vaSurface);

    AVPixelFormat fmt = HWAccel::format(frame);
    bool needsConversion;
    auto qtFormat = QFFmpegVideoBuffer::toQtPixelFormat(fmt, &needsConversion);
    QSpan<const DRMFormat> drm_formats = dmaBufFourccFromPixelFormat(qtFormat);
    if (drm_formats.empty() || needsConversion) {
        qWarning() << "can't use DMA transfer for pixel format" << fmt << qtFormat;
        return nullptr;
    }

    auto *desc = QVideoTextureHelper::textureDescription(qtFormat);
    int nPlanes = int(drm_formats.size());
    Q_ASSERT(nPlanes == desc->nplanes);
    nPlanes = desc->nplanes;

    std::array<DmaBufPlane, 4> planes;
    for (int i = 0; i < nPlanes; ++i) {
        const int layer = VAExportUseLayers ? i : 0;
        const int plane = VAExportUseLayers ? 0 : i;

        if constexpr (VAExportUseLayers) {
            if (prime.layers[i].drm_format != quint32(drm_formats[i])) {
                qWarning() << "expected DRM format check failed expected" << Qt::hex
                           << drm_formats[i] << "got" << prime.layers[i].drm_format;
            }
        }

        planes[i].fd = prime.objects[prime.layers[layer].object_index[plane]].fd;
        planes[i].offset = prime.layers[layer].offset[plane];
        planes[i].pitch = prime.layers[layer].pitch[plane];
        planes[i].drmFormat = drm_formats[i];
        planes[i].modifier = DRMModifier{
            prime.objects[prime.layers[layer].object_index[plane]].drm_format_modifier
        };
    }

    auto textureHandles = importPlanes(rhi, context, QSpan(planes.data(), nPlanes), qtFormat,
                                       frameSize(*frame), converter.shared_from_this());
    if (!textureHandles) {
        if (textureHandles.error() == FailureSeverity::unrecoverable) {
            qWarning() << "VAAPI driver:" << vaQueryVendorString(vaDisplay);
            converter.rhi = nullptr; // Disabling texture conversion here to fix QTBUG-112312
        }
        return nullptr;
    }
    return std::move(*textureHandles);
}

#if QT_CONFIG(linux_dmabuf_egl)
class VAAPIGLTextureConverter final : public TextureConverterBackend
{
public:
    static std::shared_ptr<VAAPIGLTextureConverter> create(QRhi *rhi)
    {
        auto converter = std::shared_ptr<VAAPIGLTextureConverter>(new VAAPIGLTextureConverter(rhi));
        if (!converter->m_context.isValid())
            return nullptr;
        return converter;
    }

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr /*oldHandles*/) override
    {
        return createVaapiTextureHandles(frame, *this, m_context,
                                         [](QRhi &rhi,
                                            const QtMultimediaPrivate::DmaBufEglContext &eglContext,
                                            QSpan<const QtMultimediaPrivate::DmaBufPlane> planes,
                                            QVideoFrameFormat::PixelFormat qtFormat, QSize size,
                                            std::shared_ptr<void> parentKeepAlive) {
            return importDmaBufTextures(rhi, eglContext, planes, qtFormat, size,
                                        std::move(parentKeepAlive));
        });
    }

private:
    explicit VAAPIGLTextureConverter(QRhi *rhi) : TextureConverterBackend(rhi), m_context(rhi) { }

    QtMultimediaPrivate::DmaBufEglContext m_context;
};
#endif // QT_CONFIG(linux_dmabuf_egl)

#if QT_CONFIG(linux_dmabuf_vulkan)
class VAAPIVulkanTextureConverter final : public TextureConverterBackend
{
public:
    static std::shared_ptr<VAAPIVulkanTextureConverter> create(QRhi *rhi)
    {
        auto converter =
                std::shared_ptr<VAAPIVulkanTextureConverter>(new VAAPIVulkanTextureConverter(rhi));
        if (!converter->m_context.isValid())
            return nullptr;

        return converter;
    }

    QVideoFrameTexturesHandlesUPtr
    createTextureHandles(AVFrame *frame, QVideoFrameTexturesHandlesUPtr /*oldHandles*/) override
    {
        return createVaapiTextureHandles(frame, *this, m_context,
                                         QtMultimediaPrivate::importDmaBufTexturesVulkan);
    }

private:
    explicit VAAPIVulkanTextureConverter(QRhi *rhi) : TextureConverterBackend(rhi), m_context(rhi)
    {
    }

    QtMultimediaPrivate::DmaBufVulkanContext m_context;
};
#endif // QT_CONFIG(linux_dmabuf_vulkan)

} // namespace

std::shared_ptr<TextureConverterBackend> VAAPITextureConverterFactory::create(QRhi *rhi)
{
    if (rhi) {
        switch (rhi->backend()) {
#if QT_CONFIG(linux_dmabuf_egl)
        case QRhi::OpenGLES2:
            return VAAPIGLTextureConverter::create(rhi);
#endif
#if QT_CONFIG(linux_dmabuf_vulkan)
        case QRhi::Vulkan:
            return VAAPIVulkanTextureConverter::create(rhi);
#endif
        default:
            break;
        }
    }
    return nullptr;
}

} // namespace QFFmpeg

QT_END_NAMESPACE
