// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qplatformmediaplugin_p.h"

QT_BEGIN_NAMESPACE

QPlatformMediaPlugin::QPlatformMediaPlugin(QObject *parent) : QObject(parent) { }

QPlatformMediaPlugin::~QPlatformMediaPlugin() = default;

QT_END_NAMESPACE

#include "moc_qplatformmediaplugin_p.cpp"
