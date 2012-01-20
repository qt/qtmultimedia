/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Toolkit.
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

#ifndef QT7PLAYERSERVICE_H
#define QT7PLAYERSERVICE_H

#include <QtCore/qobject.h>
#include <QtCore/qset.h>
#include <qmediaservice.h>


QT_BEGIN_NAMESPACE
class QMediaMetaData;
class QMediaPlayerControl;
class QMediaPlaylist;
class QMediaPlaylistNavigator;
class QT7PlayerControl;
class QT7PlayerMetaDataControl;
class QT7VideoWindowControl;
class QT7VideoWidgetControl;
class QT7VideoRendererControl;
class QT7VideoOutput;
class QT7PlayerSession;

class QT7PlayerService : public QMediaService
{
Q_OBJECT
public:
    QT7PlayerService(QObject *parent = 0);
    ~QT7PlayerService();

    QMediaControl* requestControl(const char *name);
    void releaseControl(QMediaControl *control);

private:
    QT7PlayerSession *m_session;
    QT7PlayerControl *m_control;
    QMediaControl * m_videoOutput;
    QT7PlayerMetaDataControl *m_playerMetaDataControl;
};

QT_END_NAMESPACE

#endif
