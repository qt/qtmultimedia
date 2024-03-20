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
    title: qsTr("GStreamer RTP receiver")

    MediaPlayer {
        id: player
        videoOutput: output

        source: "udp://127.0.0.1:50004"
    }

    Component.onCompleted: {
        player.play()
    }

    VideoOutput {
        id: output
        anchors.fill: parent
    }
}
