// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qaudioinput.h>
#include <QtMultimedia/qmediacapturesession.h>
#include <QtMultimedia/private/qplatformcamera_p.h>
#include <QtMultimedia/private/qplatformmediacapture_p.h>
#include <QtMultimedia/private/qmediacapturesession_p.h>
#include <QtMultimedia/private/qplatformsurfacecapture_p.h>
#include <QtMultimedia/private/qplatformvideoframeinput_p.h>
#include <QtMultimedia/private/qtmultimediaglobal_p.h>

QT_BEGIN_NAMESPACE

QPlatformMediaCaptureSession::~QPlatformMediaCaptureSession() = default;

std::vector<QPlatformVideoSource *> QPlatformMediaCaptureSession::activeVideoSources()
{
    std::vector<QPlatformVideoSource *> result;

    auto checkSource = [&result](QPlatformVideoSource *source) {
        if (source && source->isActive())
            result.push_back(source);
    };

    checkSource(videoFrameInput());
    checkSource(camera());
    checkSource(screenCapture());
    checkSource(windowCapture());

    return result;
}

QT_END_NAMESPACE

#include "moc_qplatformmediacapture_p.cpp"
