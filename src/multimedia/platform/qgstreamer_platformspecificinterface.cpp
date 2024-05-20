// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QtMultimedia/private/qgstreamer_platformspecificinterface_p.h>

QT_BEGIN_NAMESPACE

QGStreamerPlatformSpecificInterface::~QGStreamerPlatformSpecificInterface() = default;

QGStreamerPlatformSpecificInterface *QGStreamerPlatformSpecificInterface::instance()
{
    return dynamic_cast<QGStreamerPlatformSpecificInterface *>(
            QPlatformMediaIntegration::instance()->platformSpecificInterface());
}

QT_END_NAMESPACE
