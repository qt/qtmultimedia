/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qxavideowidgetcontrol.h"
#include "qxacommon.h"
#include "qxawidget.h"
#include <QEvent>

QXAVideoWidgetControl::QXAVideoWidgetControl(QXAPlaySession *session, QObject *parent)
    : QVideoWidgetControl(parent), mSession(session)
{
    QT_TRACE_FUNCTION_ENTRY;
    mWidget = new QXAWidget;
    if (mWidget)
        mWidget->installEventFilter(this);
    QT_TRACE_FUNCTION_EXIT;
}

QXAVideoWidgetControl::~QXAVideoWidgetControl()
{
    QT_TRACE_FUNCTION_ENTRY;
    delete mWidget;
    QT_TRACE_FUNCTION_EXIT;
}

QWidget* QXAVideoWidgetControl::videoWidget()
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return mWidget;
}

Qt::AspectRatioMode QXAVideoWidgetControl::aspectRatioMode() const
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_s_IF_p_IS_NULL(mSession, Qt::IgnoreAspectRatio);
    QT_TRACE_FUNCTION_EXIT;
    return mSession->getAspectRatioMode();
}

void QXAVideoWidgetControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setAspectRatioMode(mode);
    QT_TRACE_FUNCTION_EXIT;
}

bool QXAVideoWidgetControl::isFullScreen() const
{
    QT_TRACE_FUNCTION_ENTRY;
    bool retVal = false;
    RET_s_IF_p_IS_NULL(mWidget, retVal);
    retVal = mWidget->isFullScreen();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

void QXAVideoWidgetControl::setFullScreen(bool fullScreen)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mWidget);
    if (fullScreen == mWidget->isFullScreen())
        return;
    else if (fullScreen)
        mWidget->showFullScreen();
    else
        mWidget->showNormal();
    
    emit fullScreenChanged(fullScreen);
    
    QT_TRACE_FUNCTION_EXIT;
}

int QXAVideoWidgetControl::brightness() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWidgetControl::setBrightness(int brightness)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(brightness);
}

int QXAVideoWidgetControl::contrast() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWidgetControl::setContrast(int contrast)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(contrast);
}

int QXAVideoWidgetControl::hue() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWidgetControl::setHue(int hue)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(hue);
}

int QXAVideoWidgetControl::saturation() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWidgetControl::setSaturation(int saturation)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(saturation);
}

bool QXAVideoWidgetControl::eventFilter(QObject *object, QEvent *event)
{
    if (object == mWidget) {
        if (   event->type() == QEvent::Resize 
            || event->type() == QEvent::Move 
            || event->type() == QEvent::WinIdChange
            || event->type() == QEvent::ParentChange 
            || event->type() == QEvent::Show) {
        emit widgetUpdated();
        }
    }    
    return false;
}

WId QXAVideoWidgetControl::videoWidgetWId()
{
    if (mWidget->internalWinId())
        return mWidget->internalWinId();
    else if (mWidget->effectiveWinId())
        return mWidget->effectiveWinId();

    return NULL;
}
