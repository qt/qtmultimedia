// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QGSTREAMEREGLDISPLAY_P_H
#define QGSTREAMEREGLDISPLAY_P_H

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

#include <QtCore/qnamespace.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(gstreamer_gl) && QT_CONFIG(gstreamer_gl_egl)
Qt::HANDLE qGstEglDisplay();

#  if QT_CONFIG(linux_dmabuf)
QFunctionPointer qGstEglImageTargetTexture2D();
bool qGstEglCanMapDmaBuf();
#  endif
#endif

QT_END_NAMESPACE

#endif
