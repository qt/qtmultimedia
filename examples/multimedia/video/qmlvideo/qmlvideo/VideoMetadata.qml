// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia

pragma ComponentBehavior: Bound

Scene {
    id: root
    property string contentType: "video"

    Content {
        id: videoContent
        anchors.centerIn: parent
        width: parent.contentWidth
        contentType: "video"
        source: parent.source1
        volume: parent.volume
        onInitialized: {
            if (!dummy)
                metadata.createObject(root)
        }
        onVideoFramePainted: root.videoFramePainted()
    }

    Component {
        id: metadata
        Column {
            anchors.fill: parent
            // qmllint disable
            property var videoMetaData: videoContent.contentItem?.metaData
            // qmllint enable
            Label {
                text: qsTr("Title: %1").arg(parent.videoMetaData?.value(MediaMetaData.Title) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Resolution: %1").arg(parent.videoMetaData?.value(MediaMetaData.Resolution) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Media type: %1").arg(parent.videoMetaData?.value(MediaMetaData.MediaType) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Video codec: %1").arg(parent.videoMetaData?.value(MediaMetaData.VideoCodec) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Video bit rate: %1").arg(parent.videoMetaData?.value(MediaMetaData.VideoBitRate) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Video frame rate: %1").arg(parent.videoMetaData?.value(MediaMetaData.VideoFrameRate) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Audio codec: %1").arg(parent.videoMetaData?.value(MediaMetaData.AudioCodec) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Audio bit rate: %1").arg(parent.videoMetaData?.value(MediaMetaData.AudioBitRate) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Date: %1").arg(parent.videoMetaData?.value(MediaMetaData.Date) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Description: %1").arg(parent.videoMetaData?.value(MediaMetaData.Description) ?? qsTr("Unknown"))
            }
            Label {
                text: qsTr("Copyright: %1").arg(parent.videoMetaData?.value(MediaMetaData.Copyright) ?? qsTr("Unknown"))
            }
            Label {
                // qmllint disable
                text: qsTr("Seekable: %1").arg(videoContent.contentItem?.seekable ?? qsTr("Unknown"))
                // qmllint enable
            }
            Label {
                text: qsTr("Orientation: %1").arg(parent.videoMetaData?.value(MediaMetaData.Orientation) ?? qsTr("Unknown"))
            }
        }
    }

    Component.onCompleted: root.content = videoContent
}
