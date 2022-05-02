/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#ifndef MFPLAYERCONTROL_H
#define MFPLAYERCONTROL_H

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

#include "QUrl.h"
#include "qplatformmediaplayer_p.h"

#include <QtCore/qcoreevent.h>

QT_USE_NAMESPACE

class MFPlayerSession;

class MFPlayerControl : public QPlatformMediaPlayer
{
public:
    MFPlayerControl(QMediaPlayer *player);
    ~MFPlayerControl();

    QMediaPlayer::PlaybackState state() const override;

    QMediaPlayer::MediaStatus mediaStatus() const override;

    qint64 duration() const override;

    qint64 position() const override;
    void setPosition(qint64 position) override;

    float bufferProgress() const override;

    bool isAudioAvailable() const override;
    bool isVideoAvailable() const override;

    bool isSeekable() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &media, QIODevice *stream) override;

    void play() override;
    void pause() override;
    void stop() override;

    bool streamPlaybackSupported() const override { return true; }

    QMediaMetaData metaData() const override;

    void setAudioOutput(QPlatformAudioOutput *output) override;

    void setVideoSink(QVideoSink *sink) override;

    void setActiveTrack(TrackType type, int index) override;
    int activeTrack(TrackType type) override;
    int trackCount(TrackType type) override;
    QMediaMetaData trackMetaData(TrackType type, int trackNumber) override;

    void handleStatusChanged();
    void handleTracksChanged();
    void handleVideoAvailable();
    void handleAudioAvailable();
    void handleDurationUpdate(qint64 duration);
    void handleSeekableUpdate(bool seekable);
    void handleError(QMediaPlayer::Error errorCode, const QString& errorString, bool isFatal);

private:
    void changeState(QMediaPlayer::PlaybackState state);
    void resetAudioVideoAvailable();
    void refreshState();

    QMediaPlayer::PlaybackState m_state;
    bool m_stateDirty;

    bool     m_videoAvailable;
    bool     m_audioAvailable;
    qint64   m_duration;
    bool     m_seekable;

    QIODevice *m_stream;
    QUrl m_media;
    MFPlayerSession *m_session;
};

#endif
