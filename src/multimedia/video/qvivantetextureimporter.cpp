// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvivantetextureimporter_p.h"

#include <QtMultimedia/private/qvideotexturehelper_p.h>

#include <QtGui/rhi/qrhi.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>

#include <EGL/egl.h>

static_assert(QT_CONFIG(gpu_vivante));

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(qLVivante, "qt.multimedia.vivante");

namespace QtMultimediaPrivate {

QVivanteGLFunctions::QVivanteGLFunctions()
{
    m_glTexDirectVIVMap = reinterpret_cast<PFNGLTEXDIRECTVIVMAPPROC>(
            eglGetProcAddress("glTexDirectVIVMap"));
    m_glTexDirectInvalidateVIV = reinterpret_cast<PFNGLTEXDIRECTINVALIDATEVIVPROC>(
            eglGetProcAddress("glTexDirectInvalidateVIV"));
}

const QVivanteGLFunctions &QVivanteGLFunctions::instance()
{
    static const QVivanteGLFunctions singleton;
    return singleton;
}

bool QVivanteGLFunctions::isValid() const
{
    return m_glTexDirectVIVMap && m_glTexDirectInvalidateVIV;
}

void QVivanteGLFunctions::glTexDirectVIVMap(GLenum target, GLsizei width, GLsizei height,
                                            GLenum format, GLvoid **logical,
                                            const GLuint *physical) const
{
    m_glTexDirectVIVMap(target, width, height, format, logical, physical);
}

void QVivanteGLFunctions::glTexDirectInvalidateVIV(GLenum target) const
{
    m_glTexDirectInvalidateVIV(target);
}

VivanteContext::VivanteContext(QRhi *rhi)
{
    if (!rhi || rhi->backend() != QRhi::OpenGLES2) {
        qCDebug(qLVivante) << "VivanteContext: no rhi or non OpenGLES2 based RHI";
        return;
    }

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    m_glContext = nativeHandles->context;
    if (!m_glContext) {
        qCDebug(qLVivante) << "    no GL context, disabling";
        return;
    }

    if (!QVivanteGLFunctions::instance().isValid()) {
        qCDebug(qLVivante)
                << "    glTexDirectVIVMap/glTexDirectInvalidateVIV not resolved, disabling";
        return;
    }

    m_valid = true;
}

VivanteTextureHandles::VivanteTextureHandles(QRhi &rhi, QOpenGLContext *glContext, GLuint texture,
                                             std::shared_ptr<void> parentKeepAlive)
    : m_parentKeepAlive(std::move(parentKeepAlive)),
      m_rhi(rhi),
      m_glContext(glContext),
      m_texture(texture)
{
    rhi.addCleanupCallback(this, [this](QRhi *rhi) {
        release(*rhi);
    });
}

VivanteTextureHandles::~VivanteTextureHandles()
{
    if (!m_released) {
        m_rhi.removeCleanupCallback(this);
        release(m_rhi);
    }
}

void VivanteTextureHandles::release(QRhi &rhi)
{
    rhi.makeThreadLocalNativeContextCurrent();
    QOpenGLFunctions functions(m_glContext);
    functions.glDeleteTextures(1, &m_texture);
    m_released = true;
}

q23::expected<QVideoFrameTexturesHandlesUPtr, FailureSeverity>
importVivanteTexture(QRhi &rhi, const VivanteContext &context, quint32 physicalAddress,
                     void *logicalAddr, QSize size, GLenum vivFormatToken,
                     std::shared_ptr<void> parentKeepAlive)
{
    qCDebug(qLVivante) << "Importing texture from Vivante phys-mem buffer";
    QOpenGLContext *glContext = context.glContext();
    QOpenGLFunctions functions(glContext);
    const auto &vivFunctions = QVivanteGLFunctions::instance();

    rhi.makeThreadLocalNativeContextCurrent();

    GLuint texture = 0;
    functions.glGenTextures(1, &texture);

    auto releaseTexture = qScopeGuard([&] {
        functions.glDeleteTextures(1, &texture);
    });

    functions.glActiveTexture(GL_TEXTURE0);
    functions.glBindTexture(GL_TEXTURE_2D, texture);

    // Each QVideoFrame maps a fresh address, so glTexDirectVIVMap (map the
    // address into the texture) and glTexDirectInvalidateVIV (tell the
    // driver the content changed) are both called unconditionally on every
    // import.
    const GLuint physical = physicalAddress;
    vivFunctions.glTexDirectVIVMap(GL_TEXTURE_2D, size.width(), size.height(), vivFormatToken,
                                   &logicalAddr, &physical);
    if (const GLenum error = functions.glGetError(); error != GL_NO_ERROR) {
        qCWarning(qLVivante) << "glTexDirectVIVMap failed with GL error" << Qt::hex << error
                             << "for format token" << vivFormatToken << Qt::dec << "size" << size;
        functions.glBindTexture(GL_TEXTURE_2D, 0);
        return q23::unexpected{ FailureSeverity::recoverable };
    }

    vivFunctions.glTexDirectInvalidateVIV(GL_TEXTURE_2D);
    if (const GLenum error = functions.glGetError(); error != GL_NO_ERROR) {
        qCWarning(qLVivante) << "glTexDirectInvalidateVIV failed with GL error" << Qt::hex
                             << error;
        functions.glBindTexture(GL_TEXTURE_2D, 0);
        return q23::unexpected{ FailureSeverity::recoverable };
    }

    functions.glBindTexture(GL_TEXTURE_2D, 0);
    releaseTexture.dismiss();

    return std::make_unique<VivanteTextureHandles>(rhi, glContext, texture,
                                                   std::move(parentKeepAlive));
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
