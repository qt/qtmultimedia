// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root

    property real itemWidth: (width / 3) - 40
    property real itemTopMargin: 50

    QtObject {
        id: contentProxy
        function initialize() {
            video1.initialize()
            video2.initialize()
        }
    }

    Component {
        id: startStopComponent

        Rectangle {
            id: root
            color: "transparent"

            function content() {
                return root.parent
            }

            Text {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.bottom
                    margins: 20
                }
                text: content() ? content().started ? "Tap to stop" : "Tap to start" : ""
                color: "#e0e0e0"
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (content().started)
                        content().stop()
                    else
                        content().start()
                }
            }
        }
    }

    Content {
        id: video1
        anchors {
            left: parent.left
            leftMargin: 10
            top: parent.top
            topMargin: root.itemTopMargin
        }
        autoStart: false
        contentType: "video"
        showBorder: true
        showFrameRate: started
        source: parent.source1
        width: itemWidth
        volume: parent.volume

        Loader {
            id: video1StartStopLoader
            onLoaded: {
                item.parent = video1
                item.anchors.fill = video1
            }
        }

        onInitialized: video1StartStopLoader.sourceComponent = startStopComponent
    }

    Rectangle {
        id: cameraHolder
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: root.itemTopMargin
        }
        border.width: 1
        border.color: "white"
        color: "transparent"
        width: itemWidth
        height: width
        property bool started: false

        Loader {
            id: cameraLoader
            onLoaded: {
                item.parent = cameraHolder
                item.anchors.centerIn = cameraHolder
                item.contentType = "camera"
                item.showFrameRate = true
                item.width = itemWidth
                item.z = 1.0
                cameraErrorConnection.target = item
                item.initialize()
            }
        }

        Loader {
            id: cameraStartStopLoader
            sourceComponent: startStopComponent
            onLoaded: {
                item.parent = cameraHolder
                item.anchors.fill = cameraHolder
                item.z = 2.0
            }
        }

        Connections {
            id: cameraErrorConnection
            onError: {
                console.log("[qmlvideo] SceneMulti.camera.onError")
                cameraHolder.stop()
            }
            ignoreUnknownSignals: true
        }

        function start() {
            cameraLoader.source = "Content.qml"
            cameraHolder.started = true
        }

        function stop() {
            cameraLoader.source = ""
            cameraHolder.started = false
        }
    }

    Content {
        id: video2
        anchors {
            right: parent.right
            rightMargin: 10
            top: parent.top
            topMargin: root.itemTopMargin
        }
        autoStart: false
        contentType: "video"
        showBorder: true
        showFrameRate: started
        source: parent.source2
        width: itemWidth
        volume: parent.volume

        Loader {
            id: video2StartStopLoader
            onLoaded: {
                item.parent = video2
                item.anchors.fill = video2
            }
        }

        onInitialized: video2StartStopLoader.sourceComponent = startStopComponent
    }

    Component.onCompleted: root.content = contentProxy
}
