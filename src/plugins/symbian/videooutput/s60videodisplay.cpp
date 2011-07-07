/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "s60videodisplay.h"
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <coecntrl.h>
#include <w32std.h>

S60VideoDisplay::S60VideoDisplay(QObject *parent)
:   QObject(parent)
,   m_fullScreen(false)
,   m_visible(true)
,   m_aspectRatioMode(Qt::KeepAspectRatio)
,   m_paintingEnabled(false)
,   m_rotation(0.0f)
{
    connect(this, SIGNAL(displayRectChanged(QRect, QRect)),
            this, SLOT(updateContentRect()));
    connect(this, SIGNAL(nativeSizeChanged(QSize)),
            this, SLOT(updateContentRect()));
}

S60VideoDisplay::~S60VideoDisplay()
{

}

RWindow *S60VideoDisplay::windowHandle() const
{
    return winId() ? static_cast<RWindow *>(winId()->DrawableWindow()) : 0;
}

QRect S60VideoDisplay::clipRect() const
{
    QRect displayableRect;
#ifdef VIDEOOUTPUT_GRAPHICS_SURFACES
    if (RWindow *window = windowHandle())
        displayableRect = QRect(0, 0, window->Size().iWidth, window->Size().iHeight);
#else
    displayableRect = QApplication::desktop()->screenGeometry();
#endif
    return extentRect().intersected(displayableRect);
}

QRect S60VideoDisplay::contentRect() const
{
    return m_contentRect;
}

void S60VideoDisplay::setFullScreen(bool enabled)
{
    if (m_fullScreen != enabled) {
        m_fullScreen = enabled;
        emit fullScreenChanged(m_fullScreen);
    }
}

bool S60VideoDisplay::isFullScreen() const
{
    return m_fullScreen;
}

void S60VideoDisplay::setVisible(bool enabled)
{
    if (m_visible != enabled) {
        m_visible = enabled;
        emit visibilityChanged(m_visible);
    }
}

bool S60VideoDisplay::isVisible() const
{
    return m_visible;
}

void S60VideoDisplay::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    if (m_aspectRatioMode != mode) {
        m_aspectRatioMode = mode;
        emit aspectRatioModeChanged(m_aspectRatioMode);
    }
}

Qt::AspectRatioMode S60VideoDisplay::aspectRatioMode() const
{
    return m_aspectRatioMode;
}

void S60VideoDisplay::setNativeSize(const QSize &size)
{
    if (m_nativeSize != size) {
        m_nativeSize = size;
        emit nativeSizeChanged(m_nativeSize);
    }
}

const QSize& S60VideoDisplay::nativeSize() const
{
    return m_nativeSize;
}

void S60VideoDisplay::setPaintingEnabled(bool enabled)
{
    if (m_paintingEnabled != enabled) {
        m_paintingEnabled = enabled;
        emit paintingEnabledChanged(m_paintingEnabled);
    }
}

bool S60VideoDisplay::isPaintingEnabled() const
{
    return m_paintingEnabled;
}

void S60VideoDisplay::setRotation(qreal value)
{
    if (value != m_rotation) {
        m_rotation = value;
        emit rotationChanged(m_rotation);
    }
}

qreal S60VideoDisplay::rotation() const
{
    return m_rotation;
}

void S60VideoDisplay::updateContentRect()
{
    if (isPaintingEnabled()) {
        const int dx = qMax(0, extentRect().width() - nativeSize().width());
        const int dy = qMax(0, extentRect().height() - nativeSize().height());
        QRect contentRect(QPoint(dx/2, dy/2), nativeSize());
        if (m_contentRect != contentRect) {
            m_contentRect = contentRect;
            emit contentRectChanged(m_contentRect);
        }
    }
}

