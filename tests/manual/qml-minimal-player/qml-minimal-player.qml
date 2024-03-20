// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import QtQuick.Controls
import QtMultimedia

ApplicationWindow {
    id: window
    width: 800
    height: 600
    visible: true
    title: qsTr("QmlMinimalPlayer")

    MediaPlayer {
        id: player
        audioOutput: AudioOutput {
            onMutedChanged: {
                console.log("muted", player.audioOutput.muted)
            }
        }
        videoOutput: output

        onMediaStatusChanged: {
            console.log("status", player.mediaStatus);

            if (player.mediaStatus === MediaPlayer.LoadedMedia)
                player.play()
        }
    }

    Component.onCompleted: {
        if (Qt.application.arguments.length > 1)
            player.setSource(Qt.application.arguments[1])
        else
            console.log("Please specify a video source")
    }

    VideoOutput {
        id: output
        visible: true
    }
}
