// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
