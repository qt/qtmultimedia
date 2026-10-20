// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabuftextureimporter_p.h"

#include <QtMultimedia/private/qeglimagefunctions_p.h>
#include <QtMultimedia/private/qmultimedia_gl_support_p.h>
#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/qopenglcontext.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <array>
#include <vector>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLDmaBuf, "qt.multimedia.dmabuf")

namespace QtMultimediaPrivate {

DmaBufEglContext::DmaBufEglContext(QRhi *rhi)
{
    if (!rhi || rhi->backend() != QRhi::OpenGLES2) {
        qWarning() << "DmaBufEglContext: No rhi or non openGL based RHI";
        return;
    }

    m_glContext = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles())->context;
    if (!m_glContext) {
        qCDebug(qLDmaBuf) << "    no GL context, disabling";
        return;
    }

    auto *eglContext = m_glContext->nativeInterface<QNativeInterface::QEGLContext>();
    if (!eglContext) {
        qCDebug(qLDmaBuf) << "    no EGL context, disabling";
        return;
    }

    m_eglDisplay = eglContext->display();
    if (!m_eglDisplay) {
        qCDebug(qLDmaBuf) << "    no egl display, disabling";
        return;
    }

    if (!QEglImageFunctions::instance().isValid()) {
        qCDebug(qLDmaBuf) << "    no eglImageTargetTexture2D, disabling";
        return;
    }

    m_valid = true;
}

DmaBufTextureHandles::DmaBufTextureHandles(QRhi &rhi, std::vector<GlTextureHandle> textures,
                                           QVideoFrameFormat::PixelFormat format,
                                           std::shared_ptr<void> parentKeepAlive)
    : m_parentKeepAlive(std::move(parentKeepAlive)),
      m_rhi(rhi),
      m_textures(std::move(textures)),
      m_texturePixelFormat(format)
{
}

DmaBufTextureHandles::~DmaBufTextureHandles()
{
    // m_textures is destroyed right after this body runs, calling glDeleteTextures on each plane.
    m_rhi.makeThreadLocalNativeContextCurrent();
}

static bool dmaModifierIsNotSupportedByDisplay(const DmaBufPlane &plane, EGLDisplay eglDisplay)
{
    if (plane.modifier == DmaBufFormatModifierInvalid)
        return true; // no modifier info to check

    auto &eglFunctions = QEglImageFunctions::instance();
    constexpr bool assumeSupportedWhenUnknown = true;
    const bool formatIsSupported =
            eglFunctions.isDmaBufModifierSupported(eglDisplay, plane.drmFormat, plane.modifier)
                    .value_or(assumeSupportedWhenUnknown);
    return formatIsSupported;
}

q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importDmaBufTextures(QRhi &rhi, const DmaBufEglContext &eglContext, QSpan<const DmaBufPlane> planes,
                     QVideoFrameFormat::PixelFormat qtFormat, QSize frameSize,
                     std::shared_ptr<void> parentKeepAlive, DmaBufImportMode importMode)
{
    qCDebug(qLDmaBuf) << "Importing textures from DMA buffer," << qtFormat;
    Qt::HANDLE eglDisplay = eglContext.eglDisplay();
    QOpenGLContext *glContext = eglContext.glContext();

    QOpenGLFunctions functions(glContext);
    const auto &elgImageFunctions = QEglImageFunctions::instance();

    auto *desc = QVideoTextureHelper::textureDescription(qtFormat);
    int nPlanes = desc->nplanes;
    Q_ASSERT(planes.size() == nPlanes);

    rhi.makeThreadLocalNativeContextCurrent();

    QVideoFrameFormat::PixelFormat texturePixelFormat = qtFormat;
    GLenum glTarget = GL_TEXTURE_2D;
    if (importMode == DmaBufImportMode::SingleImage) {
        // the driver converts the planes to RGB when sampling
        texturePixelFormat = QVideoFrameFormat::Format_RGBA8888;
#ifdef GL_OES_EGL_image_external
        if (QVideoTextureHelper::forceGlTextureExternalOesIsSet()) {
            texturePixelFormat = QVideoFrameFormat::Format_SamplerExternalOES;
            glTarget = GL_TEXTURE_EXTERNAL_OES;
        }
#endif
    }

    const int nImages = importMode == DmaBufImportMode::SingleImage ? 1 : nPlanes;

    std::array<EGLImageHandle, 4> images;
    std::array<GLuint, 4> rawGlTextures{};
    functions.glGenTextures(nImages, rawGlTextures.data());

    std::vector<GlTextureHandle> glTextures;
    glTextures.reserve(nImages);
    for (int i = 0; i < nImages; ++i)
        glTextures.emplace_back(rawGlTextures[i], GlTextureDeleter(glContext));

    auto importImage =
            [&](int image, DRMFormat drmFormat, QSize imageSize,
                QSpan<const DmaBufPlane> imagePlanes) -> q23::expected<void, FailureSeverity> {
        constexpr EGLint fdAttrs[4] = {
            EGL_DMA_BUF_PLANE0_FD_EXT,
            EGL_DMA_BUF_PLANE1_FD_EXT,
            EGL_DMA_BUF_PLANE2_FD_EXT,
            EGL_DMA_BUF_PLANE3_FD_EXT,
        };
        constexpr EGLint offsetAttrs[4] = {
            EGL_DMA_BUF_PLANE0_OFFSET_EXT,
            EGL_DMA_BUF_PLANE1_OFFSET_EXT,
            EGL_DMA_BUF_PLANE2_OFFSET_EXT,
            EGL_DMA_BUF_PLANE3_OFFSET_EXT,
        };
        constexpr EGLint pitchAttrs[4] = {
            EGL_DMA_BUF_PLANE0_PITCH_EXT,
            EGL_DMA_BUF_PLANE1_PITCH_EXT,
            EGL_DMA_BUF_PLANE2_PITCH_EXT,
            EGL_DMA_BUF_PLANE3_PITCH_EXT,
        };
        constexpr EGLint modifierLoAttrs[4] = {
            EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE1_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE2_MODIFIER_LO_EXT,
            EGL_DMA_BUF_PLANE3_MODIFIER_LO_EXT,
        };
        constexpr EGLint modifierHiAttrs[4] = {
            EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
            EGL_DMA_BUF_PLANE1_MODIFIER_HI_EXT,
            EGL_DMA_BUF_PLANE2_MODIFIER_HI_EXT,
            EGL_DMA_BUF_PLANE3_MODIFIER_HI_EXT,
        };

        constexpr size_t maxAttrCount = 6 + 4 * 10 + 1;
        std::array<EGLAttrib, maxAttrCount> img_attr = {
            EGL_LINUX_DRM_FOURCC_EXT,
            (EGLint)qToUnderlying(drmFormat),
            EGL_WIDTH,
            imageSize.width(),
            EGL_HEIGHT,
            imageSize.height(),
        };
        size_t img_attr_idx = 6;
        for (int plane = 0; plane < imagePlanes.size() && plane < 4; ++plane) {
            const DmaBufPlane &dmabuf = imagePlanes[plane];
            img_attr[img_attr_idx++] = fdAttrs[plane];
            img_attr[img_attr_idx++] = dmabuf.fd;
            img_attr[img_attr_idx++] = offsetAttrs[plane];
            img_attr[img_attr_idx++] = (EGLint)dmabuf.offset;
            img_attr[img_attr_idx++] = pitchAttrs[plane];
            img_attr[img_attr_idx++] = (EGLint)dmabuf.pitch;
            if (dmabuf.modifier != DmaBufFormatModifierInvalid) {
                const auto modifier = qToUnderlying(dmabuf.modifier);
                img_attr[img_attr_idx++] = modifierLoAttrs[plane];
                img_attr[img_attr_idx++] = EGLAttrib(modifier & 0xFFFFFFFF);
                img_attr[img_attr_idx++] = modifierHiAttrs[plane];
                img_attr[img_attr_idx++] = EGLAttrib(modifier >> 32);
            }
        }
        img_attr[img_attr_idx++] = EGL_NONE;
        Q_ASSERT(img_attr_idx <= maxAttrCount);

        images[image] = elgImageFunctions.eglCreateImage(
                eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr,
                QSpan<const EGLAttrib>(img_attr.data(), img_attr_idx));
        if (!images[image]) {
            const EGLError error = EGLError(eglGetError());
            switch (error) {
            case EGLError::Success: {
                break;
            }
            case EGLError::BadMatch: {
                qWarning() << "eglCreateImage failed for image" << image << "with" << error
                           << ", disabling hardware acceleration. "
                              "This could indicate an EGL implementation issue."
                              "\nEGL vendor:"
                           << eglQueryString(eglDisplay, EGL_VENDOR);
                // Disabling texture conversion here to fix QTBUG-112312
                return q23::unexpected{ FailureSeverity::unrecoverable };
            }
            case EGLError::BadParameter: {
                bool dmaModifierNotSupported =
                        ranges::any_of(imagePlanes, [&](const DmaBufPlane &plane) {
                    return dmaModifierIsNotSupportedByDisplay(plane, eglDisplay);
                });
                if (dmaModifierNotSupported) {
                    qWarning() << "eglCreateImage failed for image" << image << "with" << error
                               << ", disabling hardware acceleration. "
                                  "This could indicate an EGL implementation issue."
                                  "\nEGL vendor:"
                               << eglQueryString(eglDisplay, EGL_VENDOR);
                    // Disabling texture conversion here to fix QTBUG-112312
                    return q23::unexpected{ FailureSeverity::unrecoverable };
                }
                [[fallthrough]];
            }
            default: {
                qWarning() << "eglCreateImage failed for image" << image << "with" << error;
                return q23::unexpected{ FailureSeverity::recoverable };
            }
            }
        }
        functions.glActiveTexture(GL_TEXTURE0 + image);
        functions.glBindTexture(glTarget, glTextures[image].get());

        elgImageFunctions.glEGLImageTargetTexture2DOES(glTarget, images[image].get());
        const GLError error = GLError(glGetError());
        if (error != GLError::NoError) {
            qWarning() << "eglImageTargetTexture2D failed for image" << image << "with" << error
                       << "(the driver may not support importing this dma-buf as a GL texture)";
            return q23::unexpected{ FailureSeverity::recoverable };
        }
        return {};
    };

    switch (importMode) {
    case DmaBufImportMode::SingleImage: {
        const auto combinedFormat = dmaBufSingleImageFourccFromPixelFormat(qtFormat);
        if (!combinedFormat) {
            qCWarning(qLDmaBuf) << "No single-image DRM format for" << qtFormat;
            return q23::unexpected(FailureSeverity::recoverable);
        }
        auto result = importImage(0, *combinedFormat, frameSize, planes.first(nPlanes));
        if (!result)
            return q23::unexpected(result.error());
        break;
    }
    case DmaBufImportMode::PerPlane:
        for (int i = 0; i < nPlanes; ++i) {
            const QSize planeSize = desc->rhiPlaneSize(frameSize, i, &rhi);
            auto result = importImage(i, planes[i].drmFormat, planeSize, planes.subspan(i, 1));
            if (!result)
                return q23::unexpected(result.error());
        }
        break;
    default:
        Q_UNREACHABLE();
    }

    for (int i = 0; i < nImages; ++i) {
        functions.glActiveTexture(GL_TEXTURE0 + i);
        functions.glBindTexture(glTarget, 0);
    }

    return std::make_unique<DmaBufTextureHandles>(rhi, std::move(glTextures), texturePixelFormat,
                                                  std::move(parentKeepAlive));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
