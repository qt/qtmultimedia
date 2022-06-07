// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    color: "black"
    property alias buttonHeight: closeButton.height
    property string source1
    property string source2
    property int contentWidth: parent.width / 2
    property real volume: 0.25
    property int margins: 5
    property QtObject content

    signal close
    signal videoFramePainted

    Button {
        id: closeButton
        anchors {
            top: parent.top
            right: parent.right
            margins: root.margins
        }
        width: Math.max(parent.width, parent.height) / 12
        height: Math.min(parent.width, parent.height) / 12
        z: 2.0
        bgColor: "#212121"
        bgColorSelected: "#757575"
        textColorSelected: "white"
        text: "Back"
        onClicked: root.close()
    }
}
