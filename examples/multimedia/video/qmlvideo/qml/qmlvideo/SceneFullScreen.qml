// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property string contentType

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: root.contentType
        source: parent.source1
        volume: parent.volume
        state: "left"

        states: [
            State {
                name: "fullScreen"
                PropertyChanges { target: content; width: content.parent.width }
                PropertyChanges { target: content; height: content.parent.height }
            }
        ]

        transitions: [
            Transition {
                ParallelAnimation {
                    PropertyAnimation {
                        property: "width"
                        easing.type: Easing.Linear
                        duration: 250
                    }
                    PropertyAnimation {
                        property: "height"
                        easing.type: Easing.Linear
                        duration: 250
                    }
                }
            }
        ]

        MouseArea {
            anchors.fill: parent
            onClicked: content.state = (content.state == "fullScreen") ? "baseState" : "fullScreen"
        }

        onVideoFramePainted: root.videoFramePainted()
    }

    Text {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            margins: 20
        }
        text: "Tap on the content to toggle full-screen mode"
        color: "#e0e0e0"
        z: 2.0
    }

    Component.onCompleted: root.content = content
}

