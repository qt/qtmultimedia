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

//#define VA_EXPORT_USE_LAYERS

#if __has_include("drm/drm_fourcc.h")
#include <drm/drm_fourcc.h>
#elif __has_include("libdrm/drm_fourcc.h")
#include <libdrm/drm_fourcc.h>
#else
// keep things building without drm_fourcc.h
#define fourcc_code(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                                 ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define DRM_FORMAT_RGBA8888     fourcc_code('R', 'A', '2', '4') /* [31:0] R:G:B:A 8:8:8:8 little endian */
#define DRM_FORMAT_RGB888       fourcc_code('R', 'G', '2', '4') /* [23:0] R:G:B little endian */
#define DRM_FORMAT_RG88         fourcc_code('R', 'G', '8', '8') /* [15:0] R:G 8:8 little endian */
#define DRM_FORMAT_ABGR8888     fourcc_code('A', 'B', '2', '4') /* [31:0] A:B:G:R 8:8:8:8 little endian */
#define DRM_FORMAT_BGR888       fourcc_code('B', 'G', '2', '4') /* [23:0] B:G:R little endian */
#define DRM_FORMAT_GR88         fourcc_code('G', 'R', '8', '8') /* [15:0] G:R 8:8 little endian */
#define DRM_FORMAT_R8           fourcc_code('R', '8', ' ', ' ') /* [7:0] R */
#define DRM_FORMAT_R16          fourcc_code('R', '1', '6', ' ') /* [15:0] R little endian */
#define DRM_FORMAT_RGB565       fourcc_code('R', 'G', '1', '6') /* [15:0] R:G:B 5:6:5 little endian */
#define DRM_FORMAT_RG1616       fourcc_code('R', 'G', '3', '2') /* [31:0] R:G 16:16 little endian */
#define DRM_FORMAT_GR1616       fourcc_code('G', 'R', '3', '2') /* [31:0] G:R 16:16 little endian */
#define DRM_FORMAT_BGRA1010102  fourcc_code('B', 'A', '3', '0') /* [31:0] B:G:R:A 10:10:10:2 little endian */
#endif

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

static const quint32 *fourccFromPixelFormat(const QVideoFrameFormat::PixelFormat format)
{
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    const quint32 rgba_fourcc = DRM_FORMAT_ABGR8888;
    const quint32 rg_fourcc = DRM_FORMAT_GR88;
    const quint32 rg16_fourcc = DRM_FORMAT_GR1616;
#else
    const quint32 rgba_fourcc = DRM_FORMAT_RGBA8888;
    const quint32 rg_fourcc = DRM_FORMAT_RG88;
    const quint32 rg16_fourcc = DRM_FORMAT_RG1616;
#endif

//    qCDebug(qLHWAccelVAAPI) << "Getting DRM fourcc for pixel format" << format;

    switch (format) {
    case QVideoFrameFormat::Format_Invalid:
    case QVideoFrameFormat::Format_IMC1:
    case QVideoFrameFormat::Format_IMC2:
    case QVideoFrameFormat::Format_IMC3:
    case QVideoFrameFormat::Format_IMC4:
    case QVideoFrameFormat::Format_SamplerExternalOES:
    case QVideoFrameFormat::Format_Jpeg:
    case QVideoFrameFormat::Format_SamplerRect:
        return nullptr;

    case QVideoFrameFormat::Format_ARGB8888:
    case QVideoFrameFormat::Format_ARGB8888_Premultiplied:
    case QVideoFrameFormat::Format_XRGB8888:
    case QVideoFrameFormat::Format_BGRA8888:
    case QVideoFrameFormat::Format_BGRA8888_Premultiplied:
    case QVideoFrameFormat::Format_BGRX8888:
    case QVideoFrameFormat::Format_ABGR8888:
    case QVideoFrameFormat::Format_XBGR8888:
    case QVideoFrameFormat::Format_RGBA8888:
    case QVideoFrameFormat::Format_RGBX8888:
    case QVideoFrameFormat::Format_AYUV:
    case QVideoFrameFormat::Format_AYUV_Premultiplied:
    case QVideoFrameFormat::Format_UYVY:
    case QVideoFrameFormat::Format_YUYV:
    {
        static constexpr quint32 format[] = { rgba_fourcc, 0, 0, 0 };
        return format;
    }

    case QVideoFrameFormat::Format_Y8:
    {
        static constexpr quint32 format[] = { DRM_FORMAT_R8, 0, 0, 0 };
        return format;
    }
    case QVideoFrameFormat::Format_Y16:
    {
        static constexpr quint32 format[] = { DRM_FORMAT_R16, 0, 0, 0 };
        return format;
    }

    case QVideoFrameFormat::Format_YUV420P:
    case QVideoFrameFormat::Format_YUV422P:
    case QVideoFrameFormat::Format_YV12:
    {
        static constexpr quint32 format[] = { DRM_FORMAT_R8, DRM_FORMAT_R8, DRM_FORMAT_R8, 0 };
        return format;
    }
    case QVideoFrameFormat::Format_YUV420P10:
    {
        static constexpr quint32 format[] = { DRM_FORMAT_R16, DRM_FORMAT_R16, DRM_FORMAT_R16, 0 };
        return format;
    }

    case QVideoFrameFormat::Format_NV12:
    case QVideoFrameFormat::Format_NV21:
    {
        static constexpr quint32 format[] = { DRM_FORMAT_R8, rg_fourcc, 0, 0 };
        return format;
    }

    case QVideoFrameFormat::Format_P010:
    case QVideoFrameFormat::Format_P016:
    {
        static constexpr quint32 format[] = { DRM_FORMAT_R16, rg16_fourcc, 0, 0 };
        return format;
    }
    }
    return nullptr;
}

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
    if (!eglDisplay) {
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
    auto *drm_formats = fourccFromPixelFormat(qtFormat);
    if (!drm_formats || needsConversion) {
        qWarning() << "can't use DMA transfer for pixel format" << fmt << qtFormat;
        return nullptr;
    }

    auto *desc = QVideoTextureHelper::textureDescription(qtFormat);
    int nPlanes = 0;
    for (; nPlanes < 5; ++nPlanes) {
        if (drm_formats[nPlanes] == 0)
            break;
    }
    Q_ASSERT(nPlanes == desc->nplanes);
    nPlanes = desc->nplanes;
//        qCDebug(qLHWAccelVAAPI) << "VAAPIAccel: nPlanes" << nPlanes;

    rhi->makeThreadLocalNativeContextCurrent();

    EGLImage images[4];
    GLuint glTextures[4] = {};
    functions.glGenTextures(nPlanes, glTextures);
    for (int i = 0;  i < nPlanes;  ++i) {
#ifdef VA_EXPORT_USE_LAYERS
#define LAYER i
#define PLANE 0
        if (prime.layers[i].drm_format != drm_formats[i]) {
            qWarning() << "expected DRM format check failed expected"
                       << Qt::hex << drm_formats[i] << "got" << prime.layers[i].drm_format;
        }
#else
#define LAYER 0
#define PLANE i
#endif

        QSize planeSize = desc->rhiPlaneSize(QSize(frame->width, frame->height), i, rhi);
        constexpr uint32_t maxAttrCount = 18;
        EGLAttrib img_attr[maxAttrCount] = {
            EGL_LINUX_DRM_FOURCC_EXT,      (EGLint)drm_formats[i],
            EGL_WIDTH,                     planeSize.width(),
            EGL_HEIGHT,                    planeSize.height(),
            EGL_DMA_BUF_PLANE0_FD_EXT,     prime.objects[prime.layers[LAYER].object_index[PLANE]].fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT, (EGLint)prime.layers[LAYER].offset[PLANE],
            EGL_DMA_BUF_PLANE0_PITCH_EXT,  (EGLint)prime.layers[LAYER].pitch[PLANE],
        };
        uint32_t img_attr_idx = 12;
        uint64_t modifier = prime.objects[prime.layers[LAYER].object_index[PLANE]].drm_format_modifier;
        if (modifier != DRM_FORMAT_MOD_INVALID) {
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
