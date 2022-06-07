// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: root
    color: "transparent"
    opacity: 0.0
    property alias enabled: mouseArea.enabled
    property int dialogWidth: 300
    property int dialogHeight: 200
    state: enabled ? "on" : "baseState"

    states: [
        State {
            name: "on"
            PropertyChanges {
                target: root
                opacity: 1.0
            }
        }
    ]

    transitions: [
        Transition {
            from: "*"
            to: "*"
            NumberAnimation {
                properties: "opacity"
                easing.type: Easing.OutQuart
                duration: 500
            }
        }
    ]

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.75
    }

    Rectangle {
        anchors.centerIn: parent
        width: dialogWidth
        height: dialogHeight
        radius: 5
        color: "white"

        Text {
            id: text
            anchors.fill: parent
            anchors.margins: 10
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            color: "black"
            wrapMode: Text.WordWrap
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: root.enabled = false
    }

    function show(msg) {
        text.text = "<b>Error</b><br><br>" + msg
        root.enabled = true
    }
}
