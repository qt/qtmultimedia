// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: root

    property string text
    property color bgColor: "#757575"
    property color bgColorSelected: "#bdbdbd"
    property color textColor: "white"
    property color textColorSelected: "black"
    property alias enabled: mouseArea.enabled
    property alias radius: bgr.radius

    signal clicked

    Rectangle {
        id: bgr
        anchors.fill: parent
        color: mouseArea.pressed ? bgColorSelected : bgColor
        radius: height / 15

        Text {
            id: text
            anchors.centerIn: parent
            text: root.text
            font.pixelSize: 0.4 * parent.height
            color: mouseArea.pressed ? textColorSelected : textColor
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            onClicked: {
                root.clicked()
            }
        }
    }
}
