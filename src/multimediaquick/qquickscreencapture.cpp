// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickscreencapture_p.h"

QT_BEGIN_NAMESPACE

static qreal screenFrameRateToReal(std::optional<qreal> frameRate)
{
    return frameRate.value_or(-1.f);
}

QQuickScreenCapture::QQuickScreenCapture(QObject *parent) : QScreenCapture(parent)
{
    connect(this, &QScreenCapture::screenChanged, this, [this] {
        emit qmlScreenChanged(ensureQmlScreen());
    });

    connect(this, &QScreenCapture::maximumFrameRateChanged, this,
            [this](std::optional<qreal> frameRate) {
        emit qmlMaximumFrameRateChanged(screenFrameRateToReal(frameRate));
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
    \qmlproperty real QtMultimedia::ScreenCapture::maximumFrameRate

    The screen capture frame rate upper limit.

    This can be set to override the capture frame rate used by default based on
    e.g. display refresh rate, but only as an upper limit since screen capture
    produces frames at a variable rate. Setting this higher than the display
    refresh rate is not recommended and can cause errors.

    If -1, a platform-dependent default is used.

    Any changes to this property are applied the next time the ScreenCapture
    goes active.
*/

void QQuickScreenCapture::qmlSetMaximumFrameRate(qreal frameRate)
{
    if (qFuzzyCompare(frameRate, static_cast<qreal>(-1.f)))
        setMaximumFrameRate(std::nullopt);
    else if (frameRate > 0.f)
        setMaximumFrameRate(frameRate);
}

qreal QQuickScreenCapture::qmlMaximumFrameRate() const
{
    return screenFrameRateToReal(maximumFrameRate());
}

QT_END_NAMESPACE

#include "moc_qquickscreencapture_p.cpp"
