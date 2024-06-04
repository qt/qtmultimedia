// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef FIXTURE_H
#define FIXTURE_H

#include <qmediaplayer.h>
#include <qaudiooutput.h>
#include <qtest.h>
#include <qsignalspy.h>

#include "fake.h"
#include "testvideosink.h"

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
          metadataChanged(&player, &QMediaPlayer::metaDataChanged),
          volumeChanged(&output, &QAudioOutput::volumeChanged),
          mutedChanged(&output, &QAudioOutput::mutedChanged),
          bufferProgressChanged(&player, &QMediaPlayer::bufferProgressChanged),
          destroyed(&player, &QObject::destroyed)
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
        metadataChanged.clear();
        volumeChanged.clear();
        mutedChanged.clear();
        bufferProgressChanged.clear();
        destroyed.clear();
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
    QSignalSpy metadataChanged;
    QSignalSpy volumeChanged;
    QSignalSpy mutedChanged;
    QSignalSpy bufferProgressChanged;
    QSignalSpy destroyed;
};

// Helper to create an object that is comparable to a QSignalSpy
using SignalList = QList<QList<QVariant>>;

struct TestSubtitleSink : QObject
{
    Q_OBJECT

public Q_SLOTS:
    void addSubtitle(QString string)
    {
        QMetaObject::invokeMethod(this, [this, string = std::move(string)]() mutable {
            subtitles.append(std::move(string));
        });
    }

public:
    QStringList subtitles;
};

#endif // FIXTURE_H
