// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaenginevideosink_p.h"

QT_BEGIN_NAMESPACE

QMediaEngineVideoSink::QMediaEngineVideoSink(QVideoSink *parent) : QPlatformVideoSink(parent) { }

QMediaEngineVideoSink::~QMediaEngineVideoSink() = default;

QT_END_NAMESPACE

#include "moc_qmediaenginevideosink_p.cpp"
