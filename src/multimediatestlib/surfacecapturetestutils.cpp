// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "surfacecapturetestutils_p.h"

#include <QtMultimedia/qscreencapture.h>
#include <QtMultimedia/qwindowcapture.h>
#include <QtMultimedia/private/qscreencapture_p.h>
#include <QtMultimedia/private/qwindowcapture_p.h>

QT_BEGIN_NAMESPACE

std::unique_ptr<QScreenCapture> QtMultimediaTestLib::makeScreenCapture()
{
    auto capture = std::make_unique<QScreenCapture>();
    auto *capturePrivate = QScreenCapturePrivate::get(*capture);
    capturePrivate->setIgnoreCursor(true);
    return capture;
}

std::unique_ptr<QWindowCapture> QtMultimediaTestLib::makeWindowCapture()
{
    auto capture = std::make_unique<QWindowCapture>();
    auto *capturePrivate = QWindowCapturePrivate::get(*capture);
    capturePrivate->setIgnoreCursor(true);
    return capture;
}

QT_END_NAMESPACE
