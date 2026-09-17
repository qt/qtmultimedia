// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qgstreameregldisplay_p.h"

#if QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)

#  if QT_CONFIG(linux_dmabuf)
#    include <QtMultimedia/private/qdmabuftextureimporter_p.h>
#    include <QtMultimedia/private/qeglimagefunctions_p.h>
#  endif

#  include <QtMultimedia/private/qmultimedia_gl_support_p.h>

QT_BEGIN_NAMESPACE

#  if QT_CONFIG(linux_dmabuf)
bool qGstEglCanMapDmaBuf(QRhi *rhi)
{
    using namespace QtMultimediaPrivate;

    return rhi && resolveEglDisplay(*rhi) && QEglImageFunctions::instance().isValid();
}
#  endif

QT_END_NAMESPACE

#endif // QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)
