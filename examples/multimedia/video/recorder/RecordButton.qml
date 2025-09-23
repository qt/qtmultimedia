// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: root
    width: outerRadius * 2
    height: outerRadius * 2

    required property bool recording

    property int outerRadius: Style.height
    property int innerRadius: mouse.pressedButtons === Qt.LeftButton ? outerRadius - 6 : outerRadius - 5

    signal clicked

    Rectangle {
        anchors.fill: parent
        radius: root.outerRadius
        opacity: 0.5
        border.color: "black"
        border.width: 1
    }

    Rectangle {
        anchors.centerIn: parent
        width: root.recording ? root.innerRadius * 2 - 15 : root.innerRadius * 2
        height: root.recording ? root.innerRadius * 2 - 15 : root.innerRadius * 2
        radius: root.recording ? 2 : root.innerRadius
        color: "red"

        Behavior on width { NumberAnimation { duration: 100 }}
        Behavior on height { NumberAnimation { duration: 100 }}
        Behavior on radius { NumberAnimation { duration: 100 }}

        MouseArea {
            id: mouse
            anchors.fill: parent
            onClicked: root.clicked()
        }
    }
}
