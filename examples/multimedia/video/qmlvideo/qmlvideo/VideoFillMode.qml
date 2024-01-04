// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
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
        text: qsTr("PreserveAspectFit")
        // qmllint disable
        onClicked: {
            if (!content.dummy) {
                let video = content.contentItem
                if (video.fillMode === VideoOutput.Stretch) {
                    video.fillMode = VideoOutput.PreserveAspectFit
                    text = qsTr("PreserveAspectFit")
                } else if (video.fillMode === VideoOutput.PreserveAspectFit) {
                    video.fillMode = VideoOutput.PreserveAspectCrop
                    text = qsTr("PreserveAspectCrop")
                } else {
                    video.fillMode = VideoOutput.Stretch
                    text = qsTr("Stretch")
                }
            }
        }
        // qmllint enable
    }

    Component.onCompleted: root.content = content
}
