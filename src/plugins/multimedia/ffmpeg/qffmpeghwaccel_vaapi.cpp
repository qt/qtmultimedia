// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_vaapi_p.h"

#include "qffmpegvideobuffer_p.h"

#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qpa/qplatformnativeinterface.h>

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

using QtMultimediaPrivate::DRMFormat;

constexpr bool VAExportUseLayers = false;

VAAPITextureConverter::VAAPITextureConverter(QRhi *rhi)
    : TextureConverterBackend(nullptr), eglContext(rhi)
{
    qCDebug(qLHWAccelVAAPI) << ">>>> Creating VAAPI HW accelerator";

    if (!eglContext.isValid()) {
        this->rhi = nullptr;
        return;
    }

    // everything ok, indicate that we can do zero copy
    this->rhi = rhi;
}

VAAPITextureConverter::~VAAPITextureConverter() = default;

QVideoFrameTexturesHandlesUPtr
VAAPITextureConverter::createTextureHandles(AVFrame *frame,
                                            QVideoFrameTexturesHandlesUPtr /*oldHandles*/)
{
    //        qCDebug(qLHWAccelVAAPI) << "VAAPIAccel::createTextureHandles";
    if (frame->format != AV_PIX_FMT_VAAPI || !eglContext.eglDisplay()) {
        qCDebug(qLHWAccelVAAPI) << "format/egl error" << frame->format << eglContext.eglDisplay();
        return nullptr;
    }

    if (!frame->hw_frames_ctx)
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
    if (vaExportSurfaceHandle(vaDisplay, vaSurface, VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                              VA_EXPORT_SURFACE_READ_ONLY
                                      | (VAExportUseLayers ? VA_EXPORT_SURFACE_SEPARATE_LAYERS
                                                           : VA_EXPORT_SURFACE_COMPOSED_LAYERS),
                              &prime)
        != VA_STATUS_SUCCESS) {
        qWarning() << "vaExportSurfaceHandle failed";
        return nullptr;
    }

    // Make sure all fd's in 'prime' are closed when we return from this function
    QScopeGuard closeObjectsGuard([&prime]() {
        for (uint32_t i = 0;  i < prime.num_objects;  ++i)
            close(prime.objects[i].fd);
    });

    // ### Check that prime.fourcc is what we expect
    vaSyncSurface(vaDisplay, vaSurface);

//        qCDebug(qLHWAccelVAAPI) << "VAAPIAccel: vaSufraceDesc: width/height" << prime.width << prime.height << "num objects"
//                 << prime.num_objects << "num layers" << prime.num_layers;

    AVPixelFormat fmt = HWAccel::format(frame);
    bool needsConversion;
    auto qtFormat = QFFmpegVideoBuffer::toQtPixelFormat(fmt, &needsConversion);
    QSpan<const DRMFormat> drm_formats = QtMultimediaPrivate::dmaBufFourccFromPixelFormat(qtFormat);
    if (drm_formats.empty() || needsConversion) {
        qWarning() << "can't use DMA transfer for pixel format" << fmt << qtFormat;
        return nullptr;
    }

    auto *desc = QVideoTextureHelper::textureDescription(qtFormat);
    int nPlanes = int(drm_formats.size());
    Q_ASSERT(nPlanes == desc->nplanes);
    nPlanes = desc->nplanes;
//        qCDebug(qLHWAccelVAAPI) << "VAAPIAccel: nPlanes" << nPlanes;

    DmaBufPlane planes[4];
    for (int i = 0;  i < nPlanes;  ++i) {
        const int layer = VAExportUseLayers ? i : 0;
        const int plane = VAExportUseLayers ? 0 : i;

        if constexpr (VAExportUseLayers) {
            if (prime.layers[i].drm_format != quint32(drm_formats[i])) {
                qWarning() << "expected DRM format check failed expected" << Qt::hex
                           << quint32(drm_formats[i]) << "got" << prime.layers[i].drm_format;
            }
        }

        planes[i].fd = prime.objects[prime.layers[layer].object_index[plane]].fd;
        planes[i].offset = prime.layers[layer].offset[plane];
        planes[i].pitch = prime.layers[layer].pitch[plane];
        planes[i].drmFormat = drm_formats[i];
        planes[i].modifier =
                prime.objects[prime.layers[layer].object_index[plane]].drm_format_modifier;
    }

    auto textureHandles =
            importDmaBufTextures(*rhi, eglContext, QSpan(planes, nPlanes), qtFormat,
                                 QSize(frame->width, frame->height), shared_from_this());
    if (!textureHandles) {
        if (textureHandles.error() == FailureSeverity::unrecoverable) {
            qWarning() << "VAAPI driver:" << vaQueryVendorString(vaDisplay);
            this->rhi = nullptr; // Disabling texture conversion here to fix QTBUG-112312
        }
        return nullptr;
    }

    return std::move(*textureHandles);
}

} // namespace QFFmpeg

QT_END_NAMESPACE
