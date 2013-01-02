/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
#ifndef QT_NO_WIDGETS
#include <qvideowidgetcontrol.h>
#endif
#include <qvideorenderercontrol.h>
#include <qmediaplayer.h>

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
    "org.qt-project.qt.QT7VideoOutput/5.0"
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

#ifndef QT_NO_WIDGETS
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
#endif

QT_END_NAMESPACE

#endif
