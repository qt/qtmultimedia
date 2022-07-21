// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Dialogs

Rectangle {
    id: root
    anchors.fill: parent
    color: "black"

    property string source1
    property string source2
    property color bgColor: "black"
    property real volume: 0.25
    property bool perfMonitorsLogging: false
    property bool perfMonitorsVisible: false

    QtObject {
        id: d
        property int itemHeight: root.height > root.width ? root.width / 10 : root.height / 10
        property int buttonHeight: 0.8 * itemHeight
        property int margins: 5
    }

    Loader {
        id: performanceLoader

        Connections {
            target: inner
            function onVisibleChanged() {
                if (performanceLoader.item)
                    performanceLoader.item.enabled = !inner.visible
            }
            ignoreUnknownSignals: true
        }

        function init() {
            var enabled = root.perfMonitorsLogging || root.perfMonitorsVisible
            source = enabled ? "../performancemonitor/PerformanceItem.qml" : ""
        }

        onLoaded: {
            item.parent = root
            item.anchors.fill = root
            item.logging = root.perfMonitorsLogging
            item.displayed = root.perfMonitorsVisible
            item.enabled = false
            item.init()
        }
    }

    Rectangle {
        id: inner
        anchors.fill: parent
        color: root.bgColor

        Button {
            id: openFile1Button
            anchors {
                top: parent.top
                left: parent.left
                right: exitButton.left
                margins: d.margins
            }
            bgColor: "#212121"
            bgColorSelected: "#757575"
            textColorSelected: "white"
            height: d.buttonHeight
            text: (root.source1 == "") ? "Select file 1" : root.source1
            onClicked: {
                fileBrowser.setFirstSource = true
                fileBrowser.open()
            }
        }

        Button {
            id: openFile2Button
            anchors {
                top: openFile1Button.bottom
                left: parent.left
                right: exitButton.left
                margins: d.margins
            }
            bgColor: "#212121"
            bgColorSelected: "#757575"
            textColorSelected: "white"
            height: d.buttonHeight
            text: (root.source2 == "") ? "Select file 2" : root.source2
            onClicked: {
                fileBrowser.setFirstSource = false
                fileBrowser.open()
            }
        }

        Button {
            id: exitButton
            anchors {
                top: parent.top
                right: parent.right
                margins: d.margins
            }
            bgColor: "#212121"
            bgColorSelected: "#757575"
            textColorSelected: "white"
            width: parent.width / 10
            height: d.buttonHeight
            text: "Exit"
            onClicked: Qt.quit()
        }

        Row {
            id: modes
            anchors.top: openFile2Button.bottom
            anchors.margins: 0
            anchors.topMargin: 5
            Button {
                width: root.width / 2
                height: 0.8 * d.itemHeight
                bgColor: "#212121"
                radius: 0
                text: "Video Modes"
                enabled: false
            }
            Button {
                width: root.width / 2
                height: 0.8 * d.itemHeight
                bgColor: "#212121"
                radius: 0
                text: "Camera Modes"
                enabled: false
            }
        }

        Rectangle {
            id: divider
            height: 1
            width: parent.width
            color: "black"
            anchors.top: modes.bottom
        }

        SceneSelectionPanel {
            id: sceneSelectionPanel
            itemHeight: d.itemHeight
            color: "#212121"
            anchors {
                top: divider.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
            }
            radius: 0
            onSceneSourceChanged: {
                sceneLoader.source = sceneSource
                var scene = null
                var innerVisible = true
                if (sceneSource == "") {
                    if (performanceLoader.item)
                        performanceLoader.item.videoActive = false
                } else {
                    scene = sceneLoader.item
                    if (scene) {
                        if (scene.contentType === "video" && source1 === "") {
                            errorDialog.show("You must first select a video file")
                            sceneSource = ""
                        } else {
                            scene.parent = root
                            scene.color = root.bgColor
                            scene.buttonHeight = d.buttonHeight
                            scene.source1 = source1
                            scene.source2 = source2
                            scene.volume = volume
                            scene.anchors.fill = root
                            scene.close.connect(closeScene)
                            scene.content.initialize()
                            innerVisible = false
                        }
                    }
                }
                videoFramePaintedConnection.target = scene
                inner.visible = innerVisible
            }
        }
    }

    Loader {
        id: sceneLoader
    }

    Connections {
        id: videoFramePaintedConnection
        function onVideoFramePainted() {
            if (performanceLoader.item)
                performanceLoader.item.videoFramePainted()
        }
        ignoreUnknownSignals: true
    }

    FileDialog {
        id: fileBrowser
        property bool setFirstSource
        onAccepted: {
            if (setFirstSource)
               root.source1 = currentFile
            else
               root.source2 = currentFile
        }
    }

    ErrorDialog {
        id: errorDialog
        anchors.fill: root
        dialogWidth: d.itemHeight * 5
        dialogHeight: d.itemHeight * 3
        enabled: false
    }

    // Called from main() once root properties have been set
    function init() {
        performanceLoader.init()
        fileBrowser.currentFolder = videoPath
    }

    function qmlFramePainted() {
        if (performanceLoader.item)
            performanceLoader.item.qmlFramePainted()
    }

    function closeScene() {
        sceneSelectionPanel.sceneSource = ""
    }
}
