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


#include <QtCore/qurl.h>
#include "qxamediaplayercontrol.h"
#include "qxaplaysession.h"
#include "qxacommon.h"

QXAMediaPlayerControl::QXAMediaPlayerControl(QXAPlaySession *session, QObject *parent)
   :QMediaPlayerControl(parent), mSession(session)
{
    QT_TRACE_FUNCTION_ENTRY;
    connect(mSession, SIGNAL(mediaChanged(const QMediaContent &)),
            this, SIGNAL(mediaChanged(const QMediaContent& )));
    connect(mSession, SIGNAL(durationChanged(qint64)),
            this, SIGNAL(durationChanged(qint64)));
    connect(mSession, SIGNAL(positionChanged(qint64)),
            this, SIGNAL(positionChanged(qint64)));
    connect(mSession, SIGNAL(stateChanged(QMediaPlayer::State)),
            this, SIGNAL(stateChanged(QMediaPlayer::State)));
    connect(mSession, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),
            this, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)));
    connect(mSession, SIGNAL(volumeChanged(int)),
            this, SIGNAL(volumeChanged(int)));
    connect(mSession, SIGNAL(mutedChanged(bool)),
            this, SIGNAL(mutedChanged(bool)));
    connect(mSession, SIGNAL(audioAvailableChanged(bool)),
            this, SIGNAL(audioAvailableChanged(bool)));
    connect(mSession, SIGNAL(videoAvailableChanged(bool)),
            this, SIGNAL(videoAvailableChanged(bool)));
    connect(mSession,SIGNAL(bufferStatusChanged(int)),
            this, SIGNAL(bufferStatusChanged(int)));
    connect(mSession, SIGNAL(seekableChanged(bool)),
            this, SIGNAL(seekableChanged(bool)));
    connect(mSession, SIGNAL(availablePlaybackRangesChanged(const QMediaTimeRange&)),
            this, SIGNAL(availablePlaybackRangesChanged(const QMediaTimeRange&)));
    connect(mSession, SIGNAL(playbackRateChanged(qreal)),
            this, SIGNAL(playbackRateChanged(qreal)));
    connect(mSession, SIGNAL(error(int, const QString &)),
            this, SIGNAL(error(int, const QString &)));
    QT_TRACE_FUNCTION_EXIT;
}

QXAMediaPlayerControl::~QXAMediaPlayerControl()
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
}

QMediaPlayer::State QXAMediaPlayerControl::state() const
{
    QT_TRACE_FUNCTION_ENTRY;
    QMediaPlayer::State retVal = QMediaPlayer::StoppedState;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->state();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

QMediaPlayer::MediaStatus QXAMediaPlayerControl::mediaStatus() const
{
    QT_TRACE_FUNCTION_ENTRY;
    QMediaPlayer::MediaStatus retVal = QMediaPlayer::NoMedia;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->mediaStatus();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

qint64 QXAMediaPlayerControl::duration() const
{
    QT_TRACE_FUNCTION_ENTRY;
    qint64 retVal = 0;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->duration();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

qint64 QXAMediaPlayerControl::position() const
{
    QT_TRACE_FUNCTION_ENTRY;
    qint64 retVal = 0;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->position();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

void QXAMediaPlayerControl::setPosition(qint64 pos)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setPosition(pos);
    QT_TRACE_FUNCTION_EXIT;
}

int QXAMediaPlayerControl::volume() const
{
    QT_TRACE_FUNCTION_ENTRY;
    int retVal = 0;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->volume();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

void QXAMediaPlayerControl::setVolume(int volume)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setVolume(volume);
    QT_TRACE_FUNCTION_EXIT;
}

bool QXAMediaPlayerControl::isMuted() const
{    
    QT_TRACE_FUNCTION_ENTRY;
    bool retVal = false;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->isMuted();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

void QXAMediaPlayerControl::setMuted(bool muted)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setMuted(muted);
    QT_TRACE_FUNCTION_EXIT;
}

int QXAMediaPlayerControl::bufferStatus() const
{
    QT_TRACE_FUNCTION_ENTRY;
    int retVal = 0;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->bufferStatus();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

bool QXAMediaPlayerControl::isAudioAvailable() const
{
    QT_TRACE_FUNCTION_ENTRY;
    bool retVal = false;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->isAudioAvailable();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

bool QXAMediaPlayerControl::isVideoAvailable() const
{
    QT_TRACE_FUNCTION_ENTRY;
    bool retVal = false;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->isVideoAvailable();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

bool QXAMediaPlayerControl::isSeekable() const
{
    QT_TRACE_FUNCTION_ENTRY;
    bool retVal = false;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->isSeekable();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

QMediaTimeRange QXAMediaPlayerControl::availablePlaybackRanges() const
{
    QT_TRACE_FUNCTION_ENTRY;
    QMediaTimeRange retVal;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    if (mSession->isSeekable())
        retVal.addInterval(0, mSession->duration());
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

float QXAMediaPlayerControl::playbackRate() const
{
    QT_TRACE_FUNCTION_ENTRY;
    float retVal = 0;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->playbackRate();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

void QXAMediaPlayerControl::setPlaybackRate(float rate)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setPlaybackRate(rate);
    QT_TRACE_FUNCTION_EXIT;
}

QMediaContent QXAMediaPlayerControl::media() const
{
    QT_TRACE_FUNCTION_ENTRY;
    QMediaContent retVal;
    RET_s_IF_p_IS_NULL(mSession, retVal);
    retVal = mSession->media();
    QT_TRACE_FUNCTION_EXIT;
    return retVal;
}

const QIODevice *QXAMediaPlayerControl::mediaStream() const
{
    QT_TRACE_FUNCTION_ENTRY_EXIT;
    return mStream;
}

void QXAMediaPlayerControl::setMedia(const QMediaContent &content, QIODevice *stream)
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->setMedia(content);
    mStream = stream;
    QT_TRACE_FUNCTION_EXIT;
}

void QXAMediaPlayerControl::play()
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->play();
    QT_TRACE_FUNCTION_EXIT;
}

void QXAMediaPlayerControl::pause()
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->pause();
    QT_TRACE_FUNCTION_EXIT;
}

void QXAMediaPlayerControl::stop()
{
    QT_TRACE_FUNCTION_ENTRY;
    RET_IF_p_IS_NULL(mSession);
    mSession->stop();
    QT_TRACE_FUNCTION_EXIT;
}
