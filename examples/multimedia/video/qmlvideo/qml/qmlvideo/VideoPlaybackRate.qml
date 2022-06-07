// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

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
        width: Math.max(parent.width, parent.height) / 10
        height: root.buttonHeight
        text: "Increase"
        onClicked: {
            var video = content.contentItem()
            video.playbackRate += delta
        }
    }

    Button {
        id: decreaseButton
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        width: Math.max(parent.width, parent.height) / 10
        height: root.buttonHeight
        text: "Decrease"
        onClicked: {
            var video = content.contentItem()
            video.playbackRate -= delta
        }
    }

    Button {
        id: valueButton
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        width: Math.max(parent.width, parent.height) / 25
        height: root.buttonHeight
        enabled: false
        text: Math.round(10 * content.contentItem().playbackRate) / 10
    }

    Component.onCompleted: root.content = content
}
