// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    property int itemHeight: 25
    property string sceneSource: ""

    ListModel {
        id: videolist
        ListElement { name: "Multi"; source: "SceneMulti.qml" }
        ListElement { name: "Video"; source: "VideoBasic.qml" }
        ListElement { name: "Drag"; source: "VideoDrag.qml" }
        ListElement { name: "Fillmode"; source: "VideoFillMode.qml" }
        ListElement { name: "Fullscreen"; source: "VideoFullScreen.qml" }
        ListElement { name: "Fullscreen-inverted"; source: "VideoFullScreenInverted.qml" }
        ListElement { name: "Metadata"; source: "VideoMetadata.qml" }
        ListElement { name: "Move"; source: "VideoMove.qml" }
        ListElement { name: "Overlay"; source: "VideoOverlay.qml" }
        ListElement { name: "Playback Rate"; source: "VideoPlaybackRate.qml" }
        ListElement { name: "Resize"; source: "VideoResize.qml" }
        ListElement { name: "Rotate"; source: "VideoRotate.qml" }
        ListElement { name: "Spin"; source: "VideoSpin.qml" }
        ListElement { name: "Seek"; source: "VideoSeek.qml" }
    }

    ListModel {
        id: cameralist
        ListElement { name: "Camera"; source: "CameraBasic.qml" }
        ListElement { name: "Drag"; source: "CameraDrag.qml" }
        ListElement { name: "Fullscreen"; source: "CameraFullScreen.qml" }
        ListElement { name: "Fullscreen-inverted"; source: "CameraFullScreenInverted.qml" }
        ListElement { name: "Move"; source: "CameraMove.qml" }
        ListElement { name: "Overlay"; source: "CameraOverlay.qml" }
        ListElement { name: "Resize"; source: "CameraResize.qml" }
        ListElement { name: "Rotate"; source: "CameraRotate.qml" }
        ListElement { name: "Spin"; source: "CameraSpin.qml" }
    }

    Component {
        id: leftDelegate
        Item {
            width: root.width / 2
            height: 0.8 * itemHeight

            Button {
                anchors.fill: parent
                anchors.margins: 5
                anchors.rightMargin: 2.5
                anchors.bottomMargin: 0
                text: name
                onClicked: root.sceneSource = source
            }
        }
    }

    Component {
        id: rightDelegate
        Item {
            width: root.width / 2
            height: 0.8 * itemHeight

            Button {
                anchors.fill: parent
                anchors.margins: 5
                anchors.leftMargin: 2.5
                anchors.bottomMargin: 0
                text: name
                onClicked: root.sceneSource = source
            }
        }
    }

    Flickable {
        anchors.fill: parent
        contentHeight: (itemHeight * videolist.count) + 10
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
                    delegate: leftDelegate
                }
            }

            Column {
                Repeater {
                    model: cameralist
                    delegate: rightDelegate
                }
            }
        }
    }
}
