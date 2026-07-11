// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickscreencapture_p.h"

QT_BEGIN_NAMESPACE

QQuickScreenCapture::QQuickScreenCapture(QObject *parent) : QScreenCapture(parent)
{
    connect(this, &QScreenCapture::screenChanged, this, [this] {
        emit qmlScreenChanged(ensureQmlScreen());
    });
}

void QQuickScreenCapture::qmlSetScreen(QQuickScreenInfo *qmlScreen)
{
    setScreen(qmlScreen ? qmlScreen->wrappedScreen() : nullptr);
}

QQuickScreenInfo *QQuickScreenCapture::ensureQmlScreen()
{
    // Note QQuickApplication may recreate QQuickScreenInfo
    // so we implement creating our own instance.
    // TODO: perhaps, we should return null QQuickScreenInfo for null QScreen
    if (!m_qmlScreen || m_qmlScreen->wrappedScreen() != screen()) {
        m_ownQmlScreen = std::make_unique<QQuickScreenInfo>(this, screen());
        m_qmlScreen = m_ownQmlScreen.get();
    }

    return m_qmlScreen;
}

/*!
    \since 6.12
    \qmlproperty real QtMultimedia::ScreenCapture::frameRate

    The target screen capture framerate.

    Actual frame rate depends on the platform. For platforms with fixed rate capture, this
    frame rate is followed. For platforms with variable rate capture, this frame rate is either
    used as the polling rate (maximum frame rate) or completely ignored.

    If -1, a platform-dependent default is used.

    Any changes to this property are applied the next time the ScreenCapture goes active.
*/

void QQuickScreenCapture::qmlSetFrameRate(qreal frameRate)
{
    if (qFuzzyCompare(frameRate, static_cast<qreal>(-1.f)))
        setFrameRate(std::nullopt);
    else if (frameRate > 0.f)
        setFrameRate(frameRate);
}

qreal QQuickScreenCapture::qmlFrameRate() const
{
    return frameRate().value_or(-1.f);
}

QT_END_NAMESPACE

#include "moc_qquickscreencapture_p.cpp"
