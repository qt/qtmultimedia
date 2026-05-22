// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef GSTREAMER_PLATFORMSPECIFICINTERFACE_P_H
#define GSTREAMER_PLATFORMSPECIFICINTERFACE_P_H

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

#if defined(__clang__) || defined(__GNUC__) || __cplusplus >= 202302L
#  warning "Use QGStreamerInterface from <QtMultimedia/spi/qgstreamerinterface.h>"
#elif defined(_MSC_VER)
#  pragma message("Warning: Use QGStreamerInterface from <QtMultimedia/spi/qgstreamerinterface.h>")
#endif

#include <QtMultimedia/spi/qgstreamerinterface.h>

QT_BEGIN_NAMESPACE

using QGStreamerPlatformSpecificInterface = QGStreamerInterface;

QT_END_NAMESPACE

#endif // GSTREAMER_PLATFORMSPECIFICINTERFACE_P_H
