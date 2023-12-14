// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property int margin: 20
    property string contentType

    Content {
        id: content
        anchors.verticalCenter: parent.verticalCenter
        width: parent.contentWidth
        contentType: root.contentType
        source: parent.source1
        volume: parent.volume

        SequentialAnimation on x {
            id: animation
            loops: Animation.Infinite
            property int from: margin
            property int to: 100
            property int duration: 1500
            running: false
            PropertyAnimation {
                from: animation.from
                to: animation.to
                duration: animation.duration
                easing.type: Easing.InOutCubic
            }
            PropertyAnimation {
                from: animation.to
                to: animation.from
                duration: animation.duration
                easing.type: Easing.InOutCubic
            }
        }

        onVideoFramePainted: root.videoFramePainted()
    }

    onWidthChanged: {
        animation.to = root.width - content.width - margin
        animation.start()
    }

    Component.onCompleted: root.content = content
}
