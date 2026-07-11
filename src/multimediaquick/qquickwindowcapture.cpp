// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindowcapture_p.h"

QT_BEGIN_NAMESPACE

QQuickWindowCapture::QQuickWindowCapture(QObject *parent) : QWindowCapture(parent)
{}

/*!
    \since 6.12
    \qmlproperty real QtMultimedia::WindowCapture::frameRate

    The target window capture framerate.

    Actual frame rate depends on the platform. For platforms with fixed rate capture, this
    frame rate is followed. For platforms with variable rate capture, this frame rate is either
    used as the polling rate (maximum frame rate) or completely ignored.

    If -1, a platform-dependent default is used.

    Any changes to this property are applied the next time the WindowCapture goes active.
*/

void QQuickWindowCapture::qmlSetFrameRate(qreal frameRate)
{
    if (qFuzzyCompare(frameRate, static_cast<qreal>(-1.f)))
        setFrameRate(std::nullopt);
    else if (frameRate > 0.f)
        setFrameRate(frameRate);
}

qreal QQuickWindowCapture::qmlFrameRate() const
{
    return frameRate().value_or(-1.f);
}

QT_END_NAMESPACE

#include "moc_qquickwindowcapture_p.cpp"
