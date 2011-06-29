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

#ifndef QT7VIDEOOUTPUTCONTROL_H
#define QT7VIDEOOUTPUTCONTROL_H

#include <QtCore/qobject.h>
#include <QtCore/qsize.h>

#include <qvideowindowcontrol.h>
#include <qvideowidgetcontrol.h>
#include <qvideorenderercontrol.h>
#include <qmediaplayer.h>

#include <QtGui/qmacdefines_mac.h>


QT_BEGIN_NAMESPACE

class QMediaPlaylist;
class QMediaPlaylistNavigator;
class QT7PlayerSession;
class QT7PlayerService;


class QT7VideoOutput {
public:
    virtual ~QT7VideoOutput() {}
    virtual void setMovie(void *movie) = 0;
    virtual void updateNaturalSize(const QSize &newSize) = 0;
};

#define QT7VideoOutput_iid \
    "com.nokia.Qt.QT7VideoOutput/1.0"
Q_DECLARE_INTERFACE(QT7VideoOutput, QT7VideoOutput_iid)

class QT7VideoWindowControl : public QVideoWindowControl, public QT7VideoOutput
{
Q_OBJECT
Q_INTERFACES(QT7VideoOutput)
public:
    virtual ~QT7VideoWindowControl() {}

protected:
    QT7VideoWindowControl(QObject *parent)
        :QVideoWindowControl(parent)
    {}
};

class QT7VideoRendererControl : public QVideoRendererControl, public QT7VideoOutput
{
Q_OBJECT
Q_INTERFACES(QT7VideoOutput)
public:
    virtual ~QT7VideoRendererControl() {}

protected:
    QT7VideoRendererControl(QObject *parent)
        :QVideoRendererControl(parent)
    {}
};

class QT7VideoWidgetControl : public QVideoWidgetControl, public QT7VideoOutput
{
Q_OBJECT
Q_INTERFACES(QT7VideoOutput)
public:
    virtual ~QT7VideoWidgetControl() {}

protected:
    QT7VideoWidgetControl(QObject *parent)
        :QVideoWidgetControl(parent)
    {}
};

QT_END_NAMESPACE

#endif
