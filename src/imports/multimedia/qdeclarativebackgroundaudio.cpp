/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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
#include "qdeclarativebackgroundaudio_p.h"
#include <qmediabackgroundplaybackcontrol.h>
#include <qmediaservice.h>

void QDeclarativeBackgroundAudio::classBegin()
{
    setObject(this, Q_MEDIASERVICE_BACKGROUNDMEDIAPLAYER);
    if (m_mediaService) {
        m_backgroundPlaybackControl =
                static_cast<QMediaBackgroundPlaybackControl*>(
                        m_mediaService->requestControl(QMediaBackgroundPlaybackControl_iid));
        if (m_backgroundPlaybackControl) {
            connect(m_backgroundPlaybackControl, SIGNAL(acquired()), this, SIGNAL(acquiredChanged()));
            connect(m_backgroundPlaybackControl, SIGNAL(lost()), this, SIGNAL(acquiredChanged()));
        } else {
            qWarning("can not get QMediaBackgroundPlaybackControl!");
        }
    } else {
        qWarning("Unable to get any background mediaplayer!");
    }
    emit mediaObjectChanged();

    //Note: we are not calling QDeclarativeAudio::classBegin here,
    //otherwise there will be conflict for setObject().
}

void QDeclarativeBackgroundAudio::componentComplete()
{
    QDeclarativeAudio::componentComplete();
}

QDeclarativeBackgroundAudio::QDeclarativeBackgroundAudio(QObject *parent)
    : QDeclarativeAudio(parent)
    , m_backgroundPlaybackControl(0)
{

}

QDeclarativeBackgroundAudio::~QDeclarativeBackgroundAudio()
{
    if (m_backgroundPlaybackControl)
        m_mediaService->releaseControl(m_backgroundPlaybackControl);
}

/*!
    \qmlproperty string BackgroundAudio::contextId

    This property holds the unique contextId for the application

    When a new contextId is set, the previously set contextId will be released automatically.
*/
QString QDeclarativeBackgroundAudio::contextId() const
{
    return m_contextId;
}

void QDeclarativeBackgroundAudio::setContextId(QString contextId)
{
    if (m_contextId == contextId)
        return;
    m_contextId = contextId;
    if (m_backgroundPlaybackControl)
        m_backgroundPlaybackControl->setContextId(m_contextId);
    emit contextIdChanged();
}

/*!
    \qmlproperty bool BackgroundAudio::acquired

    This property indicates whether the application holds the playback resource in music daemon
*/
bool QDeclarativeBackgroundAudio::isAcquired() const
{
    if (!m_backgroundPlaybackControl)
        return false;
    return m_backgroundPlaybackControl->isAcquired();
}

/*!
    \qmlmethod BackgroundAudio::acquire()

    try to acquire the playback resource in music daemon
*/
void QDeclarativeBackgroundAudio::acquire()
{
    if (isAcquired() || !m_backgroundPlaybackControl)
        return;
    m_backgroundPlaybackControl->acquire();
}

/*!
    \qmlmethod BackgroundAudio::acquire()

    try to release the playback resource in music daemon
*/
void QDeclarativeBackgroundAudio::release()
{
    if (!isAcquired() || !m_backgroundPlaybackControl)
        return;
    m_backgroundPlaybackControl->release();
}
