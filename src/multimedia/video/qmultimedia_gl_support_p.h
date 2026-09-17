// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMULTIMEDIA_GL_SUPPORT_P_H
#define QMULTIMEDIA_GL_SUPPORT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtGui/qopengl.h>
#include <QtCore/private/quniquehandle_p.h>
#include <QtCore/qstring.h>

// thanks X.h
#pragma push_macro("Success")
#ifdef Success
#  undef Success
#endif

#pragma push_macro("BadAccess")
#ifdef BadAccess
#  undef BadAccess
#endif

#pragma push_macro("BadAlloc")
#ifdef BadAlloc
#  undef BadAlloc
#endif

#pragma push_macro("BadMatch")
#ifdef BadMatch
#  undef BadMatch
#endif

#if QT_CONFIG(egl)
typedef void *EGLDisplay;
typedef void *EGLImage;
#endif

QT_BEGIN_NAMESPACE

class QOpenGLContext;
class QDebug;
class QRhi;

namespace QtMultimediaPrivate {

// Error codes returned by glGetError(), see
// https://registry.khronos.org/OpenGL-Refpages/gl4/html/glGetError.xhtml
enum class GLError : unsigned int {
    NoError = 0,
    InvalidEnum = 0x0500,
    InvalidValue = 0x0501,
    InvalidOperation = 0x0502,
    StackOverflow = 0x0503,
    StackUnderflow = 0x0504,
    OutOfMemory = 0x0505,
    InvalidFramebufferOperation = 0x0506,
};

Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug debug, GLError error);

#if QT_CONFIG(egl)

// Error codes returned by eglGetError(), see
// https://registry.khronos.org/EGL/sdk/docs/man/html/eglGetError.xhtml
enum class EGLError : int {
    Success = 0x3000,
    NotInitialized = 0x3001,
    BadAccess = 0x3002,
    BadAlloc = 0x3003,
    BadAttribute = 0x3004,
    BadConfig = 0x3005,
    BadContext = 0x3006,
    BadCurrentSurface = 0x3007,
    BadDisplay = 0x3008,
    BadMatch = 0x3009,
    BadNativePixmap = 0x300A,
    BadNativeWindow = 0x300B,
    BadParameter = 0x300C,
    BadSurface = 0x300D,
    ContextLost = 0x300E,
};

Q_MULTIMEDIA_EXPORT QDebug operator<<(QDebug debug, EGLError error);

Q_MULTIMEDIA_EXPORT EGLDisplay resolveEglDisplay(QRhi &);

///////////////////////////////////////////////////////////////////////////////
// (e)gl handle types

class Q_MULTIMEDIA_EXPORT EglImageDeleter
{
public:
    EglImageDeleter() = default;
    explicit EglImageDeleter(EGLDisplay display) : m_display(display) { }
    void operator()(EGLImage image) const noexcept;

private:
    EGLDisplay m_display = {}; // EGL_NO_DISPLAY;
};

struct EGLImageHandleTraits
{
    using Type = EGLImage;
    static Type invalidValue() { return nullptr; } // EGL_NO_IMAGE;
};
using EGLImageHandle = QUniqueHandle<EGLImageHandleTraits, EglImageDeleter>;

#endif // QT_CONFIG(egl)

class Q_MULTIMEDIA_EXPORT GlTextureDeleter
{
public:
    GlTextureDeleter() = default;
    explicit GlTextureDeleter(QOpenGLContext *context) : m_context(context) { }
    void operator()(GLuint texture) const noexcept;

private:
    QOpenGLContext *m_context = nullptr;
};

struct GlTextureHandleTraits
{
    using Type = GLuint;
    static Type invalidValue() { return 0; }
};
using GlTextureHandle = QUniqueHandle<GlTextureHandleTraits, GlTextureDeleter>;

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#pragma pop_macro("Success")
#pragma pop_macro("BadAccess")
#pragma pop_macro("BadAlloc")
#pragma pop_macro("BadMatch")

#endif
