// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickwindowcapture_p.h"

QT_BEGIN_NAMESPACE

static qreal windowFrameRateToReal(std::optional<qreal> frameRate)
{
    return frameRate.value_or(-1.f);
}

QQuickWindowCapture::QQuickWindowCapture(QObject *parent) : QWindowCapture(parent)
{
}

/*!
    \since 6.12
    \qmlproperty real QtMultimedia::WindowCapture::maximumFrameRate

    The window capture frame rate upper limit.

    This can be set to override the capture frame rate used by default based on
    e.g. display refresh rate, but only as an upper limit since window capture
    produces frames at a variable rate. Setting this higher than the display
    refresh rate is not recommended and can cause errors.

    If -1, a platform-dependent default is used.

    Any changes to this property are applied the next time the WindowCapture
    goes active.
*/

void QQuickWindowCapture::qmlSetMaximumFrameRate(qreal frameRate)
{
    if (qFuzzyCompare(frameRate, static_cast<qreal>(-1.f)))
        setMaximumFrameRate(std::nullopt);
    else if (frameRate > 0.f)
        setMaximumFrameRate(frameRate);
}

qreal QQuickWindowCapture::qmlMaximumFrameRate() const
{
    return windowFrameRateToReal(maximumFrameRate());
}

void QQuickWindowCapture::qmlResetMaximumFrameRate()
{
    setMaximumFrameRate(std::nullopt);
}

QT_END_NAMESPACE

#include "moc_qquickwindowcapture_p.cpp"
