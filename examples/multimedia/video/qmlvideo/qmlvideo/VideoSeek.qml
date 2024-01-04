// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Scene {
    id: root
    property string contentType: "video"
    contentWidth: parent.width

    Content {
        id: content
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: "video"
        source: parent.source1
        volume: parent.volume
        onVideoFramePainted: root.videoFramePainted()
    }

    SeekControl {
        anchors {
            left: parent.left
            right: parent.right
            margins: 10
            bottom: parent.bottom
        }
        // qmllint disable
        duration: content.contentItem?.duration ?? 0
        playPosition: content.contentItem?.position ?? 0
        onSeekPositionChanged: content.contentItem?.seek(seekPosition);
        // qmllint enable
    }

    Component.onCompleted: root.content = content
}
