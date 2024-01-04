// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

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
                PropertyChanges {
                    content.width: content.parent.width
                    content.height: content.parent.height
                }
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

    Label {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            margins: 20
        }
        text: qsTr("Tap on the content to toggle full-screen mode")
        z: 2.0
    }

    Component.onCompleted: root.content = content
}

