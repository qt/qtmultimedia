// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id : zoomControl
    property real currentZoom : 1
    property real maximumZoom : 1
    property real minimumZoom : 1
    signal zoomTo(real target)

    visible: zoomControl.maximumZoom > zoomControl.minimumZoom

    MouseArea {
        id : mouseArea
        anchors.fill: parent

        property real initialZoom : 0
        property real initialPos : 0

        onPressed: {
            initialPos = mouseY
            initialZoom = zoomControl.currentZoom
        }

        onPositionChanged: {
            if (pressed) {
                var target = initialZoom * Math.pow(5, (initialPos-mouseY)/zoomControl.height);
                target = Math.max(zoomControl.minimumZoom, Math.min(target, zoomControl.maximumZoom))
                zoomControl.zoomTo(target)
            }
        }
    }

    Item {
        id : bar
        x : 16
        y : parent.height/4
        width : 24
        height : parent.height/2

        Rectangle {
            anchors.fill: parent

            smooth: true
            radius: 8
            border.color: "white"
            border.width: 2
            color: "black"
            opacity: 0.3
        }

        Rectangle {
            id: groove
            x : 0
            y : parent.height * (1.0 - (zoomControl.currentZoom-zoomControl.minimumZoom) / (zoomControl.maximumZoom-zoomControl.minimumZoom))
            width: parent.width
            height: parent.height - y
            smooth: true
            radius: 8
            color: "white"
            opacity: 0.5
        }

        Text {
            id: zoomText
            anchors {
                left: bar.right; leftMargin: 16
            }
            y: Math.min(parent.height - height, Math.max(0, groove.y - height / 2))
            text: "x" + Math.round(zoomControl.currentZoom * 100) / 100
            font.bold: true
            color: "white"
            style: Text.Raised; styleColor: "black"
            opacity: 0.85
            font.pixelSize: 18
        }
    }
}
