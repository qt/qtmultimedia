/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "s60videowidgetcontrol.h"
#include "s60videowidgetdisplay.h"

S60VideoWidgetControl::S60VideoWidgetControl(QObject *parent)
:   QVideoWidgetControl(parent)
,   m_display(new S60VideoWidgetDisplay(this))
{
    connect(m_display, SIGNAL(nativeSizeChanged(QSize)),
            this, SIGNAL(nativeSizeChanged()));
}

S60VideoWidgetControl::~S60VideoWidgetControl()
{

}

QWidget *S60VideoWidgetControl::videoWidget()
{
    return m_display->widget();
}

Qt::AspectRatioMode S60VideoWidgetControl::aspectRatioMode() const
{
    return m_display->aspectRatioMode();
}

void S60VideoWidgetControl::setAspectRatioMode(Qt::AspectRatioMode ratio)
{
    m_display->setAspectRatioMode(ratio);
}

bool S60VideoWidgetControl::isFullScreen() const
{
    return m_display->isFullScreen();
}

void S60VideoWidgetControl::setFullScreen(bool fullScreen)
{
    m_display->setFullScreen(fullScreen);
}

int S60VideoWidgetControl::brightness() const
{
    return 0;
}

void S60VideoWidgetControl::setBrightness(int brightness)
{
    Q_UNUSED(brightness);
}

int S60VideoWidgetControl::contrast() const
{
    return 0;
}

void S60VideoWidgetControl::setContrast(int contrast)
{
    Q_UNUSED(contrast);
}

int S60VideoWidgetControl::hue() const
{
    return 0;
}

void S60VideoWidgetControl::setHue(int hue)
{
    Q_UNUSED(hue);
}

int S60VideoWidgetControl::saturation() const
{
    return 0;
}

void S60VideoWidgetControl::setSaturation(int saturation)
{
    Q_UNUSED(saturation);
}

S60VideoWidgetDisplay *S60VideoWidgetControl::display() const
{
    return m_display;
}

void S60VideoWidgetControl::setTopWinId(WId id)
{
    m_display->setTopWinId(id);
}

WId S60VideoWidgetControl::topWinId() const
{
    return m_display->topWinId();
}

int S60VideoWidgetControl::ordinalPosition() const
{
    return m_display->ordinalPosition();
}

void S60VideoWidgetControl::setOrdinalPosition(int ordinalPosition)
{
    m_display->setOrdinalPosition(ordinalPosition);
}

const QRect &S60VideoWidgetControl::extentRect() const
{
    return m_display->explicitExtentRect();
}

void S60VideoWidgetControl::setExtentRect(const QRect &rect)
{
    m_display->setExplicitExtentRect(rect);
}

QSize S60VideoWidgetControl::nativeSize() const
{
    return m_display->nativeSize();
}

qreal S60VideoWidgetControl::rotation() const
{
    return m_display->rotation();
}

void S60VideoWidgetControl::setRotation(qreal value)
{
    m_display->setRotation(value);
}
