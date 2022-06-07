// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property int margin: 20
    property string contentType

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: root.contentType
        source: parent.source1
        volume: parent.volume

        PropertyAnimation on rotation {
            id: animation
            loops: Animation.Infinite
            running: true
            from: 0
            to: 360
            duration: 3000
            easing.type: Easing.Linear
        }

        onVideoFramePainted: root.videoFramePainted()
    }


    Component.onCompleted: root.content = content
}
