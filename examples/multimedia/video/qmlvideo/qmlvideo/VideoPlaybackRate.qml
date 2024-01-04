// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Scene {
    id: root
    property int margin: 20
    property real delta: 0.1
    property string contentType: "video"

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
        id: increaseButton
        anchors {
            right: parent.right
            bottom: decreaseButton.top
            margins: parent.margins
        }
        text: qsTr("Increase")
        onClicked: {
            let video = (content.contentItem as VideoItem)
            video.playbackRate += root.delta
        }
    }

    Button {
        id: decreaseButton
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        text: qsTr("Decrease")
        onClicked: {
            let video = (content.contentItem as VideoItem)
            video.playbackRate -= root.delta
        }
    }

    Button {
        id: valueButton
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        enabled: false
        // qmllint disable
        text: Math.round(10 * content.contentItem?.playbackRate ?? 1) / 10
        // qmllint enable
    }

    Component.onCompleted: root.content = content
}
