/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MOCKPLAYERSERVICE_H
#define MOCKPLAYERSERVICE_H

#include "private/qmediaplatformplayerinterface_p.h"

#include "mockmediaplayercontrol.h"
#include "mockmediastreamscontrol.h"

class MockMediaPlayerService : public QMediaPlatformPlayerInterface
{
    Q_OBJECT

public:
    MockMediaPlayerService()
    {
        mockControl = new MockMediaPlayerControl;
        mockStreamsControl = new MockStreamsControl;
    }

    ~MockMediaPlayerService()
    {
        delete mockControl;
        delete mockStreamsControl;
    }

    MockMediaPlayerControl *player() { return mockControl; }

    QMediaStreamsControl *streams() { return mockStreamsControl; } // ###

    void setState(QMediaPlayer::State state) { emit mockControl->stateChanged(mockControl->_state = state); }
    void setState(QMediaPlayer::State state, QMediaPlayer::MediaStatus status) {
        mockControl->_state = state;
        mockControl->_mediaStatus = status;
        emit mockControl->mediaStatusChanged(status);
        emit mockControl->stateChanged(state);
    }
    void setMediaStatus(QMediaPlayer::MediaStatus status) { emit mockControl->mediaStatusChanged(mockControl->_mediaStatus = status); }
    void setIsValid(bool isValid) { mockControl->_isValid = isValid; }
    void setMedia(QUrl media) { mockControl->_media = media; }
    void setDuration(qint64 duration) { mockControl->_duration = duration; }
    void setPosition(qint64 position) { mockControl->_position = position; }
    void setSeekable(bool seekable) { mockControl->_isSeekable = seekable; }
    void setVolume(int volume) { mockControl->_volume = volume; }
    void setMuted(bool muted) { mockControl->_muted = muted; }
    void setVideoAvailable(bool videoAvailable) { mockControl->_videoAvailable = videoAvailable; }
    void setBufferStatus(int bufferStatus) { mockControl->_bufferStatus = bufferStatus; }
    void setPlaybackRate(qreal playbackRate) { mockControl->_playbackRate = playbackRate; }
    void setError(QMediaPlayer::Error error) { mockControl->_error = error; emit mockControl->error(mockControl->_error, mockControl->_errorString); }
    void setErrorString(QString errorString) { mockControl->_errorString = errorString; emit mockControl->error(mockControl->_error, mockControl->_errorString); }

    void reset()
    {
        mockControl->_state = QMediaPlayer::StoppedState;
        mockControl->_mediaStatus = QMediaPlayer::UnknownMediaStatus;
        mockControl->_error = QMediaPlayer::NoError;
        mockControl->_duration = 0;
        mockControl->_position = 0;
        mockControl->_volume = 0;
        mockControl->_muted = false;
        mockControl->_bufferStatus = 0;
        mockControl->_videoAvailable = false;
        mockControl->_isSeekable = false;
        mockControl->_playbackRate = 0.0;
        mockControl->_media = QUrl();
        mockControl->_stream = 0;
        mockControl->_isValid = false;
        mockControl->_errorString = QString();
        mockControl->hasAudioRole = true;
        mockControl->hasCustomAudioRole = true;
    }

    MockMediaPlayerControl *mockControl;
    MockStreamsControl *mockStreamsControl;
};



#endif // MOCKPLAYERSERVICE_H
