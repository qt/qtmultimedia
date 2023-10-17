// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Item {
    id: videoPreview
    property alias source : player.source
    signal closed

    MediaPlayer {
        id: player

        //switch back to viewfinder after playback finished
        onMediaStatusChanged: {
            if (mediaStatus == MediaPlayer.EndOfMedia)
                videoPreview.closed();
        }
        onSourceChanged: {
            if (videoPreview.visible && source !== "")
                play();
        }

        videoOutput: output
        audioOutput: AudioOutput {
        }
    }

    VideoOutput {
        id: output
        anchors.fill : parent
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            videoPreview.closed();
        }
    }
}

