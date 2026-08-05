// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qffmpeghwaccel_vaapi_p.h"

#if !QT_CONFIG(vaapi)
#error "Configuration error"
#endif

#include <va/va.h>

#include <qvideoframeformat.h>
#include "qffmpegvideobuffer_p.h"
#include "private/qvideotexturehelper_p.h"

#include <rhi/qrhi.h>

#include <qguiapplication.h>
#include <qpa/qplatformnativeinterface.h>

#include <qopenglfunctions.h>

#include <QtMultimedia/private/qmultimedia_drm_support_p.h>

//#define VA_EXPORT_USE_LAYERS

extern "C" {
#include <libavutil/hwcontext_vaapi.h>
}

#include <va/va_drm.h>
#include <va/va_drmcommon.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <unistd.h>

#include <qloggingcategory.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLHWAccelVAAPI, "qt.multimedia.ffmpeg.hwaccelvaapi");

namespace QFFmpeg {

using QtMultimediaPrivate::DRMFormat;

namespace {
class VAAPITextureHandles : public QVideoFrameTexturesHandles
{
public:
    ~VAAPITextureHandles() override;
    quint64 textureHandle(QRhi &, int plane) override {
        return textures[plane];
    }

    TextureConverterBackendPtr parentConverterBackend; // ensures the backend is deleted after the texture
    QRhi *rhi = nullptr;
    QOpenGLContext *glContext = nullptr;
    int nPlanes = 0;
    GLuint textures[4] = {};
};
} // namespace

VAAPITextureConverter::VAAPITextureConverter(QRhi *rhi)
    : TextureConverterBackend(nullptr)
{
    qCDebug(qLHWAccelVAAPI) << ">>>> Creating VAAPI HW accelerator";

    if (!rhi || rhi->backend() != QRhi::OpenGLES2) {
        qWarning() << "VAAPITextureConverter: No rhi or non openGL based RHI";
        this->rhi = nullptr;
        return;
    }

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    glContext = nativeHandles->context;
    if (!glContext) {
        qCDebug(qLHWAccelVAAPI) << "    no GL context, disabling";
        return;
    }
    const QString platform = QGuiApplication::platformName();
    QPlatformNativeInterface *pni = QGuiApplication::platformNativeInterface();
    eglDisplay = pni->nativeResourceForIntegration(QByteArrayLiteral("egldisplay"));
    qCDebug(qLHWAccelVAAPI) << "     platform is" << platform << eglDisplay;
    if (!eglDisplay) {
        qCDebug(qLHWAccelVAAPI) << "    no egl display, disabling";
        return;
    }

    eglImageTargetTexture2D = eglGetProcAddress("glEGLImageTargetTexture2DOES");
    if (!eglImageTargetTexture2D) {
        qCDebug(qLHWAccelVAAPI) << "    no eglImageTargetTexture2D, disabling";
        return;
    }

    // everything ok, indicate that we can do zero copy
    this->rhi = rhi;
}

VAAPITextureConverter::~VAAPITextureConverter() = default;

//#define VA_EXPORT_USE_LAYERS
QVideoFrameTexturesHandlesUPtr
VAAPITextureConverter::createTextureHandles(AVFrame *frame,
                                            QVideoFrameTexturesHandlesUPtr /*oldHandles*/)
{
    //        qCDebug(qLHWAccelVAAPI) << "VAAPIAccel::createTextureHandles";
    if (frame->format != AV_PIX_FMT_VAAPI || !eglDisplay) {
        qCDebug(qLHWAccelVAAPI) << "format/egl error" << frame->format << eglDisplay;
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
    if (vaExportSurfaceHandle(vaDisplay, vaSurface,
                              VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
                              VA_EXPORT_SURFACE_READ_ONLY |
#ifdef VA_EXPORT_USE_LAYERS
                                  VA_EXPORT_SURFACE_SEPARATE_LAYERS,
#else
                                  VA_EXPORT_SURFACE_COMPOSED_LAYERS,
#endif
                              &prime) != VA_STATUS_SUCCESS)
    {
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

    QOpenGLFunctions functions(glContext);

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

    rhi->makeThreadLocalNativeContextCurrent();

    EGLImage images[4] = {};
    GLuint glTextures[4] = {};
    functions.glGenTextures(nPlanes, glTextures);

    auto releaseTextures = qScopeGuard([&] {
        for (EGLImage img : images) {
            if (img)
                eglDestroyImage(eglDisplay, img);
        }
        functions.glDeleteTextures(nPlanes, glTextures);
    });

    for (int i = 0;  i < nPlanes;  ++i) {
#ifdef VA_EXPORT_USE_LAYERS
#define LAYER i
#define PLANE 0
        if (prime.layers[i].drm_format != quint32(drm_formats[i])) {
            qWarning() << "expected DRM format check failed expected" << Qt::hex
                       << quint32(drm_formats[i]) << "got" << prime.layers[i].drm_format;
        }
#else
#define LAYER 0
#define PLANE i
#endif

        QSize planeSize = desc->rhiPlaneSize(QSize(frame->width, frame->height), i, rhi);
        constexpr uint32_t maxAttrCount = 18;
        EGLAttrib img_attr[maxAttrCount] = {
            EGL_LINUX_DRM_FOURCC_EXT,      (EGLint)quint32(drm_formats[i]),
            EGL_WIDTH,                     planeSize.width(),
            EGL_HEIGHT,                    planeSize.height(),
            EGL_DMA_BUF_PLANE0_FD_EXT,     prime.objects[prime.layers[LAYER].object_index[PLANE]].fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)prime.layers[LAYER].offset[PLANE],
            EGL_DMA_BUF_PLANE0_PITCH_EXT,  (EGLint)prime.layers[LAYER].pitch[PLANE],
        };
        uint32_t img_attr_idx = 12;
        uint64_t modifier = prime.objects[prime.layers[LAYER].object_index[PLANE]].drm_format_modifier;
        if (modifier != QtMultimediaPrivate::DmaBufFormatModifierInvalid) {
            img_attr[img_attr_idx++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
            img_attr[img_attr_idx++] = modifier & 0xFFFFFFFF;
            img_attr[img_attr_idx++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
            img_attr[img_attr_idx++] = modifier >> 32;
        }
        img_attr[img_attr_idx++] = EGL_NONE;
        Q_ASSERT(img_attr_idx <= maxAttrCount);
        images[i] = eglCreateImage(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr, img_attr);
        if (!images[i]) {
            const GLenum error = eglGetError();
            if (error == EGL_BAD_MATCH) {
                qWarning() << "eglCreateImage failed for plane" << i << "with error code EGL_BAD_MATCH, "
                              "disabling hardware acceleration. This could indicate an EGL implementation issue."
                              "\nVAAPI driver: " << vaQueryVendorString(vaDisplay)
                           << "\nEGL vendor:" << eglQueryString(eglDisplay, EGL_VENDOR);
                this->rhi = nullptr; // Disabling texture conversion here to fix QTBUG-112312
                return nullptr;
            }
            if (error) {
                qWarning() << "eglCreateImage failed for plane" << i << "with error code" << error;
                return nullptr;
            }
        }
        functions.glActiveTexture(GL_TEXTURE0 + i);
        functions.glBindTexture(GL_TEXTURE_2D, glTextures[i]);

        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC eglImageTargetTexture2D = (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)this->eglImageTargetTexture2D;
        eglImageTargetTexture2D(GL_TEXTURE_2D, images[i]);
        GLenum error = glGetError();
        if (error)
            qWarning() << "eglImageTargetTexture2D failed with error code" << error;
    }

    releaseTextures.dismiss();

    for (int i = 0;  i < nPlanes;  ++i) {
        functions.glActiveTexture(GL_TEXTURE0 + i);
        functions.glBindTexture(GL_TEXTURE_2D, 0);
        eglDestroyImage(eglDisplay, images[i]);
    }

    auto textureHandles = std::make_unique<VAAPITextureHandles>();
    textureHandles->parentConverterBackend = shared_from_this();
    textureHandles->nPlanes = nPlanes;
    textureHandles->rhi = rhi;
    textureHandles->glContext = glContext;

    for (int i = 0; i < 4; ++i)
        textureHandles->textures[i] = glTextures[i];
//        qCDebug(qLHWAccelVAAPI) << "VAAPIAccel: got textures" << textures[0] << textures[1] << textures[2] << textures[3];

    return textureHandles;
}

VAAPITextureHandles::~VAAPITextureHandles()
{
    if (rhi) {
        rhi->makeThreadLocalNativeContextCurrent();
        QOpenGLFunctions functions(glContext);
        functions.glDeleteTextures(nPlanes, textures);
    }
}

} // namespace QFFmpeg

QT_END_NAMESPACE
