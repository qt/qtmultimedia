// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_multimediaquick_qml_testhelper.h"

#include <QtMultimedia/private/qplatformmediaintegration_p.h>

QT_USE_NAMESPACE

QString TestHelper::mediaBackendName()
{
    return QPlatformMediaIntegration::instance()->name();
}
