// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

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

            signal start
            signal stop

            Label {
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.bottom
                    margins: 20
                }
                // qmllint disable
                text:  root.started ? qsTr("Tap to stop") : qsTr("Tap to start")
                // qmllint enable
            }

            MouseArea {
                anchors.fill: parent
                // qmllint disable
                onClicked: {
                    if (root.started)
                        root.stop()
                    else
                        root.start()
                }
                // qmllint enable
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
        width: root.itemWidth
        volume: parent.volume

        Loader {
            id: video1StartStopLoader

            property bool started: parent.started

            onLoaded: {
                item.parent = video1
                item.anchors.fill = video1
                item.start.connect(video1.start)
                item.stop.connect(video1.stop)
            }
        }

        onInitialized: video1StartStopLoader.sourceComponent = startStopComponent
    }

    Rectangle {
        id: cameraHolder

        property bool started: false

        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            topMargin: root.itemTopMargin
        }
        border.width: 1
        border.color: palette.base
        color: "transparent"
        width: root.itemWidth
        height: width

        Loader {
            id: cameraLoader
            onLoaded: {
                item.parent = cameraHolder
                item.anchors.fill = cameraHolder
                item.contentType = "camera"
                item.showFrameRate = true
                item.z = 1.0
                cameraErrorConnection.target = item
                item.initialize()
            }
        }

        Loader {
            id: cameraStartStopLoader

            property bool started: parent.started

            sourceComponent: startStopComponent
            onLoaded: {
                item.parent = cameraHolder
                item.anchors.fill = cameraHolder
                item.z = 2.0
                item.start.connect(cameraHolder.start)
                item.stop.connect(cameraHolder.stop)
            }
        }

        Connections {
            id: cameraErrorConnection
            function onError() {
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
        width: root.itemWidth
        volume: parent.volume

        Loader {
            id: video2StartStopLoader

            property bool started: parent.started

            onLoaded: {
                item.parent = video2
                item.anchors.fill = video2
                item.start.connect(video2.start)
                item.stop.connect(video2.stop)
            }
        }

        onInitialized: video2StartStopLoader.sourceComponent = startStopComponent
    }

    Component.onCompleted: root.content = contentProxy
}
