// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qplatformvideosink_p.h"

QT_BEGIN_NAMESPACE

QPlatformVideoSink::QPlatformVideoSink(QVideoSink *parent)
    : QObject(parent),
    sink(parent)
{
}

QT_END_NAMESPACE

#include "moc_qplatformvideosink_p.cpp"
