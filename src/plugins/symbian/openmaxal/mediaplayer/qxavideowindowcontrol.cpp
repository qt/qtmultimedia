/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qxavideowindowcontrol.h"
#include "qxacommon.h"
#include <QEvent>
#include "qxaplaysession.h"

QXAVideoWindowControl::QXAVideoWindowControl(QXAPlaySession* session, QObject *parent)
    :QVideoWindowControl(parent),
     mWid(NULL),
     mWidget(NULL),
     mAspectRatioMode(Qt::IgnoreAspectRatio),
     mSession(session)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
}

QXAVideoWindowControl::~QXAVideoWindowControl()
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mWidget)
        mWidget->removeEventFilter(this);
    QT_TRACE_FUNCTION_EXIT;
}

WId QXAVideoWindowControl::winId() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return mWid;
}

void QXAVideoWindowControl::setWinId(WId id)
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mWid != id) {        
        if (mWidget)
            mWidget->removeEventFilter(this);
        mWid = id;
        mWidget = QWidget::find(mWid);
        if (mWidget)
            mWidget->installEventFilter(this);
        emit windowUpdated();
    }
    QT_TRACE_FUNCTION_EXIT;
}

QRect QXAVideoWindowControl::displayRect() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return mDisplayRect;
}

void QXAVideoWindowControl::setDisplayRect(const QRect &rect)
{
    QT_TRACE_FUNCTION_ENTRY;
    mDisplayRect = rect;
    QT_TRACE_FUNCTION_EXIT;
}

bool QXAVideoWindowControl::isFullScreen() const
{
    QT_TRACE_FUNCTION_ENTRY;
    bool retVal(false);
    if (mWidget)
        retVal = mWidget->isFullScreen();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

void QXAVideoWindowControl::setFullScreen(bool fullScreen)
{
    QT_TRACE_FUNCTION_ENTRY;
    if (mWidget && (fullScreen != mWidget->isFullScreen())) {
        if (fullScreen)
            mWidget->showFullScreen();
        else
            mWidget->showNormal();
        emit fullScreenChanged(fullScreen);
    }
    QT_TRACE_FUNCTION_EXIT;
}

void QXAVideoWindowControl::repaint()
{
}

QSize QXAVideoWindowControl::nativeSize() const
{
    QT_TRACE_FUNCTION_ENTRY;    
    QSize size(0, 0);
    RET_s_IF_p_IS_NULL(mSession, size);
    QVariant sizeBundle = mSession->metaData(QtMultimediaKit::Resolution);
    QString metadata = sizeBundle.toString();
    if (!metadata.isNull() && !metadata.isEmpty()) {
        int xIndex = metadata.indexOf('x');
        if (xIndex > 0) {
            size.setWidth(metadata.left(xIndex).toInt());
            xIndex = metadata.length() - (xIndex + 1);
            if (xIndex > 0)
                size.setHeight(metadata.right(xIndex).toInt());
        }
    }    
    QT_TRACE_FUNCTION_EXIT;
    return size;
}

Qt::AspectRatioMode QXAVideoWindowControl::aspectRatioMode() const
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_s_IF_p_IS_NULL(mSession, Qt::IgnoreAspectRatio);
    QT_TRACE_FUNCTION_EXIT;
    return mSession->getAspectRatioMode();
}

void QXAVideoWindowControl::setAspectRatioMode(Qt::AspectRatioMode mode)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setAspectRatioMode(mode);
    QT_TRACE_FUNCTION_EXIT;
}

int QXAVideoWindowControl::brightness() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWindowControl::setBrightness(int brightness)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(brightness);
}

int QXAVideoWindowControl::contrast() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWindowControl::setContrast(int contrast)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(contrast);
}

int QXAVideoWindowControl::hue() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWindowControl::setHue(int hue)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(hue);
}

int QXAVideoWindowControl::saturation() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return 0;
}

void QXAVideoWindowControl::setSaturation(int saturation)
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    Q_UNUSED(saturation);
}

bool QXAVideoWindowControl::eventFilter(QObject *object, QEvent *event)
{
    if (object == mWidget) {
        if (event->type() == QEvent::Resize 
            || event->type() == QEvent::Move 
            || event->type() == QEvent::WinIdChange
            || event->type() == QEvent::ParentChange 
            || event->type() == QEvent::Show) {
            emit windowUpdated();
        }
    } 
    return false;
}
