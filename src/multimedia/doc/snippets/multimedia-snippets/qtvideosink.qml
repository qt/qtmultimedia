// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

//! [complete]
Item {
    MediaPlayer {
        id: mediaplayer
        source: "file:///test.mp4"
        videoOutput: videoOutput
        audioOutput: AudioOutput {

        }
    }

    VideoOutput {
        id: videoOutput
        anchors.fill: parent
    }

    MouseArea {
        id: playArea
        anchors.fill: parent
        onPressed: mediaplayer.play();
    }
}
//! [complete]
