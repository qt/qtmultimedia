// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls
import QtMultimedia

ApplicationWindow {
    id: window
    width: output.width
    height: output.height
    visible: true
    title: qsTr("QML GStreamer Pipeline")

    MediaPlayer {
        id: player
        audioOutput: AudioOutput {}
        videoOutput: output

        onSourceChanged: {
            console.log(player.source)
            window.setTitle(player.source)
        }

        onMediaStatusChanged: {
            player.play()
        }
    }

    Component.onCompleted: {
        var src

        if (Qt.application.arguments.length > 1)
            src = Qt.application.arguments[0];
        else
            // src = "gstreamer-pipeline: videotestsrc"
            // src = "gstreamer-pipeline: videotestsrc is-live=true"
            src = "gstreamer-pipeline: uridecodebin uri=http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"

        player.setSource(src)
        window.setTitle(src)
        player.play()
    }

    VideoOutput {
        id: output
        visible: true
        anchors.fill: parent
    }
}
