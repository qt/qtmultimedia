// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MEDIAINFO_H
#define MEDIAINFO_H

#include <QtTest/QTest>
#include <QtMultimedia/qmediaplayer.h>
#include <QtMultimedia/qmediametadata.h>
#include <QtMultimedia/qaudiooutput.h>
#include "../shared/testvideosink.h"
#include <chrono>

QT_USE_NAMESPACE

using namespace std::chrono;

// Extracts media metadata from a input media file
struct MediaInfo
{
    static std::optional<MediaInfo> create(const QUrl &fileLocation)
    {
        QMediaPlayer player;
        const QSignalSpy mediaStatusChanged{ &player, &QMediaPlayer::mediaStatusChanged };

        QAudioOutput audioOutput;
        player.setAudioOutput(&audioOutput);

        TestVideoSink sink;
        player.setVideoSink(&sink);

        player.setSource(fileLocation);

        // Loop through all frames to be able to count them
        player.setPlaybackRate(50); // let's speed it up
        player.play();

        const bool endReached = QTest::qWaitFor(
                [&] {
                    return mediaStatusChanged.contains(QList<QVariant>{ QMediaPlayer::EndOfMedia })
                            || mediaStatusChanged.contains(
                                    QList<QVariant>{ QMediaPlayer::InvalidMedia });
                },
                10min);

        if (!endReached)
            return {};

        MediaInfo info{};
        info.m_frameRate = player.metaData().value(QMediaMetaData::VideoFrameRate).toReal();
        info.m_size = player.metaData().value(QMediaMetaData::Resolution).toSize();

        info.m_duration = milliseconds{ player.duration() };
        info.m_frameCount = sink.m_totalFrames - 1;
        info.m_frameTimes = sink.m_frameTimes;
        info.m_hasVideo = player.hasVideo();
        info.m_hasAudio = player.hasAudio();
        return info;
    }

    int m_frameCount = 0;
    qreal m_frameRate = 0.0f;
    QSize m_size;
    milliseconds m_duration;
    bool m_hasVideo = false;
    bool m_hasAudio = false;

    std::vector<TestVideoSink::TimePoint> m_frameTimes;
};

#endif
