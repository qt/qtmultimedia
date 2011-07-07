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


#include <QtCore/qurl.h>

#include "qxamediastreamscontrol.h"
#include "qxaplaysession.h"
#include "qxacommon.h"

QXAMediaStreamsControl::QXAMediaStreamsControl(QXAPlaySession *session, QObject *parent)
   :QMediaStreamsControl(parent), mSession(session)
{
    QT_TRACE_FUNCTION_ENTRY;
    connect(mSession, SIGNAL(activeStreamsChanged()),
            this, SIGNAL(activeStreamsChanged()));
    connect(mSession, SIGNAL(streamsChanged()),
            this, SIGNAL(streamsChanged()));
    QT_TRACE_FUNCTION_EXIT;
}

QXAMediaStreamsControl::~QXAMediaStreamsControl()
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
}

bool  QXAMediaStreamsControl::isActive (int stream)
{
    RET_s_IF_p_IS_NULL(mSession, false);
    return mSession->isStreamActive(stream);
}

QVariant  QXAMediaStreamsControl::metaData (int stream, QtMultimediaKit::MetaData key)
{
    QVariant var;
    RET_s_IF_p_IS_NULL(mSession, var);
    QT_TRACE_FUNCTION_ENTRY;
    var = mSession->metaData(stream, key);
    QT_TRACE_FUNCTION_EXIT;
    return var;
}

void  QXAMediaStreamsControl::setActive (int stream, bool state)
{
    Q_UNUSED(stream);
    Q_UNUSED(state);
}

int  QXAMediaStreamsControl::streamCount()
{
    RET_s_IF_p_IS_NULL(mSession, 0);
    return mSession->streamCount();
}

QMediaStreamsControl::StreamType QXAMediaStreamsControl::streamType (int stream)
{
    RET_s_IF_p_IS_NULL(mSession, QMediaStreamsControl::UnknownStream);
    return mSession->streamType(stream);
}

