// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

pragma ComponentBehavior: Bound

Rectangle {
    id: root
    property int itemHeight: 25
    property string sceneSource: ""

    ListModel {
        id: videolist
        ListElement { name: qsTr("Multi"); source: "SceneMulti.qml" }
        ListElement { name: qsTr("Video"); source: "VideoBasic.qml" }
        ListElement { name: qsTr("Drag"); source: "VideoDrag.qml" }
        ListElement { name: qsTr("Fillmode"); source: "VideoFillMode.qml" }
        ListElement { name: qsTr("Fullscreen"); source: "VideoFullScreen.qml" }
        ListElement { name: qsTr("Fullscreen-inverted"); source: "VideoFullScreenInverted.qml" }
        ListElement { name: qsTr("Metadata"); source: "VideoMetadata.qml" }
        ListElement { name: qsTr("Move"); source: "VideoMove.qml" }
        ListElement { name: qsTr("Overlay"); source: "VideoOverlay.qml" }
        ListElement { name: qsTr("Playback Rate"); source: "VideoPlaybackRate.qml" }
        ListElement { name: qsTr("Resize"); source: "VideoResize.qml" }
        ListElement { name: qsTr("Rotate"); source: "VideoRotate.qml" }
        ListElement { name: qsTr("Spin"); source: "VideoSpin.qml" }
        ListElement { name: qsTr("Seek"); source: "VideoSeek.qml" }
    }

    ListModel {
        id: cameralist
        ListElement { name: qsTr("Camera"); source: "CameraBasic.qml" }
        ListElement { name: qsTr("Drag"); source: "CameraDrag.qml" }
        ListElement { name: qsTr("Fullscreen"); source: "CameraFullScreen.qml" }
        ListElement { name: qsTr("Fullscreen-inverted"); source: "CameraFullScreenInverted.qml" }
        ListElement { name: qsTr("Move"); source: "CameraMove.qml" }
        ListElement { name: qsTr("Overlay"); source: "CameraOverlay.qml" }
        ListElement { name: qsTr("Resize"); source: "CameraResize.qml" }
        ListElement { name: qsTr("Rotate"); source: "CameraRotate.qml" }
        ListElement { name: qsTr("Spin"); source: "CameraSpin.qml" }
    }

    Component {
        id: buttonDelegate
        Button {
            required property string name
            required property string source

            width: root.width / 2
            height: root.itemHeight

            text: name
            padding: 3
            onClicked: root.sceneSource = source
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: (root.itemHeight * videolist.count) + 10
        clip: true

        Row {
            id: layout
            anchors {
                fill: parent
                topMargin: 5
                bottomMargin: 5
            }

            Column {
                Repeater {
                    model: videolist
                    delegate: buttonDelegate
                }
            }

            Column {
                Repeater {
                    model: cameralist
                    delegate: buttonDelegate
                }
            }
        }
    }
}
