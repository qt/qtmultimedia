// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia

// Plays back a recorded file, scaled to fit. Playback starts right away, since the
// player is only created while it is shown.
ColumnLayout {
    id: root

    required property url source

    function formatTime(milliseconds) {
        const totalSeconds = Math.floor(milliseconds / 1000)
        const minutes = Math.floor(totalSeconds / 60)
        const seconds = totalSeconds % 60
        return String(minutes).padStart(2, "0") + ":" + String(seconds).padStart(2, "0")
    }

    MediaPlayer {
        id: player

        source: root.source
        videoOutput: videoOutput
        audioOutput: AudioOutput {}
        autoPlay: true

        onErrorOccurred: (error, errorString) => {
            console.log("Recording playback error: " + errorString)
        }
    }

    VideoOutput {
        id: videoOutput

        Layout.fillWidth: true
        Layout.fillHeight: true
        fillMode: VideoOutput.PreserveAspectFit
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        visible: player.error !== MediaPlayer.NoError
        text: "Error: " + player.errorString
    }

    RowLayout {
        Layout.fillWidth: true

        Button {
            enabled: player.mediaStatus !== MediaPlayer.NoMedia
                && player.mediaStatus !== MediaPlayer.InvalidMedia
            text: player.playbackState === MediaPlayer.PlayingState ? "Pause" : "Play"
            onClicked: () => {
                if (player.playbackState === MediaPlayer.PlayingState)
                    player.pause()
                else
                    player.play()
            }
        }

        Slider {
            Layout.fillWidth: true
            enabled: player.seekable
            from: 0
            to: player.duration
            value: player.position
            onMoved: () => {
                player.position = value
            }
        }

        Label {
            text: root.formatTime(player.position) + " / " + root.formatTime(player.duration)
        }
    }

    Label {
        Layout.fillWidth: true
        wrapMode: Text.WrapAnywhere
        text: root.source.toString()
    }
}
