// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property string contentType

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: root.contentType
        source: parent.source1
        volume: parent.volume

        SequentialAnimation on scale {
            id: animation
            loops: Animation.Infinite
            property int duration: 1500
            running: true
            PropertyAnimation {
                from: 1.5
                to: 0.5
                duration: animation.duration
                easing.type: Easing.InOutCubic
            }
            PropertyAnimation {
                from: 0.5
                to: 1.5
                duration: animation.duration
                easing.type: Easing.InOutCubic
            }
        }

        onVideoFramePainted: root.videoFramePainted()
    }

    Component.onCompleted: root.content = content
}
