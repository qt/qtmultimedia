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

#ifndef QGSTREAMERSTREAMSCONTROL_H
#define QGSTREAMERSTREAMSCONTROL_H

#include <qmediastreamscontrol.h>

QT_BEGIN_NAMESPACE

class QGstreamerPlayerSession;

class QGstreamerStreamsControl : public QMediaStreamsControl
{
    Q_OBJECT
public:
    QGstreamerStreamsControl(QGstreamerPlayerSession *session, QObject *parent);
    virtual ~QGstreamerStreamsControl();

    virtual int streamCount();
    virtual StreamType streamType(int streamNumber);

    virtual QVariant metaData(int streamNumber, const QString &key);

    virtual bool isActive(int streamNumber);
    virtual void setActive(int streamNumber, bool state);

private:
    QGstreamerPlayerSession *m_session;
};

QT_END_NAMESPACE

#endif // QGSTREAMERSTREAMSCONTROL_H

