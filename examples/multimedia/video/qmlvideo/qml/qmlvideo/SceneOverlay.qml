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
        onVideoFramePainted: root.videoFramePainted()
    }

    Rectangle {
        id: overlay
        y: 0.5 * parent.height
        width: content.width
        height: content.height
        color: "#e0e0e0"
        opacity: 0.5

        SequentialAnimation on x {
            id: xAnimation
            loops: Animation.Infinite
            property int from: margin
            property int to: 100
            property int duration: 1500
            running: false
            PropertyAnimation {
                from: xAnimation.from
                to: xAnimation.to
                duration: xAnimation.duration
                easing.type: Easing.InOutCubic
            }
            PropertyAnimation {
                from: xAnimation.to
                to: xAnimation.from
                duration: xAnimation.duration
                easing.type: Easing.InOutCubic
            }
        }

        SequentialAnimation on y {
            id: yAnimation
            loops: Animation.Infinite
            property int from: margin
            property int to: 180
            property int duration: 1500
            running: false
            PropertyAnimation {
                from: yAnimation.from
                to: yAnimation.to
                duration: yAnimation.duration
                easing.type: Easing.InOutCubic
            }
            PropertyAnimation {
                from: yAnimation.to
                to: yAnimation.from
                duration: yAnimation.duration
                easing.type: Easing.InOutCubic
            }
        }
    }

    onWidthChanged: {
        xAnimation.to = root.width - content.width - margin
        xAnimation.start()
    }

    onHeightChanged: {
        //yAnimation.to = root.height - content.height - margin
        yAnimation.start()
    }

    Component.onCompleted: root.content = content
}
