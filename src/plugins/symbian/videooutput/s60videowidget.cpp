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

#include "s60videowidget.h"
#include "s60videooutpututils.h"

#include <QtCore/QVariant>
#include <QtCore/QEvent>
#include <QtGui/QApplication>
#include <QtGui/QPainter>

#include <coemain.h>    // CCoeEnv
#include <coecntrl.h>   // CCoeControl
#include <w32std.h>

using namespace S60VideoOutputUtils;

const int NullOrdinalPosition = -1;

S60VideoWidget::S60VideoWidget(QWidget *parent)
:   QWidget(parent)
,   m_pixmap(NULL)
,   m_paintingEnabled(false)
,   m_topWinId(0)
,   m_ordinalPosition(NullOrdinalPosition)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setPalette(QPalette(Qt::black));
    setAutoFillBackground(false);
    if (!parent)
        setProperty("_q_DummyWindowSurface", true);
    S60VideoOutputUtils::setIgnoreFocusChanged(this);
}

S60VideoWidget::~S60VideoWidget()
{

}

bool S60VideoWidget::event(QEvent *event)
{
    if (event->type() == QEvent::WinIdChange)
        updateOrdinalPosition();
    return QWidget::event(event);
}

void S60VideoWidget::paintEvent(QPaintEvent *event)
{
    if (m_paintingEnabled && m_pixmap) {
        QPainter painter(this);
        if (m_pixmap->size() != m_contentRect.size())
            qWarning("pixmap size does not match expected value");
        painter.drawPixmap(m_contentRect.topLeft(), *m_pixmap);
    }
}

void S60VideoWidget::setVisible(bool visible)
{
    queueReactivateWindow();
    QWidget::setVisible(visible);
}


WId S60VideoWidget::videoWinId() const
{
    WId wid = 0;
    if (internalWinId())
        wid = internalWinId();
    else if (parentWidget() && effectiveWinId())
        wid = effectiveWinId();
    return wid;
}

void S60VideoWidget::setPixmap(const QPixmap *pixmap)
{
    m_pixmap = pixmap;
    update();
}

void S60VideoWidget::setContentRect(const QRect &rect)
{
    if (m_contentRect != rect) {
        m_contentRect = rect;
        update();
    }
}

void S60VideoWidget::setWindowBackgroundColor()
{
    if (WId wid = internalWinId())
        static_cast<RWindow *>(wid->DrawableWindow())->SetBackgroundColor(TRgb(0, 0, 0, 255));
}

WId S60VideoWidget::topWinId() const
{
    return m_topWinId;
}

void S60VideoWidget::setTopWinId(WId id)
{
    m_topWinId = id;
    updateOrdinalPosition();
#ifdef VIDEOOUTPUT_GRAPHICS_SURFACES
    // This function may be called from a paint event, so defer any window
    // manipulation until painting is complete.
    QMetaObject::invokeMethod(this, "setWindowsNonFading", Qt::QueuedConnection);
#endif
}

void S60VideoWidget::setOrdinalPosition(int ordinalPosition)
{
    m_ordinalPosition = ordinalPosition;
    updateOrdinalPosition();
}

int S60VideoWidget::ordinalPosition() const
{
    return m_ordinalPosition;
}

void S60VideoWidget::updateOrdinalPosition()
{
    if ((m_ordinalPosition != NullOrdinalPosition) && m_topWinId) {
        if (WId wid = videoWinId()) {
            int topOrdinalPosition = m_topWinId->DrawableWindow()->OrdinalPosition();
            queueReactivateWindow();
            wid->DrawableWindow()->SetOrdinalPosition(m_ordinalPosition + topOrdinalPosition);
        }
    }
}

void S60VideoWidget::queueReactivateWindow()
{
    if (!parent()) {
        if (QWidget *activeWindow = QApplication::activeWindow())
            QMetaObject::invokeMethod(this, "reactivateWindow", Qt::QueuedConnection,
                                      Q_ARG(QWidget *, activeWindow));
    }
}

void S60VideoWidget::reactivateWindow(QWidget *widget)
{
    widget->activateWindow();
}

void S60VideoWidget::setWindowsNonFading()
{
    winId()->DrawableWindow()->SetNonFading(ETrue);
    if (m_topWinId)
        m_topWinId->DrawableWindow()->SetNonFading(ETrue);
}

void S60VideoWidget::beginNativePaintEvent(const QRect &rect)
{
    Q_UNUSED(rect)
    emit beginVideoWidgetNativePaint();
}

void S60VideoWidget::endNativePaintEvent(const QRect &rect)
{
    Q_UNUSED(rect)
    CCoeEnv::Static()->WsSession().Flush();
    emit endVideoWidgetNativePaint();
}

void S60VideoWidget::setPaintingEnabled(bool enabled)
{
    if (enabled) {
#ifndef VIDEOOUTPUT_GRAPHICS_SURFACES
        setAttribute(Qt::WA_OpaquePaintEvent, false);
        setAttribute(Qt::WA_NoSystemBackground, false);
        S60VideoOutputUtils::setReceiveNativePaintEvents(this, false);
        S60VideoOutputUtils::setNativePaintMode(this, Default);
#else
        S60VideoOutputUtils::setNativePaintMode(this, Default);
#endif // !VIDEOOUTPUT_GRAPHICS_SURFACES
    } else {
#ifndef VIDEOOUTPUT_GRAPHICS_SURFACES
        setAttribute(Qt::WA_OpaquePaintEvent, true);
        setAttribute(Qt::WA_NoSystemBackground, true);
        S60VideoOutputUtils::setReceiveNativePaintEvents(this, true);
        S60VideoOutputUtils::setNativePaintMode(this, ZeroFill);
#else
        S60VideoOutputUtils::setNativePaintMode(this, Disable);
#endif // !VIDEOOUTPUT_GRAPHICS_SURFACES
        winId(); // Create native window handle
    }
    m_paintingEnabled = enabled;
    setWindowBackgroundColor();
}

void S60VideoWidget::setFullScreen(bool enabled)
{
    if (enabled)
        showFullScreen();
    else
        showMaximized();
}

