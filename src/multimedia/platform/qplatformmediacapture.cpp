// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qtmultimediaglobal_p.h>
#include "qplatformmediacapture_p.h"
#include "qaudiodevice.h"
#include "qaudioinput.h"
#include "qplatformcamera_p.h"
#include "qplatformsurfacecapture_p.h"

QT_BEGIN_NAMESPACE

QPlatformMediaCaptureSession::~QPlatformMediaCaptureSession() = default;

std::vector<QPlatformVideoSource *> QPlatformMediaCaptureSession::activeVideoSources()
{
    std::vector<QPlatformVideoSource *> result;

    auto checkSource = [&result](QPlatformVideoSource *source) {
        if (source && source->isActive())
            result.push_back(source);
    };

    checkSource(camera());
    checkSource(screenCapture());
    checkSource(windowCapture());

    return result;
}

QT_END_NAMESPACE

#include "moc_qplatformmediacapture_p.cpp"
