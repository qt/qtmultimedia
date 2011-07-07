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

#include "s60videowindowdisplay.h"
#include "s60videooutpututils.h"
#include <QtCore/QVariant>
#include <coecntrl.h>
#include <w32std.h>

using namespace S60VideoOutputUtils;

S60VideoWindowDisplay::S60VideoWindowDisplay(QObject *parent)
:   S60VideoDisplay(parent)
,   m_winId(0)
,   m_bitmap(0)
{
    parent->setProperty("colorKey", Qt::transparent);
}

S60VideoWindowDisplay::~S60VideoWindowDisplay()
{

}

WId S60VideoWindowDisplay::winId() const
{
    return m_winId;
}

QRect S60VideoWindowDisplay::extentRect() const
{
    QRect rect = displayRect();
#ifndef VIDEOOUTPUT_GRAPHICS_SURFACES
    if (RWindow *window = windowHandle()) {
        const TPoint windowPos = window->AbsPosition();
        rect.translate(windowPos.iX, windowPos.iY);
    }
#endif // VIDEOOUTPUT_GRAPHICS_SURFACES
    return rect;
}

void S60VideoWindowDisplay::setFrame(const CFbsBitmap &bitmap)
{
    m_bitmap = const_cast<CFbsBitmap*>(&bitmap);
    if (m_winId) {
        // Blit the bitmap into the native window owned by m_winId
        CWindowGc &gc = m_winId->SystemGc();
        RWindow *window = windowHandle();
        gc.Activate(*window);
        const QPoint offsetQ = displayRect().topLeft() + contentRect().topLeft();
        const TPoint offsetT(offsetQ.x(), offsetQ.y());
        const TRect winRect(offsetT, m_bitmap->SizeInPixels());
        window->BeginRedraw(winRect);
        gc.BitBlt(offsetT, m_bitmap);
        window->EndRedraw();
        gc.Deactivate();
    }
}

void S60VideoWindowDisplay::setWinId(WId id)
{
    if (m_winId != id) {
        m_winId = id;
        if (m_winId) {
            static_cast<RWindow *>(m_winId->DrawableWindow())->SetBackgroundColor(TRgb(0, 0, 0, 0));
#ifndef VIDEOOUTPUT_GRAPHICS_SURFACES
            if (QSysInfo::s60Version() >= QSysInfo::SV_S60_5_0)
                S60VideoOutputUtils::setNativePaintMode(m_winId, BlitWriteAlpha);
#endif // !VIDEOOUTPUT_GRAPHICS_SURFACES
        }
        emit windowHandleChanged(windowHandle());
    }
}

void S60VideoWindowDisplay::setDisplayRect(const QRect &rect)
{
    if (m_displayRect != rect) {
        // If QGraphicsVideoItem moves out of screen, display rect is invalidated
        if (rect == QRect(QPoint(-1,-1), QSize(1,1)))
            emit visibilityChanged(false);
        else
            emit visibilityChanged(true);
        m_displayRect = rect;
        emit displayRectChanged(extentRect(), clipRect());
    }
}

QRect S60VideoWindowDisplay::displayRect() const
{
    return m_displayRect;
}

void S60VideoWindowDisplay::repaint()
{
    // TODO
}

void S60VideoWindowDisplay::refreshDisplay()
{
    emit displayRectChanged(extentRect(), clipRect());
}

