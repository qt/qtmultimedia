// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtMultimedia

//! [complete]
Item {
    MediaPlayer {
        id: mediaplayer
        source: "http://commondatastorage.googleapis.com/gtv-videos-bucket/sample/BigBuckBunny.mp4"
        videoOutput: [v1, v2]
        audioOutput: AudioOutput {

        }
    }

    VideoOutput {
        id: v1
        anchors.fill: parent
    }

    Window {
        visible: true
        width: 480; height: 320
        VideoOutput {
            id: v2
            anchors.fill: parent
        }
    }
}
//! [complete]
