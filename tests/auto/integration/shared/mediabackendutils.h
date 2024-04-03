// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIABACKENDUTILS_H
#define MEDIABACKENDUTILS_H

#include <QtTest/qtestcase.h>
#include <private/qplatformmediaintegration_p.h>

#define QSKIP_GSTREAMER(message)                                          \
    do {                                                                  \
        if (QPlatformMediaIntegration::instance()->name() == "gstreamer") \
            QSKIP(message);                                               \
    } while (0)

#define QEXPECT_FAIL_GSTREAMER(dataIndex, comment, mode)                  \
    do {                                                                  \
        if (QPlatformMediaIntegration::instance()->name() == "gstreamer") \
            QEXPECT_FAIL(dataIndex, comment, mode);                       \
    } while (0)

#endif // MEDIABACKENDUTILS_H
