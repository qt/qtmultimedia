// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickscreencapture_p.h"

QT_BEGIN_NAMESPACE

QQuickScreenCatpure::QQuickScreenCatpure(QObject *parent) : QScreenCapture(parent)
{
    connect(this, &QScreenCapture::screenChanged, this, [this](QScreen *screen) {
        emit QQuickScreenCatpure::screenChanged(new QQuickScreenInfo(this, screen));
    });
}

void QQuickScreenCatpure::qmlSetScreen(const QQuickScreenInfo *info)
{
    setScreen(info ? info->wrappedScreen() : nullptr);
}

QQuickScreenInfo *QQuickScreenCatpure::qmlScreen()
{
    return new QQuickScreenInfo(this, screen());
}

QT_END_NAMESPACE

#include "moc_qquickscreencapture_p.cpp"
