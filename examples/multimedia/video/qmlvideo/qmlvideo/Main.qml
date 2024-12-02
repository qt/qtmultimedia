// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import frequencymonitor

Rectangle {
    id: root
    anchors.fill: parent
    color: palette.window

    property bool perfMonitorsLogging: false
    property bool perfMonitorsVisible: false

    Loader {
        id: performanceLoader

        Connections {
            target: columnLayout
            function onVisibleChanged() {
                if (performanceLoader.item)
                    performanceLoader.item.enabled = !columnLayout.visible
            }
            ignoreUnknownSignals: true
        }

        Component {
            id: performanceItem
            PerformanceItem {}
        }

        function init() {
            var enabled = root.perfMonitorsLogging || root.perfMonitorsVisible
            sourceComponent = enabled ? performanceItem : undefined
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

    ColumnLayout {
        id: columnLayout
        anchors.fill: parent
        spacing: 5

        Button {
            id: openFile1Button
            text: (VideoSingleton.source1 == '') ? qsTr("Select file 1") : VideoSingleton.source1
            Component.onCompleted: console.log("source1: " + VideoSingleton.source1)
            onClicked: {
                fileDialog.setFirstSource = true
                fileDialog.open()
            }

            Layout.fillWidth: true
        }

        Button {
            id: openFile2Button
            text: (VideoSingleton.source2 == '') ? qsTr("Select file 2") : VideoSingleton.source2
            Component.onCompleted: console.log("source2: " + VideoSingleton.source2)
            onClicked: {
                fileDialog.setFirstSource = false
                fileDialog.open()
            }

            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true

            Label {
                text: qsTr("Video Modes")

                horizontalAlignment: Qt.AlignHCenter
                Layout.preferredWidth: 50
                Layout.fillWidth: true
            }
            Label {
                text: qsTr("Camera Modes")

                horizontalAlignment: Qt.AlignHCenter
                Layout.preferredWidth: 50
                Layout.fillWidth: true
            }
        }

        SceneSelectionPanel {
            id: sceneSelectionPanel
            itemHeight: Math.min(width / 10, height / 10)
            color: palette.dark
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
                        if (scene.contentType === "video" && VideoSingleton.source1 === "") {
                            errorDialog.show(qsTr("You must first select a video file"))
                            sceneSource = ""
                        } else {
                            scene.parent = root
                            scene.color = root.palette.window
                            scene.source1 = VideoSingleton.source1
                            scene.source2 = VideoSingleton.source2
                            scene.volume = VideoSingleton.volume
                            scene.anchors.fill = root
                            scene.close.connect(closeScene)
                            scene.content.initialize()
                            innerVisible = false
                        }
                    }
                }
                videoFramePaintedConnection.target = scene
                columnLayout.visible = innerVisible
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }

    Loader {
        id: sceneLoader
    }

    Connections {
        id: videoFramePaintedConnection
        // qmllint disable
        function onVideoFramePainted() {
            if (performanceLoader.item)
                performanceLoader.item.videoFramePainted()
        }
        // qmllint enable
        ignoreUnknownSignals: true
    }

    FileDialog {
        id: fileDialog
        property bool setFirstSource
        onAccepted: function() {
            if (setFirstSource)
               VideoSingleton.source1 = selectedFile
            else
               VideoSingleton.source2 = selectedFile
        }
    }

    ErrorDialog {
        id: errorDialog
        anchors.fill: root
        dialogWidth: Math.min(root.width, root.height) * 0.5
        dialogHeight: Math.min(root.width, root.height) * 0.3
        enabled: false
    }

    // Called from main() once root properties have been set
    function init() {
        performanceLoader.init()
        fileDialog.currentFolder = VideoSingleton.videoPath
    }

    // qmllint disable
    function qmlFramePainted() {
        if (performanceLoader.item)
            performanceLoader.item.qmlFramePainted()
    }
    // qmllint enable

    function closeScene() {
        sceneSelectionPanel.sceneSource = ""
    }
}
