// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef FIXTURE_H
#define FIXTURE_H

#include <qmediaplayer.h>
#include <qaudiooutput.h>
#include <qtest.h>

QT_USE_NAMESPACE

struct Fixture : QObject
{
    Q_OBJECT
public:
    Fixture()
        : playbackStateChanged(&player, &QMediaPlayer::playbackStateChanged),
          errorOccurred(&player, &QMediaPlayer::errorOccurred),
          sourceChanged(&player, &QMediaPlayer::sourceChanged),
          mediaStatusChanged(&player, &QMediaPlayer::mediaStatusChanged),
          positionChanged(&player, &QMediaPlayer::positionChanged),
          durationChanged(&player, &QMediaPlayer::durationChanged),
          playbackRateChanged(&player, &QMediaPlayer::playbackRateChanged),
          volumeChanged(&output, &QAudioOutput::volumeChanged),
          mutedChanged(&output, &QAudioOutput::mutedChanged)
    {
        setVideoSinkAsyncFramesCounter(surface, framesCount);

        player.setAudioOutput(&output);
        player.setVideoOutput(&surface);
    }

    void clearSpies()
    {
        playbackStateChanged.clear();
        errorOccurred.clear();
        sourceChanged.clear();
        mediaStatusChanged.clear();
        positionChanged.clear();
        durationChanged.clear();
        playbackRateChanged.clear();
        volumeChanged.clear();
        mutedChanged.clear();
    }

    QMediaPlayer player;
    QAudioOutput output;
    TestVideoSink surface;
    std::atomic_int framesCount = 0;

    QSignalSpy playbackStateChanged;
    QSignalSpy errorOccurred;
    QSignalSpy sourceChanged;
    QSignalSpy mediaStatusChanged;
    QSignalSpy positionChanged;
    QSignalSpy durationChanged;
    QSignalSpy playbackRateChanged;
    QSignalSpy volumeChanged;
    QSignalSpy mutedChanged;
};

// Helper to create an object that is comparable to a QSignalSpy
using SignalList = QList<QList<QVariant>>;

#endif // FIXTURE_H
