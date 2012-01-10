/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef MOCKVIDEOWINDOWCONTROL_H
#define MOCKVIDEOWINDOWCONTROL_H

#if defined(QT_MULTIMEDIA_MOCK_WIDGETS)

#include "qvideowindowcontrol.h"

class MockVideoWindowControl : public QVideoWindowControl
{
public:
    MockVideoWindowControl(QObject *parent = 0) : QVideoWindowControl(parent) {}
    WId winId() const { return 0; }
    void setWinId(WId) {}
    QRect displayRect() const { return QRect(); }
    void setDisplayRect(const QRect &) {}
    bool isFullScreen() const { return false; }
    void setFullScreen(bool) {}
    void repaint() {}
    QSize nativeSize() const { return QSize(); }
    Qt::AspectRatioMode aspectRatioMode() const { return Qt::KeepAspectRatio; }
    void setAspectRatioMode(Qt::AspectRatioMode) {}
    int brightness() const { return 0; }
    void setBrightness(int) {}
    int contrast() const { return 0; }
    void setContrast(int) {}
    int hue() const { return 0; }
    void setHue(int) {}
    int saturation() const { return 0; }
    void setSaturation(int) {}
};

#endif // QT_MULTIMEDIA_MOCK_WIDGETS
#endif // MOCKVIDEOWINDOWCONTROL_H
