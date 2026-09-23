// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformlibcameraplugin_p.h"

QT_BEGIN_NAMESPACE

namespace QLibCamera {

QPlatformLibCameraPlugin::QPlatformLibCameraPlugin(QObject *parent) : QObject(parent) { }

QPlatformLibCameraPlugin::~QPlatformLibCameraPlugin() = default;

} // namespace QLibCamera

QT_END_NAMESPACE

#include "moc_qplatformlibcameraplugin_p.cpp"
