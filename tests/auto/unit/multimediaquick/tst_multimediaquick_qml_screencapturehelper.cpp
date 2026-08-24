// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_multimediaquick_qml_screencapturehelper.h"

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>

#include <QtMultimediaQuick/private/qquickscreencapture_p.h>

#include <QtQuick/private/qquickscreen_p.h>

#include <optional>

QString ScreenCaptureHelper::primaryScreenName()
{
    const QScreen *screen = QGuiApplication::primaryScreen();
    return screen ? screen->name() : QString{};
}

bool ScreenCaptureHelper::isScreenCapture(QScreenCapture *capture)
{
    return capture != nullptr;
}

bool ScreenCaptureHelper::isQuickScreenCapture(QScreenCapture *capture)
{
    return qobject_cast<QQuickScreenCapture *>(capture) != nullptr;
}

bool ScreenCaptureHelper::hasScreen(QScreenCapture *capture)
{
    return capture && capture->screen();
}

QString ScreenCaptureHelper::screenName(QScreenCapture *capture)
{
    if (!capture || !capture->screen())
        return {};

    return capture->screen()->name();
}

QObject *ScreenCaptureHelper::qmlScreen(QScreenCapture *capture)
{
    auto *quickCapture = qobject_cast<QQuickScreenCapture *>(capture);
    return quickCapture ? quickCapture->ensureQmlScreen() : nullptr;
}

bool ScreenCaptureHelper::hasMaximumFrameRate(QScreenCapture *capture)
{
    return capture && capture->maximumFrameRate().has_value();
}

qreal ScreenCaptureHelper::maximumFrameRate(QScreenCapture *capture)
{
    if (!capture)
        return -1;

    return capture->maximumFrameRate().value_or(-1);
}

bool ScreenCaptureHelper::isActive(QScreenCapture *capture)
{
    return capture && capture->isActive();
}

void ScreenCaptureHelper::setPrimaryScreen(QScreenCapture *capture)
{
    if (capture)
        capture->setScreen(QGuiApplication::primaryScreen());
}

void ScreenCaptureHelper::clearScreen(QScreenCapture *capture)
{
    if (capture)
        capture->setScreen(nullptr);
}

void ScreenCaptureHelper::setScreenFromTemporaryQmlScreen(QScreenCapture *capture, bool primary)
{
    auto *quickCapture = qobject_cast<QQuickScreenCapture *>(capture);
    if (!quickCapture)
        return;

    QQuickScreenInfo qmlScreen(nullptr, primary ? QGuiApplication::primaryScreen() : nullptr);
    quickCapture->qmlSetScreen(&qmlScreen);
}

void ScreenCaptureHelper::setMaximumFrameRate(QScreenCapture *capture, qreal frameRate)
{
    if (capture)
        capture->setMaximumFrameRate(frameRate);
}

void ScreenCaptureHelper::clearMaximumFrameRate(QScreenCapture *capture)
{
    if (capture)
        capture->setMaximumFrameRate(std::nullopt);
}

void ScreenCaptureHelper::setActive(QScreenCapture *capture, bool active)
{
    if (capture)
        capture->setActive(active);
}

void ScreenCaptureHelper::rememberQmlScreen(QScreenCapture *capture)
{
    m_rememberedQmlScreen = qmlScreen(capture);
}

bool ScreenCaptureHelper::rememberedQmlScreenIsDestroyed() const
{
    return m_rememberedQmlScreen.isNull();
}
