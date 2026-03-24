// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickscreencapture_p.h"

QT_BEGIN_NAMESPACE

QQuickScreenCatpure::QQuickScreenCatpure(QObject *parent) : QScreenCapture(parent)
{
    connect(this, &QScreenCapture::screenChanged, this, [this] {
        emit qmlScreenChanged(ensureQmlScreen());
    });
}

void QQuickScreenCatpure::qmlSetScreen(QQuickScreenInfo *qmlScreen)
{
    setScreen(qmlScreen ? qmlScreen->wrappedScreen() : nullptr);
}

QQuickScreenInfo *QQuickScreenCatpure::ensureQmlScreen()
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

QT_END_NAMESPACE

#include "moc_qquickscreencapture_p.cpp"
