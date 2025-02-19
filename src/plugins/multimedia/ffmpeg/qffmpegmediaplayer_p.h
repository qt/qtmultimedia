// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#ifndef QFFMPEGMEDIAPLAYER_H
#define QFFMPEGMEDIAPLAYER_H

#include <QtMultimedia/private/qplatformmediaplayer_p.h>
#include <qmediametadata.h>
#include <qtimer.h>
#include <qpointer.h>
#include <qfuture.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpeg_p.h>
#include <QtFFmpegMediaPluginImpl/private/qffmpegmediadataholder_p.h>

QT_BEGIN_NAMESPACE

namespace QFFmpeg {
class CancelToken;

class PlaybackEngine;
} // namespace QFFmpeg

class QPlatformAudioOutput;

class QFFmpegMediaPlayer : public QObject, public QPlatformMediaPlayer
{
    Q_OBJECT
public:
    QFFmpegMediaPlayer(QMediaPlayer *player);
    ~QFFmpegMediaPlayer() override;

    qint64 duration() const override;

    void setPosition(qint64 position) override;

    float bufferProgress() const override;

    QMediaTimeRange availablePlaybackRanges() const override;

    qreal playbackRate() const override;
    void setPlaybackRate(qreal rate) override;

    QUrl media() const override;
    const QIODevice *mediaStream() const override;
    void setMedia(const QUrl &media, QIODevice *stream) override;

    void play() override;
    void pause() override;
    void stop() override;

    void setAudioOutput(QPlatformAudioOutput *) override;

    void setAudioBufferOutput(QAudioBufferOutput *) override;

    QMediaMetaData metaData() const override;

    void setVideoSink(QVideoSink *sink) override;
    QVideoSink *videoSink() const;

    int trackCount(TrackType) override;
    QMediaMetaData trackMetaData(TrackType type, int streamNumber) override;
    int activeTrack(TrackType) override;
    void setActiveTrack(TrackType, int streamNumber) override;
    void setLoops(int loops) override;

private:
    void runPlayback();
    void handleIncorrectMedia(QMediaPlayer::MediaStatus status);
    void setMediaAsync(QFFmpeg::MediaDataHolder::Maybe mediaDataHolder,
                       const std::shared_ptr<QFFmpeg::CancelToken> &cancelToken);

    void mediaStatusChanged(QMediaPlayer::MediaStatus);

private slots:
    void updatePosition();
    void endOfStream();
    void error(int error, const QString &errorString)
    {
        QPlatformMediaPlayer::error(error, errorString);
    }
    void onLoopChanged();
    void onBuffered();

private:
    QTimer m_positionUpdateTimer;
    QMediaPlayer::PlaybackState m_requestedStatus = QMediaPlayer::StoppedState;

    using PlaybackEngine = QFFmpeg::PlaybackEngine;

    std::unique_ptr<PlaybackEngine> m_playbackEngine;
    QPlatformAudioOutput *m_audioOutput = nullptr;
    QPointer<QAudioBufferOutput> m_audioBufferOutput;
    QPointer<QVideoSink> m_videoSink;

    QUrl m_url;
    QPointer<QIODevice> m_device;
    float m_playbackRate = 1.;
    float m_bufferProgress = 0.f;
    QFuture<void> m_loadMedia;
    std::shared_ptr<QFFmpeg::CancelToken> m_cancelToken; // For interrupting ongoing
                                                         // network connection attempt
};

QT_END_NAMESPACE


#endif  // QMEDIAPLAYERCONTROL_H

