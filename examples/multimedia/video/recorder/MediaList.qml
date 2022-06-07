// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtMultimedia
import QtQuick.Layouts

Item {
    id: root

    required property Playback playback

    property string mediaThumbnail
    property string mediaUrl

    function append() {
        if (mediaUrl !== "")
            mediaList.append({"thumbnail": root.mediaThumbnail, "url": root.mediaUrl})
        mediaThumbnail = ""
        mediaUrl = ""
    }

    ListModel { id: mediaList }

    ListView {
        id: listView
        anchors.fill: parent
        model:  mediaList
        orientation: ListView.Horizontal
        spacing: Style.intraSpacing

        delegate: Frame {
            padding: Style.intraSpacing
            width: root.height
            height: root.height
            background: StyleRectangle { anchors.fill: parent }

            required property string url
            required property string thumbnail

            ColumnLayout {
                anchors.fill: parent
                Image {
                    id: image
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    source: thumbnail
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    Layout.fillWidth: true
                    elide: Text.ElideLeft
                    text: url
                }
            }
            RoundButton {
                anchors.centerIn: parent
                width: 30
                height: 30
                text: "\u25B6";
                onClicked: { playback.playUrl(url) }
            }
        }
    }
}
