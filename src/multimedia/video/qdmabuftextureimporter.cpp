// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdmabuftextureimporter_p.h"

#include <QtMultimedia/private/qmultimedia_ranges_p.h>
#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qpa/qplatformnativeinterface.h>
#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

static_assert(QT_CONFIG(linux_dmabuf));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLDmaBuf, "qt.multimedia.dmabuf");

namespace QtMultimediaPrivate {

QEglImageFunctions::QEglImageFunctions()
{
    m_glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
            eglGetProcAddress("glEGLImageTargetTexture2DOES"));
}

const QEglImageFunctions &QEglImageFunctions::instance()
{
    static const QEglImageFunctions singleton;
    return singleton;
}

bool QEglImageFunctions::isValid() const
{
    return m_glEGLImageTargetTexture2DOES != nullptr;
}

void QEglImageFunctions::glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) const
{
    m_glEGLImageTargetTexture2DOES(target, image);
}

DmaBufEglContext::DmaBufEglContext(QRhi *rhi)
{
    if (!rhi || rhi->backend() != QRhi::OpenGLES2) {
        qWarning() << "DmaBufEglContext: No rhi or non openGL based RHI";
        return;
    }

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    m_glContext = nativeHandles->context;
    if (!m_glContext) {
        qCDebug(qLDmaBuf) << "    no GL context, disabling";
        return;
    }
    QPlatformNativeInterface *pni = QGuiApplication::platformNativeInterface();
    m_eglDisplay = pni->nativeResourceForIntegration(QByteArrayLiteral("egldisplay"));
    qCDebug(qLDmaBuf) << "     platform is" << QGuiApplication::platformName() << m_eglDisplay;

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

DmaBufTextureHandles::DmaBufTextureHandles(QRhi &rhi, QOpenGLContext *glContext, int nPlanes,
                                           std::array<unsigned int, 4> textures,
                                           std::shared_ptr<void> parentKeepAlive)
    : m_parentKeepAlive(std::move(parentKeepAlive)),
      m_rhi(rhi),
      m_glContext(glContext),
      m_nPlanes(nPlanes),
      m_textures(textures)
{
}

DmaBufTextureHandles::~DmaBufTextureHandles()
{
    m_rhi.makeThreadLocalNativeContextCurrent();
    QOpenGLFunctions functions(m_glContext);
    functions.glDeleteTextures(m_nPlanes, m_textures.data());
}

q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importDmaBufTextures(QRhi &rhi, const DmaBufEglContext &eglContext, QSpan<const DmaBufPlane> planes,
                     QVideoFrameFormat::PixelFormat qtFormat, QSize frameSize,
                     std::shared_ptr<void> parentKeepAlive)
{
    Qt::HANDLE eglDisplay = eglContext.eglDisplay();
    QOpenGLContext *glContext = eglContext.glContext();

    QOpenGLFunctions functions(glContext);

    auto *desc = QVideoTextureHelper::textureDescription(qtFormat);
    int nPlanes = desc->nplanes;
    Q_ASSERT(planes.size() == nPlanes);

    rhi.makeThreadLocalNativeContextCurrent();

    EGLImage images[4] = {};
    std::array<GLuint, 4> glTextures{};
    functions.glGenTextures(nPlanes, glTextures.data());

    auto releaseTextures = qScopeGuard([&] {
        for (EGLImage img : images | views::filter_nonnull)
            eglDestroyImage(eglDisplay, img);

        functions.glDeleteTextures(nPlanes, glTextures.data());
    });

    for (int i = 0; i < nPlanes; ++i) {
        const DmaBufPlane &plane = planes[i];

        QSize planeSize = desc->rhiPlaneSize(frameSize, i, &rhi);
        constexpr uint32_t maxAttrCount = 18;
        EGLAttrib img_attr[maxAttrCount] = {
            EGL_LINUX_DRM_FOURCC_EXT,
            (EGLint)qToUnderlying(plane.drmFormat),
            EGL_WIDTH,
            planeSize.width(),
            EGL_HEIGHT,
            planeSize.height(),
            EGL_DMA_BUF_PLANE0_FD_EXT,
            plane.fd,
            EGL_DMA_BUF_PLANE0_OFFSET_EXT,
            (EGLint)plane.offset,
            EGL_DMA_BUF_PLANE0_PITCH_EXT,
            (EGLint)plane.pitch,
        };
        uint32_t img_attr_idx = 12;
        if (plane.modifier != DmaBufFormatModifierInvalid) {
            img_attr[img_attr_idx++] = EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT;
            img_attr[img_attr_idx++] = plane.modifier & 0xFFFFFFFF;
            img_attr[img_attr_idx++] = EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT;
            img_attr[img_attr_idx++] = plane.modifier >> 32;
        }
        img_attr[img_attr_idx++] = EGL_NONE;
        Q_ASSERT(img_attr_idx <= maxAttrCount);
        images[i] = eglCreateImage(eglDisplay, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, nullptr,
                                   img_attr);
        if (!images[i]) {
            const GLenum error = eglGetError();
            if (error == EGL_BAD_MATCH) {
                qWarning() << "eglCreateImage failed for plane" << i
                           << "with error code EGL_BAD_MATCH, disabling hardware acceleration. "
                              "This could indicate an EGL implementation issue."
                              "\nEGL vendor:"
                           << eglQueryString(eglDisplay, EGL_VENDOR);
                // Disabling texture conversion here to fix QTBUG-112312
                return q23::unexpected{ FailureSeverity::unrecoverable };
            }
            if (error) {
                qWarning() << "eglCreateImage failed for plane" << i << "with error code" << error;
                return q23::unexpected{ FailureSeverity::recoverable };
            }
        }
        functions.glActiveTexture(GL_TEXTURE0 + i);
        functions.glBindTexture(GL_TEXTURE_2D, glTextures[i]);

        QEglImageFunctions::instance().glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, images[i]);
        GLenum error = glGetError();
        if (error) {
            qWarning() << "eglImageTargetTexture2D failed for plane" << i << "with error code"
                       << error
                       << "(the driver may not support importing this dma-buf as a GL texture)";
            return q23::unexpected{ FailureSeverity::recoverable };
        }
    }

    releaseTextures.dismiss();

    for (int i = 0; i < nPlanes; ++i) {
        functions.glActiveTexture(GL_TEXTURE0 + i);
        functions.glBindTexture(GL_TEXTURE_2D, 0);
        eglDestroyImage(eglDisplay, images[i]);
    }

    return std::make_unique<DmaBufTextureHandles>(rhi, glContext, nPlanes, glTextures,
                                                  std::move(parentKeepAlive));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
