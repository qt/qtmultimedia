// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtMultimedia

Scene {
    id: root
    property string contentType: "video"

    Content {
        id: content
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
            property var videoMetaData: content.contentItem().metaData
            Text {
                color: "#e0e0e0"
                text: "Title:" + videoMetaData.value(MediaMetaData.Title)
            }
            Text {
                color: "#e0e0e0"
                text: "Resolution:" + videoMetaData.value(MediaMetaData.Resolution)
            }
            Text {
                color: "#e0e0e0"
                text: "Media type:" + videoMetaData.value(MediaMetaData.MediaType)
            }
            Text {
                color: "#e0e0e0"
                text: "Video codec:" + videoMetaData.value(MediaMetaData.VideoCodec)
            }
            Text {
                color: "#e0e0e0"
                text: "Video bit rate:" + videoMetaData.value(MediaMetaData.VideoBitRate)
            }
            Text {
                color: "#e0e0e0"
                text: "Video frame rate:" +videoMetaData.value(MediaMetaData.VideoFrameRate)
            }
            Text {
                color: "#e0e0e0"
                text: "Audio codec:" + videoMetaData.value(MediaMetaData.AudioCodec)
            }
            Text {
                color: "#e0e0e0"
                text: "Audio bit rate:" + videoMetaData.value(MediaMetaData.AudioBitRate)
            }
            Text {
                color: "#e0e0e0"
                text: "Date:" + videoMetaData.value(MediaMetaData.Date)
            }
            Text {
                color: "#e0e0e0"
                text: "Description:" + videoMetaData.value(MediaMetaData.Description)
            }
            Text {
                color: "#e0e0e0"
                text: "Copyright:" + videoMetaData.value(MediaMetaData.Copyright)
            }
            Text {
                color: "#e0e0e0"
                text: "Seekable:" + content.contentItem().seekable
            }
            Text {
                color: "#e0e0e0"
                text: "Orientation:" + videoMetaData.value(MediaMetaData.Orientation)
            }
        }
    }

    Component.onCompleted: root.content = content
}
