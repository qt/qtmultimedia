// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MEDIAPLAYERSTATE_H
#define MEDIAPLAYERSTATE_H

#include <qlist.h>
#include <qmediatimerange.h>
#include <qmediametadata.h>
#include <qtestcase.h>

QT_USE_NAMESPACE

/*!
 * Helper class that simplifies testing the state of
 * a media player against an expected state.
 *
 * Use the COMPARE_MEDIA_PLAYER_STATE_EQ macro to compare
 * the media player state against the expected state.
 *
 * Individual properties can be ignored by comparison by
 * assigning the std::nullopt value to the property of the
 * expected state.
 */
struct MediaPlayerState
{
    std::optional<QList<QMediaMetaData>> audioTracks;
    std::optional<QList<QMediaMetaData>> videoTracks;
    std::optional<QList<QMediaMetaData>> subtitleTracks;
    std::optional<int> activeAudioTrack;
    std::optional<int> activeVideoTrack;
    std::optional<int> activeSubtitleTrack;
    std::optional<QAudioOutput*> audioOutput;
    std::optional<QObject*> videoOutput;
    std::optional<QVideoSink*> videoSink;
    std::optional<QUrl> source;
    std::optional<QIODevice const*> sourceDevice;
    std::optional<QMediaPlayer::PlaybackState> playbackState;
    std::optional<QMediaPlayer::MediaStatus> mediaStatus;
    std::optional<qint64> duration;
    std::optional<qint64> position;
    std::optional<bool> hasAudio;
    std::optional<bool> hasVideo;
    std::optional<float> bufferProgress;
    std::optional<QMediaTimeRange> bufferedTimeRange;
    std::optional<bool> isSeekable;
    std::optional<qreal> playbackRate;
    std::optional<bool> isPlaying;
    std::optional<int> loops;
    std::optional<QMediaPlayer::Error> error;
    std::optional<bool> isAvailable;
    std::optional<QMediaMetaData> metaData;

    /*!
     * Read the state from an existing media player
     */
    MediaPlayerState(const QMediaPlayer &player)
        : audioTracks{ player.audioTracks() },
          videoTracks{ player.videoTracks() },
          subtitleTracks{ player.subtitleTracks() },
          activeAudioTrack{ player.activeAudioTrack() },
          activeVideoTrack{ player.activeVideoTrack() },
          activeSubtitleTrack{ player.activeSubtitleTrack() },
          audioOutput{ player.audioOutput() },
          videoOutput{ player.videoOutput() },
          videoSink{ player.videoSink() },
          source{ player.source() },
          sourceDevice{ player.sourceDevice() },
          playbackState{ player.playbackState() },
          mediaStatus{ player.mediaStatus() },
          duration{ player.duration() },
          position{ player.position() },
          hasAudio{ player.hasAudio() },
          hasVideo{ player.hasVideo() },
          bufferProgress{ player.bufferProgress() },
          bufferedTimeRange{ player.bufferedTimeRange() },
          isSeekable{ player.isSeekable() },
          playbackRate{ player.playbackRate() },
          isPlaying{ player.isPlaying() },
          loops{ player.loops() },
          error{ player.error() },
          isAvailable{ player.isAvailable() },
          metaData{ player.metaData() }
    {
    }

    /*!
     * Creates the default state of a media player. The default state
     * is the state the player should have when it is default constructed.
     */
    static MediaPlayerState defaultState()
    {
        MediaPlayerState state{};
        state.audioTracks = QList<QMediaMetaData>{};
        state.videoTracks = QList<QMediaMetaData>{};
        state.subtitleTracks = QList<QMediaMetaData>{};
        state.activeAudioTrack = -1;
        state.activeVideoTrack = -1;
        state.activeSubtitleTrack = -1;
        state.audioOutput = nullptr;
        state.videoOutput = nullptr;
        state.videoSink = nullptr;
        state.source = QUrl{};
        state.sourceDevice = nullptr;
        state.playbackState = QMediaPlayer::StoppedState;
        state.mediaStatus = QMediaPlayer::NoMedia;
        state.duration = 0;
        state.position = 0;
        state.hasAudio = false;
        state.hasVideo = false;
        state.bufferProgress = 1.0f;
        state.bufferedTimeRange = QMediaTimeRange{};
        state.isSeekable = false;
        state.playbackRate = static_cast<qreal>(1);
        state.isPlaying = false;
        state.loops = 1;
        state.error = QMediaPlayer::NoError;
        state.isAvailable = true;
        state.metaData = QMediaMetaData{};
        return state;
    }

private:
    MediaPlayerState() = default;

};

#define COMPARE_EQ_IGNORE_OPTIONAL(actual, expected) \
    do {                                             \
        if ((expected).has_value()) {                \
            QCOMPARE_EQ(actual, expected);           \
        }                                            \
    } while (false)

#define COMPARE_MEDIA_PLAYER_STATE_EQ(actual, expected)                                           \
    do {                                                                                          \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).audioTracks, (expected).audioTracks);                 \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).videoTracks, (expected).videoTracks);                 \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).subtitleTracks, (expected).subtitleTracks);           \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).activeAudioTrack, (expected).activeAudioTrack);       \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).activeVideoTrack, (expected).activeVideoTrack);       \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).activeSubtitleTrack, (expected).activeSubtitleTrack); \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).source, (expected).source);                           \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).sourceDevice, (expected).sourceDevice);               \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).playbackState, (expected).playbackState);             \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).mediaStatus, (expected).mediaStatus);                 \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).duration, (expected).duration);                       \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).position, (expected).position);                       \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).hasAudio, (expected).hasAudio);                       \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).hasVideo, (expected).hasVideo);                       \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).bufferProgress, (expected).bufferProgress);           \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).bufferedTimeRange, (expected).bufferedTimeRange);     \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).isSeekable, (expected).isSeekable);                   \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).playbackRate, (expected).playbackRate);               \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).isPlaying, (expected).isPlaying);                     \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).loops, (expected).loops);                             \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).error, (expected).error);                             \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).isAvailable, (expected).isAvailable);                 \
        COMPARE_EQ_IGNORE_OPTIONAL((actual).metaData, (expected).metaData);                       \
    } while (false)

#endif // MEDIAPLAYERSTATE_H
