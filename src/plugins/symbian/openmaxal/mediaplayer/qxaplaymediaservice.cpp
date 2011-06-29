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


#include <QString>
#include "qxaplaymediaservice.h"
#include "qxaplaysession.h"
#include "qxamediaplayercontrol.h"
#include "qxacommon.h"
#include "qxavideowidgetcontrol.h"
#include "qxavideowindowcontrol.h"
#include "qxametadatacontrol.h"
#include "qxamediastreamscontrol.h"

QXAPlayMediaService::QXAPlayMediaService(QObject *parent) : QMediaService(parent)
{
    mSession = NULL;
    mMediaPlayerControl = NULL;
    mVideowidgetControl = NULL;
    mVideoWindowControl = NULL;
    mMetaDataControl = NULL;
    mMediaStreamsControl = NULL;

    mSession = new QXAPlaySession(this);
    mMediaPlayerControl = new QXAMediaPlayerControl(mSession, this);
    mMetaDataControl = new QXAMetaDataControl(mSession, this);
    mMediaStreamsControl = new QXAMediaStreamsControl(mSession, this);
}

QXAPlayMediaService::~QXAPlayMediaService()
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
}

QMediaControl *QXAPlayMediaService::requestControl(const char *name)
{
    if (qstrcmp(name, QMediaPlayerControl_iid) == 0) {
        return mMediaPlayerControl;
    }
    else if (qstrcmp(name, QVideoWidgetControl_iid) == 0) {
        if (!mVideowidgetControl) {
            mVideowidgetControl = new QXAVideoWidgetControl(mSession, this);
                if (mSession && mVideowidgetControl)
                    mSession->setVideoWidgetControl(mVideowidgetControl);
        }
        return mVideowidgetControl;
    }
    else if (qstrcmp(name, QVideoWindowControl_iid) == 0) {
        if (!mVideoWindowControl) {
            mVideoWindowControl = new QXAVideoWindowControl(mSession, this);
            if (mSession && mVideoWindowControl)
                mSession->setVideoWindowControl(mVideoWindowControl);
        }
        return mVideoWindowControl;
    }
    else if (qstrcmp(name,QMetaDataReaderControl_iid) == 0) {
        return mMetaDataControl;
    }
    else if (qstrcmp(name,QMediaStreamsControl_iid) == 0) {
        return mMediaStreamsControl;
    }

    return 0;
}

void QXAPlayMediaService::releaseControl(QMediaControl *control)
{    
    if (control == mVideowidgetControl) {
        if (mSession)
            mSession->unsetVideoWidgetControl(qobject_cast<QXAVideoWidgetControl*>(control));
        mVideowidgetControl = NULL;
    }
    else if (control == mVideoWindowControl) {
        if (mSession)
            mSession->unsetVideoWindowControl(qobject_cast<QXAVideoWindowControl*>(control));
        mVideoWindowControl = NULL;
    }
}

