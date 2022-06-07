// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia
import QtQuick.Controls

Item {
    id: root

    property bool active: false
    property bool playing: false
    visible: active && playing

    function playUrl(url) {
        playing = true
        mediaPlayer.source = url
        mediaPlayer.play()
    }

    function stop() {
        playing = false
        mediaPlayer.stop()
    }

    onActiveChanged: function() {
        if (!active)
            stop();
    }

    VideoOutput {
        anchors.fill: parent
        id: videoOutput
    }

    MediaPlayer {
        id: mediaPlayer
        videoOutput: videoOutput
        audioOutput: AudioOutput {}
    }

    HoverHandler { id: hover }

    RoundButton {
        width: 50
        height: 50
        opacity: hover.hovered && active ? 1.0 : 0.0
        anchors.centerIn: root
        radius: 25
        text: "\u25A0";
        onClicked: root.stop()

        Behavior on opacity { NumberAnimation { duration: 200 } }
    }
}
