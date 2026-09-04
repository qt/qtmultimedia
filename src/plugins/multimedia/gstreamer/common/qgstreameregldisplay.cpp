// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameregldisplay_p.h"

#if QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)

#  include <QtGui/qopenglcontext.h>
#  include <QtGui/rhi/qrhi.h>

#  if QT_CONFIG(linux_dmabuf)
#    include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#  endif

QT_BEGIN_NAMESPACE

EGLDisplay qGstEglDisplay(QRhi *rhi)
{
    if (!rhi || rhi->backend() != QRhi::OpenGLES2)
        return nullptr;

    auto *nativeHandles = static_cast<const QRhiGles2NativeHandles *>(rhi->nativeHandles());
    QOpenGLContext *glContext = nativeHandles ? nativeHandles->context : nullptr;
    auto *eglContext =
            glContext ? glContext->nativeInterface<QNativeInterface::QEGLContext>() : nullptr;
    return eglContext ? eglContext->display() : nullptr;
}

#  if QT_CONFIG(linux_dmabuf)
bool qGstEglCanMapDmaBuf(QRhi *rhi)
{
    using namespace QtMultimediaPrivate;

    return qGstEglDisplay(rhi) && QEglImageFunctions::instance().isValid();
}
#  endif

QT_END_NAMESPACE

#endif // QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)
