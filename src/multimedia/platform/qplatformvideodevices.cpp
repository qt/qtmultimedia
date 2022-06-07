// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformvideodevices_p.h"
#include "qplatformmediadevices_p.h"

QT_BEGIN_NAMESPACE

QPlatformVideoDevices::~QPlatformVideoDevices()
{

}

void QPlatformVideoDevices::videoInputsChanged()
{
    QPlatformMediaDevices::instance()->videoInputsChanged();
}

QT_END_NAMESPACE
