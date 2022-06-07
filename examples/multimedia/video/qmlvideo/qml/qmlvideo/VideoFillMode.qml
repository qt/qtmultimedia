// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Scene {
    id: root

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: "video"
        source: parent.source1
        volume: parent.volume
        onVideoFramePainted: root.videoFramePainted()
    }

    Button {
        id: button
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        width: Math.max(parent.width, parent.height) / 5
        height: root.buttonHeight
        text: "PreserveAspectFit"
        onClicked: {
            if (!content.dummy) {
                var video = content.contentItem()
                if (video.fillMode === VideoOutput.Stretch) {
                    video.fillMode = VideoOutput.PreserveAspectFit
                    text = "PreserveAspectFit"
                } else if (video.fillMode === VideoOutput.PreserveAspectFit) {
                    video.fillMode = VideoOutput.PreserveAspectCrop
                    text = "PreserveAspectCrop"
                } else {
                    video.fillMode = VideoOutput.Stretch
                    text = "Stretch"
                }
            }
        }
    }

    Component.onCompleted: root.content = content
}
