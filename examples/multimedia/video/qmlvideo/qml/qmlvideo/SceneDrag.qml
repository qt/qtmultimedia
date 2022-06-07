// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property int margin: 20
    property string contentType

    Image {
        id: background
        source: "qrc:/images/leaves.jpg"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        Content {
            id: content
            anchors.centerIn: parent
            width: root.contentWidth
            contentType: root.contentType
            source: root.source1
            volume: root.volume
            onVideoFramePainted: root.videoFramePainted()
        }
    }

    MouseArea {
        anchors.fill: parent
        drag.target: background
    }

    Component.onCompleted: root.content = content
}
