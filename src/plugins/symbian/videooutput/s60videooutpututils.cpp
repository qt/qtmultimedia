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

#include "s60videooutpututils.h"

#ifdef PRIVATE_QTGUI_HEADERS_AVAILABLE
#if QT_VERSION >= 0x040601 && !defined(__WINSCW__)
#include <QtGui/private/qt_s60_p.h>
#include <QtGui/private/qwidget_p.h>
#define USE_PRIVATE_QTGUI_APIS
#endif // QT_VERSION >= 0x040601 && !defined(__WINSCW__)
#endif // PRIVATE_QTGUI_HEADERS_AVAILABLE

namespace S60VideoOutputUtils
{

void setIgnoreFocusChanged(QWidget *widget)
{
#ifdef USE_PRIVATE_QTGUI_APIS
    // Warning: if this flag is not set, the application may crash due to
    // CGraphicsContext being called from within the context of
    // QGraphicsVideoItem::paint(), when the video widget is shown.
    static_cast<QSymbianControl *>(widget->winId())->setIgnoreFocusChanged(true);
#else
    Q_UNUSED(widget)
#endif
}

void setNativePaintMode(QWidget *widget, NativePaintMode mode)
{
#ifdef USE_PRIVATE_QTGUI_APIS
    QWidgetPrivate *widgetPrivate = qt_widget_private(widget->window());
    widgetPrivate->createExtra();
    QWExtra::NativePaintMode widgetMode = QWExtra::Default;
    switch (mode) {
    case Default:
        break;
    case ZeroFill:
        widgetMode = QWExtra::ZeroFill;
        break;
    case BlitWriteAlpha:
#if QT_VERSION >= 0x040704
        widgetMode = QWExtra::BlitWriteAlpha;
#endif
        break;
    case Disable:
        widgetMode = QWExtra::Disable;
        break;
    }
    widgetPrivate->extraData()->nativePaintMode = widgetMode;
#else
    Q_UNUSED(widget)
    Q_UNUSED(mode)
#endif
}

void setNativePaintMode(WId wid, NativePaintMode mode)
{
#ifdef USE_PRIVATE_QTGUI_APIS
    QWidget *window = static_cast<QSymbianControl *>(wid)->widget()->window();
    setNativePaintMode(window, mode);
#else
    Q_UNUSED(wid)
    Q_UNUSED(mode)
#endif
}

void setReceiveNativePaintEvents(QWidget *widget, bool enabled)
{
#ifdef USE_PRIVATE_QTGUI_APIS
    QWidgetPrivate *widgetPrivate = qt_widget_private(widget);
    widgetPrivate->createExtra();
    widgetPrivate->extraData()->receiveNativePaintEvents = enabled;
#else
    Q_UNUSED(widget)
    Q_UNUSED(enabled)
#endif
}

} // namespace S60VideoOutputUtils

