// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QEGLIMAGEFUNCTIONS_P_H
#define QEGLIMAGEFUNCTIONS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtMultimedia/private/qmultimedia_drm_support_p.h>
#include <QtMultimedia/private/qmultimedia_gl_support_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>
#include <QtGui/qopengl.h>
#include <QtCore/qspan.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <optional>

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

class Q_MULTIMEDIA_EXPORT QEglImageFunctions
{
    QEglImageFunctions();

public:
    static const QEglImageFunctions &instance();

    bool isValid() const;

    void glEGLImageTargetTexture2DOES(GLenum, GLeglImageOES) const;

    EGLImageHandle eglCreateImage(EGLDisplay, EGLContext, EGLenum, EGLClientBuffer,
                                  QSpan<const EGLAttrib> attribs = {}) const;
    EGLBoolean eglDestroyImage(EGLDisplay, EGLImage) const;

    // Returns whether `modifier` is a supported dma-buf import modifier for `drmFormat` on
    // `display`, or nullopt if this EGL implementation doesn't support querying modifiers at
    // all (EGL_EXT_image_dma_buf_import_modifiers unavailable).
    std::optional<bool> isDmaBufModifierSupported(EGLDisplay display, DRMFormat drmFormat,
                                                  DRMModifier modifier) const;

private:
#ifdef GL_OES_EGL_image
    const PFNGLEGLIMAGETARGETTEXTURE2DOESPROC m_glEGLImageTargetTexture2DOES = nullptr;
#endif

#ifdef EGL_VERSION_1_5
    // egl.h declares eglCreateImage/eglDestroyImage, but the egl library may not provide the
    // symbols, so we need to resolve them at run-time.

    // NB: android does not declare these function pointer types
    typedef EGLImage(EGLAPIENTRYP PFNEGLCREATEIMAGEPROC)(EGLDisplay, EGLContext, EGLenum,
                                                         EGLClientBuffer, const EGLAttrib *);
    typedef EGLBoolean(EGLAPIENTRYP PFNEGLDESTROYIMAGEPROC)(EGLDisplay, EGLImage);

    const PFNEGLCREATEIMAGEPROC m_eglCreateImage = nullptr;
    const PFNEGLDESTROYIMAGEPROC m_eglDestroyImage = nullptr;
#endif
#if defined(EGL_KHR_image)
    const PFNEGLCREATEIMAGEKHRPROC m_eglCreateImageKHR = nullptr;
    const PFNEGLDESTROYIMAGEKHRPROC m_eglDestroyImageKHR = nullptr;
#endif
#if defined(EGL_EXT_image_dma_buf_import_modifiers)
    const PFNEGLQUERYDMABUFMODIFIERSEXTPROC m_eglQueryDmaBufModifiersEXT = nullptr;
#endif
};

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE

#endif // QEGLIMAGEFUNCTIONS_P_H
