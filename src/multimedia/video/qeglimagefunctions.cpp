// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeglimagefunctions_p.h"

#include <QtMultimedia/private/qmultimedia_ranges_p.h>

#include <QtCore/qloggingcategory.h>

#include <optional>
#include <vector>

#if defined(EGL_KHR_image)
#  include <limits>
#  include <mutex>
#endif

QT_BEGIN_NAMESPACE

namespace QtMultimediaPrivate {

namespace {

#if defined(EGL_KHR_image)
Q_STATIC_LOGGING_CATEGORY(qLcEglImage, "qt.multimedia.egl");

// eglCreateImageKHR takes an EGLint attribute list, while our callers build an EGLAttrib
// (intptr_t) list, as required by core eglCreateImage. On 64-bit platforms we need to perform
// narrowing conversion; all of our current attribute values (dimensions, fds, offsets/strides,
// and the already 32-bit-split dma-buf modifier) fit into EGLint, but we still validate this
// rather than silently truncate.
[[maybe_unused]] std::optional<std::vector<EGLint>> narrowToEGLint(QSpan<const EGLAttrib> attribs)
{
    std::vector<EGLint> narrowed(size_t(attribs.size()));

    Q_ASSERT(sizeof(EGLAttrib) > sizeof(EGLint));
    static std::once_flag warnOnce;
    std::call_once(warnOnce, [] {
        qCDebug(qLcEglImage) << "EGL_KHR_image_base fallback: narrowing 64-bit EGLAttrib "
                                "attribute values to 32-bit EGLint";
    });

    for (qsizetype i = 0; i < attribs.size(); ++i) {
        const EGLAttrib value = attribs[i];
        if (value < std::numeric_limits<EGLint>::min()
            || value > std::numeric_limits<EGLint>::max()) {
            qCWarning(qLcEglImage)
                    << "EGL attribute value" << value
                    << "does not fit into EGLint; cannot use the EGL_KHR_image_base fallback";
            return std::nullopt;
        }
        narrowed[size_t(i)] = EGLint(value);
    }

    return narrowed;
}
#endif

template <typename Fn>
Fn getEglFunction(const char *name)
{
    return reinterpret_cast<Fn>(eglGetProcAddress(name));
}

} // namespace

QEglImageFunctions::QEglImageFunctions()
    :
#ifdef GL_OES_EGL_image
      m_glEGLImageTargetTexture2DOES{
          getEglFunction<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>("glEGLImageTargetTexture2DOES"),
      }
#endif
#ifdef EGL_VERSION_1_5
      ,
      m_eglCreateImage{
          getEglFunction<PFNEGLCREATEIMAGEPROC>("eglCreateImage"),
      },
      m_eglDestroyImage{
          getEglFunction<PFNEGLDESTROYIMAGEPROC>("eglDestroyImage"),
      }
#endif
#if defined(EGL_KHR_image)
      ,
      m_eglCreateImageKHR{
          getEglFunction<PFNEGLCREATEIMAGEKHRPROC>("eglCreateImageKHR"),
      },
      m_eglDestroyImageKHR{
          getEglFunction<PFNEGLDESTROYIMAGEKHRPROC>("eglDestroyImageKHR"),
      }
#endif
#if defined(EGL_EXT_image_dma_buf_import_modifiers)
      ,
      m_eglQueryDmaBufModifiersEXT{
          getEglFunction<PFNEGLQUERYDMABUFMODIFIERSEXTPROC>("eglQueryDmaBufModifiersEXT"),
      }
#endif
{
}

const QEglImageFunctions &QEglImageFunctions::instance()
{
    static const QEglImageFunctions singleton;
    return singleton;
}

bool QEglImageFunctions::isValid() const
{
#ifdef GL_OES_EGL_image
    if (!m_glEGLImageTargetTexture2DOES)
        return false;
#else
    return false;
#endif

#ifdef EGL_VERSION_1_5
    if (m_eglCreateImage && m_eglDestroyImage)
        return true;
#endif
#if defined(EGL_KHR_image)
    if (m_eglCreateImageKHR && m_eglDestroyImageKHR)
        return true;
#endif
    return false;
}

void QEglImageFunctions::glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image) const
{
#ifdef GL_OES_EGL_image
    m_glEGLImageTargetTexture2DOES(target, image);
#endif
}

EGLImageHandle QEglImageFunctions::eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                                                  EGLClientBuffer buffer,
                                                  QSpan<const EGLAttrib> attribs) const
{
    EglImageDeleter deleter(dpy);
#ifdef EGL_VERSION_1_5
    if (m_eglCreateImage) {
        return EGLImageHandle(
                m_eglCreateImage(dpy, ctx, target, buffer,
                                 attribs.empty() ? nullptr : attribs.data()),
                deleter);
    }
#endif
#if defined(EGL_KHR_image)
    if (m_eglCreateImageKHR) {
        if constexpr (sizeof(EGLAttrib) == sizeof(EGLint)) {
            return EGLImageHandle(
                    m_eglCreateImageKHR(
                            dpy, ctx, target, buffer,
                            attribs.empty() ? nullptr
                                            : reinterpret_cast<const EGLint *>(attribs.data())),
                    deleter);
        } else {
            const auto narrowed = narrowToEGLint(attribs);
            if (!narrowed)
                return EGLImageHandle(EGL_NO_IMAGE, deleter);
            return EGLImageHandle(
                    m_eglCreateImageKHR(dpy, ctx, target, buffer,
                                       narrowed->empty() ? nullptr : narrowed->data()),
                    deleter);
        }
    }
#endif
    return EGLImageHandle(EGL_NO_IMAGE, deleter);
}

EGLBoolean QEglImageFunctions::eglDestroyImage(EGLDisplay dpy, EGLImage image) const
{
#ifdef EGL_VERSION_1_5
    if (m_eglDestroyImage)
        return m_eglDestroyImage(dpy, image);
#endif
#if defined(EGL_KHR_image)
    if (m_eglDestroyImageKHR)
        return m_eglDestroyImageKHR(dpy, image);
#endif
    return EGL_FALSE;
}

std::optional<bool> QEglImageFunctions::isDmaBufModifierSupported(EGLDisplay display,
                                                                  DRMFormat drmFormat,
                                                                  DRMModifier modifier) const
{
#if defined(EGL_EXT_image_dma_buf_import_modifiers)
    if (!m_eglQueryDmaBufModifiersEXT)
        return std::nullopt; // can't introspect

    const EGLint format = EGLint(qToUnderlying(drmFormat));
    EGLint numModifiers = 0;
    if (!m_eglQueryDmaBufModifiersEXT(display, format, 0, nullptr, nullptr, &numModifiers)
        || numModifiers <= 0)
        return std::nullopt; // query unsupported

    std::vector<EGLuint64KHR> modifiers(numModifiers);
    if (!m_eglQueryDmaBufModifiersEXT(display, format, numModifiers, modifiers.data(), nullptr,
                                      &numModifiers))
        return std::nullopt;

    return ranges::find(modifiers, EGLuint64KHR(qToUnderlying(modifier))) != modifiers.end();
#else
    Q_UNUSED(display);
    Q_UNUSED(drmFormat);
    Q_UNUSED(modifier);
    return std::nullopt;
#endif
}

} // namespace QtMultimediaPrivate

QT_END_NAMESPACE
