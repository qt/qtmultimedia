// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property string contentType
    property bool autoStart: false
    property bool started: false

    Content {
        id: content
        autoStart: parent.autoStart
        started: parent.started
        anchors.fill: parent
        width: parent.contentWidth
        contentType: parent.contentType
        source: parent.source1
        volume: parent.volume
        onVideoFramePainted: root.videoFramePainted()
    }

    Text {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
            margins: 20
        }
        text: content.started ? "Tap the screen to stop content"
                              : "Tap the screen to start content"
        color: "#e0e0e0"
        z: 2.0
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (content.started)
                content.stop()
            else
                content.start()
        }
    }

    Component.onCompleted: root.content = content
}
