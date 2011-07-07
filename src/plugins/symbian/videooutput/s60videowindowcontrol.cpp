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

#include "s60videowindowcontrol.h"
#include "s60videowindowdisplay.h"

S60VideoWindowControl::S60VideoWindowControl(QObject *parent)
:   QVideoWindowControl(parent)
,   m_display(new S60VideoWindowDisplay(this))
{
    connect(m_display, SIGNAL(nativeSizeChanged(QSize)),
            this, SIGNAL(nativeSizeChanged()));
    connect(m_display, SIGNAL(fullScreenChanged(bool)),
            this, SIGNAL(fullScreenChanged(bool)));
}

S60VideoWindowControl::~S60VideoWindowControl()
{

}

WId S60VideoWindowControl::winId() const
{
    return m_display->winId();
}

void S60VideoWindowControl::setWinId(WId id)
{
    m_display->setWinId(id);
}

QRect S60VideoWindowControl::displayRect() const
{
    return m_display->displayRect();
}

void S60VideoWindowControl::setDisplayRect(const QRect &rect)
{
    m_display->setDisplayRect(rect);
}

Qt::AspectRatioMode S60VideoWindowControl::aspectRatioMode() const
{
    return m_display->aspectRatioMode();
}

void S60VideoWindowControl::setAspectRatioMode(Qt::AspectRatioMode ratio)
{
    m_display->setAspectRatioMode(ratio);
}

QSize S60VideoWindowControl::customAspectRatio() const
{
    return QSize();
}

void S60VideoWindowControl::setCustomAspectRatio(const QSize &customRatio)
{
    Q_UNUSED(customRatio);
}

void S60VideoWindowControl::repaint()
{
    m_display->repaint();
}

int S60VideoWindowControl::brightness() const
{
    return 0;
}

void S60VideoWindowControl::setBrightness(int brightness)
{
    Q_UNUSED(brightness)
}

int S60VideoWindowControl::contrast() const
{
    return 0;
}

void S60VideoWindowControl::setContrast(int contrast)
{
    Q_UNUSED(contrast)
}

int S60VideoWindowControl::hue() const
{
    return 0;
}

void S60VideoWindowControl::setHue(int hue)
{
    Q_UNUSED(hue)
}

int S60VideoWindowControl::saturation() const
{
    return 0;
}

void S60VideoWindowControl::setSaturation(int saturation)
{
    Q_UNUSED(saturation)
}

bool S60VideoWindowControl::isFullScreen() const
{
    return m_display->isFullScreen();
}

void S60VideoWindowControl::setFullScreen(bool fullScreen)
{
    m_display->setFullScreen(fullScreen);
}

QSize S60VideoWindowControl::nativeSize() const
{
    return m_display->nativeSize();
}

void S60VideoWindowControl::refreshDisplay()
{
    m_display->refreshDisplay();
}

S60VideoWindowDisplay *S60VideoWindowControl::display() const
{
    return m_display;
}

qreal S60VideoWindowControl::rotation() const
{
    return m_display->rotation();
}

void S60VideoWindowControl::setRotation(qreal value)
{
    m_display->setRotation(value);
}
