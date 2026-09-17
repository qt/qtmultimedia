// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmultimedia_gl_support_p.h"

#include <QtGui/qopenglcontext.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/rhi/qrhi.h>

#include <QtCore/qdebug.h>

#if QT_CONFIG(egl)
#  include <QtMultimedia/private/qeglimagefunctions_p.h>
#  include <EGL/egl.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtMultimediaPrivate {

static QString toString(GLError error)
{
    switch (error) {
    case GLError::NoError:
        return u"GL_NO_ERROR"_s;
    case GLError::InvalidEnum:
        return u"GL_INVALID_ENUM"_s;
    case GLError::InvalidValue:
        return u"GL_INVALID_VALUE"_s;
    case GLError::InvalidOperation:
        return u"GL_INVALID_OPERATION"_s;
    case GLError::StackOverflow:
        return u"GL_STACK_OVERFLOW"_s;
    case GLError::StackUnderflow:
        return u"GL_STACK_UNDERFLOW"_s;
    case GLError::OutOfMemory:
        return u"GL_OUT_OF_MEMORY"_s;
    case GLError::InvalidFramebufferOperation:
        return u"GL_INVALID_FRAMEBUFFER_OPERATION"_s;
    }
    return u"unknown GL error"_s;
}

QDebug operator<<(QDebug debug, GLError error)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "GLError(0x" << Qt::hex << qToUnderlying(error) << Qt::dec << ", "
                              << toString(error) << ')';
    return debug;
}

#if QT_CONFIG(egl)

static QString toString(EGLError error)
{
    switch (error) {
    case EGLError::Success:
        return u"EGL_SUCCESS"_s;
    case EGLError::NotInitialized:
        return u"EGL_NOT_INITIALIZED"_s;
    case EGLError::BadAccess:
        return u"EGL_BAD_ACCESS"_s;
    case EGLError::BadAlloc:
        return u"EGL_BAD_ALLOC"_s;
    case EGLError::BadAttribute:
        return u"EGL_BAD_ATTRIBUTE"_s;
    case EGLError::BadConfig:
        return u"EGL_BAD_CONFIG"_s;
    case EGLError::BadContext:
        return u"EGL_BAD_CONTEXT"_s;
    case EGLError::BadCurrentSurface:
        return u"EGL_BAD_CURRENT_SURFACE"_s;
    case EGLError::BadDisplay:
        return u"EGL_BAD_DISPLAY"_s;
    case EGLError::BadMatch:
        return u"EGL_BAD_MATCH"_s;
    case EGLError::BadNativePixmap:
        return u"EGL_BAD_NATIVE_PIXMAP"_s;
    case EGLError::BadNativeWindow:
        return u"EGL_BAD_NATIVE_WINDOW"_s;
    case EGLError::BadParameter:
        return u"EGL_BAD_PARAMETER"_s;
    case EGLError::BadSurface:
        return u"EGL_BAD_SURFACE"_s;
    case EGLError::ContextLost:
        return u"EGL_CONTEXT_LOST"_s;
    }
    return u"unknown EGL error"_s;
}

QDebug operator<<(QDebug debug, EGLError error)
{
    const QDebugStateSaver saver(debug);
    debug.nospace().noquote() << "EGLError(0x" << Qt::hex << qToUnderlying(error) << Qt::dec << ", "
                              << toString(error) << ')';
    return debug;
}

EGLDisplay resolveEglDisplay(QRhi &rhi)
{
    if (rhi.backend() != QRhi::OpenGLES2)
        return EGL_NO_DISPLAY;

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi.nativeHandles());
    QOpenGLContext *glContext = nativeHandles ? nativeHandles->context : nullptr;
    if (!glContext)
        return EGL_NO_DISPLAY;

    auto *eglContext = glContext->nativeInterface<QNativeInterface::QEGLContext>();
    return eglContext ? eglContext->display() : EGL_NO_DISPLAY;
}

void EglImageDeleter::operator()(EGLImage image) const noexcept
{
    if (image != EGL_NO_IMAGE)
        QEglImageFunctions::instance().eglDestroyImage(m_display, image);
}
#endif

void GlTextureDeleter::operator()(GLuint texture) const noexcept
{
    if (texture && m_context) {
        QOpenGLFunctions functions(m_context);
        functions.glDeleteTextures(1, &texture);
    }
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
