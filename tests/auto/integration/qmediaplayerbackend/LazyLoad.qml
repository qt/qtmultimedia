// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtMultimedia

Rectangle {
    id: root
    width: 600
    height: 800
    color: "black"

    Component {
        id: videoOutputComponent

        Item {
            objectName: "videoPlayer"
            property alias mediaPlayer: mediaPlayer
            property alias videoOutput: videoOutput
            property alias videoSink: videoOutput.videoSink

            property alias playbackState: mediaPlayer.playbackState
            property alias error: mediaPlayer.error


            MediaPlayer {
                id: mediaPlayer
                objectName: "mediaPlayer"
                source: "qrc:/testdata/colors.mp4"
            }
            VideoOutput {
                id: videoOutput
                objectName: "videoOutput"
                anchors.fill: parent
            }
        }
    }

    Loader {
        id: loader
        objectName: "loader"
        sourceComponent: videoOutputComponent
        anchors.fill: parent
        active: false
        onActiveChanged: {
            if (active) {
                loader.item.mediaPlayer.videoOutput = loader.item.videoOutput
                loader.item.mediaPlayer.play()
            }
        }
    }
}
