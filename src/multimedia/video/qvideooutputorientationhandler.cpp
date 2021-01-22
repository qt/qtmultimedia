/****************************************************************************
**
** Copyright (C) 2016 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qvideooutputorientationhandler_p.h"

#include <QGuiApplication>
#include <QScreen>

QT_BEGIN_NAMESPACE

QVideoOutputOrientationHandler::QVideoOutputOrientationHandler(QObject *parent)
    : QObject(parent)
    , m_currentOrientation(0)
{
    QScreen *screen = QGuiApplication::primaryScreen();

    // we want to be informed about all orientation changes
    screen->setOrientationUpdateMask(Qt::PortraitOrientation|Qt::LandscapeOrientation
                                     |Qt::InvertedPortraitOrientation|Qt::InvertedLandscapeOrientation);

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
    const QScreen *screen = QGuiApplication::primaryScreen();

    const int angle = (360 - screen->angleBetween(screen->nativeOrientation(), orientation)) % 360;

    if (angle == m_currentOrientation)
        return;

    m_currentOrientation = angle;
    emit orientationChanged(m_currentOrientation);
}

QT_END_NAMESPACE
