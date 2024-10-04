/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QMEDIAPLAYERCONTROL_H
#define QMEDIAPLAYERCONTROL_H

#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediatimerange.h>
#include <QtMultimedia/qaudiodevice.h>
#include <QtMultimedia/qmediametadata.h>

#include <QtCore/qpair.h>

QT_BEGIN_NAMESPACE

class QMediaStreamsControl;
class QPlatformAudioOutput;

class Q_MULTIMEDIA_EXPORT QPlatformMediaPlayer
{
public:
    virtual ~QPlatformMediaPlayer();
    virtual QMediaPlayer::PlaybackState state() const { return m_state; }
    virtual QMediaPlayer::MediaStatus mediaStatus() const { return m_status; };

    virtual qint64 duration() const = 0;

    virtual qint64 position() const = 0;
    virtual void setPosition(qint64 position) = 0;

    virtual float bufferProgress() const = 0;

    virtual bool isAudioAvailable() const { return m_audioAvailable; }
    virtual bool isVideoAvailable() const { return m_videoAvailable; }

    virtual bool isSeekable() const { return m_seekable; }

    virtual QMediaTimeRange availablePlaybackRanges() const = 0;

    virtual qreal playbackRate() const = 0;
    virtual void setPlaybackRate(qreal rate) = 0;

    virtual QUrl media() const = 0;
    virtual const QIODevice *mediaStream() const = 0;
    virtual void setMedia(const QUrl &media, QIODevice *stream) = 0;

    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    virtual bool streamPlaybackSupported() const { return false; }

    virtual void setAudioOutput(QPlatformAudioOutput *) {}

    virtual QMediaMetaData metaData() const { return {}; }

    virtual void setVideoSink(QVideoSink * /*sink*/) = 0;

    // media streams
    enum TrackType { VideoStream, AudioStream, SubtitleStream, NTrackTypes };

    virtual int trackCount(TrackType) { return 0; };
    virtual QMediaMetaData trackMetaData(TrackType /*type*/, int /*streamNumber*/) { return QMediaMetaData(); }
    virtual int activeTrack(TrackType) { return -1; }
    virtual void setActiveTrack(TrackType, int /*streamNumber*/) {}

    void durationChanged(qint64 duration) { player->durationChanged(duration); }
    void positionChanged(qint64 position) { player->positionChanged(position); }
    void audioAvailableChanged(bool audioAvailable) {
        if (m_audioAvailable == audioAvailable)
            return;
        m_audioAvailable = audioAvailable;
        player->hasAudioChanged(audioAvailable);
    }
    void videoAvailableChanged(bool videoAvailable) {
        if (m_videoAvailable == videoAvailable)
            return;
        m_videoAvailable = videoAvailable;
        player->hasVideoChanged(videoAvailable);
    }
    void seekableChanged(bool seekable) {
        if (m_seekable == seekable)
            return;
        m_seekable = seekable;
        player->seekableChanged(seekable);
    }
    void playbackRateChanged(qreal rate) { player->playbackRateChanged(rate); }
    void bufferProgressChanged(float progress) { player->bufferProgressChanged(progress); }
    void metaDataChanged() { player->metaDataChanged(); }
    void tracksChanged() { player->tracksChanged(); }
    void activeTracksChanged() { player->activeTracksChanged(); }

    void stateChanged(QMediaPlayer::PlaybackState newState);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void error(int error, const QString &errorString);

    void resetCurrentLoop() { m_currentLoop = 0; }
    bool doLoop() {
        return isSeekable() && (m_loops < 0 || ++m_currentLoop < m_loops);
    }
    int loops() { return m_loops; }
    void setLoops(int loops) {
        if (m_loops == loops)
            return;
        m_loops = loops;
        Q_EMIT player->loopsChanged();
    }

    virtual void *nativePipeline() { return nullptr; }

    // private API, the purpose is getting GstPipeline
    static void *nativePipeline(QMediaPlayer *player);
protected:
    explicit QPlatformMediaPlayer(QMediaPlayer *parent = nullptr)
        : player(parent)
    {}
private:
    QMediaPlayer *player = nullptr;
    QMediaPlayer::MediaStatus m_status = QMediaPlayer::NoMedia;
    QMediaPlayer::PlaybackState m_state = QMediaPlayer::StoppedState;
    bool m_seekable = false;
    bool m_videoAvailable = false;
    bool m_audioAvailable = false;
    int m_loops = 1;
    int m_currentLoop = 0;
};

QT_END_NAMESPACE


#endif  // QMEDIAPLAYERCONTROL_H

