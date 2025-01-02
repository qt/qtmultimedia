// Copyright (C) 2016 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qvideooutputorientationhandler_p.h"

#include <QGuiApplication>
#include <QScreen>

QT_BEGIN_NAMESPACE

bool QVideoOutputOrientationHandler::m_isRecording = false;

QVideoOutputOrientationHandler::QVideoOutputOrientationHandler(QObject *parent)
    : QObject(parent)
    , m_currentOrientation(0)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    connect(screen, SIGNAL(orientationChanged(Qt::ScreenOrientation)),
            this, SLOT(screenOrientationChanged(Qt::ScreenOrientation)));

    screenOrientationChanged(screen->orientation());
}

int QVideoOutputOrientationHandler::currentOrientation() const
{
    return m_currentOrientation;
}

void QVideoOutputOrientationHandler::screenOrientationChanged(Qt::ScreenOrientation orientation)
{
    if (m_isRecording)
        return;

    const QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    const int angle = (360 - screen->angleBetween(screen->nativeOrientation(), orientation)) % 360;

    if (angle == m_currentOrientation)
        return;

    m_currentOrientation = angle;
    emit orientationChanged(m_currentOrientation);
}

QT_END_NAMESPACE

#include "moc_qvideooutputorientationhandler_p.cpp"
