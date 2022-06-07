// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property int margin: 20
    property int delta: 30
    property string contentType

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: root.contentType
        source: parent.source1
        volume: parent.volume
        onVideoFramePainted: root.videoFramePainted()
    }

    Button {
        id: rotatePositiveButton
        anchors {
            right: parent.right
            bottom: rotateNegativeButton.top
            margins: parent.margins
        }
        width: Math.max(parent.width, parent.height) / 10
        height: root.buttonHeight
        text: "Rotate +" + delta
        onClicked: content.rotation = content.rotation + delta
    }

    Button {
        id: rotateNegativeButton
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        width: Math.max(parent.width, parent.height) / 10
        height: root.buttonHeight
        text: "Rotate -" + delta
        onClicked: content.rotation = content.rotation - delta
    }

    Button {
        id: rotateValueButton
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
            margins: parent.margins
        }
        width: Math.max(parent.width, parent.height) / 25
        height: root.buttonHeight
        enabled: false
        text: content.rotation % 360
    }

    Component.onCompleted: root.content = content
}
