/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0

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
                color: "yellow"
                font.pixelSize: 20
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
                item.centerIn = cameraHolder
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
