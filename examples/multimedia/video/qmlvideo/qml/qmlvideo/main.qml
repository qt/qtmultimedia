/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

Rectangle {
    id: root
    width: 640
    height: 360
    color: "black"

    property string source1
    property string source2
    property color bgColor: "#002244"
    property real volume: 0.25
    property bool perfMonitorsLogging: false
    property bool perfMonitorsVisible: false

    QtObject {
        id: d
        property int itemHeight: 40
        property int buttonHeight: 0.8 * itemHeight
        property int margins: 10
    }

    // Create ScreenSaver element via Loader, so this app will still run if the
    // SystemInfo module is not available
    Loader {
        source: "DisableScreenSaver.qml"
    }

    Loader {
        id: performanceLoader

        Connections {
            target: inner
            onVisibleChanged:
                if (performanceLoader.item)
                    performanceLoader.item.enabled = !inner.visible
            ignoreUnknownSignals: true
        }

        function init() {
            console.log("[qmlvideo] performanceLoader.init logging " + root.perfMonitorsLogging + " visible " + root.perfMonitorsVisible)
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
            height: d.buttonHeight
            text: (root.source1 == "") ? "Select file 1" : root.source1
            onClicked: fileBrowser1.show()
        }

        Button {
            id: openFile2Button
            anchors {
                top: openFile1Button.bottom
                left: parent.left
                right: exitButton.left
                margins: d.margins
            }
            height: d.buttonHeight
            text: (root.source2 == "") ? "Select file 2" : root.source2
            onClicked: fileBrowser2.show()
        }

        Button {
            id: exitButton
            anchors {
                top: parent.top
                right: parent.right
                margins: d.margins
            }
            width: 50
            height: d.buttonHeight
            text: "Exit"
            onClicked: Qt.quit()
        }

        SceneSelectionPanel {
            id: sceneSelectionPanel
            itemHeight: d.itemHeight
            color: "#004444"
            anchors {
                top: openFile2Button.bottom
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: d.margins
            }
            radius: 10
            onSceneSourceChanged: {
                console.log("[qmlvideo] main.onSceneSourceChanged source " + sceneSource)
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
        onVideoFramePainted: {
            if (performanceLoader.item)
                performanceLoader.item.videoFramePainted()
        }
        ignoreUnknownSignals: true
    }

    FileBrowser {
        id: fileBrowser1
        anchors.fill: root
        onFolderChanged: fileBrowser2.folder = folder
        Component.onCompleted: fileSelected.connect(root.openFile1)
    }

    FileBrowser {
        id: fileBrowser2
        anchors.fill: root
        onFolderChanged: fileBrowser1.folder = folder
        Component.onCompleted: fileSelected.connect(root.openFile2)
    }

    function openFile1(path) {
        root.source1 = path
    }

    function openFile2(path) {
        root.source2 = path
    }

    ErrorDialog {
        id: errorDialog
        anchors.fill: parent
        enabled: false
    }

    // Called from main() once root properties have been set
    function init() {
        performanceLoader.init()
        fileBrowser1.folder = videoPath
        fileBrowser2.folder = videoPath
    }

    function qmlFramePainted() {
        if (performanceLoader.item)
            performanceLoader.item.qmlFramePainted()
    }

    function closeScene() {
        console.log("[qmlvideo] main.closeScene")
        sceneSelectionPanel.sceneSource = ""
    }
}
