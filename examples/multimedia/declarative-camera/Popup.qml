// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Rectangle {
    id: popup

    radius: 5
    border.color: "#000000"
    border.width: 2
    smooth: true
    color: "#5e5e5e"

    state: "invisible"

    states: [
        State {
            name: "invisible"
            PropertyChanges { popup.opacity: 0 }
        },

        State {
            name: "visible"
            PropertyChanges { popup.opacity: 1.0 }
        }
    ]

    transitions: Transition {
        NumberAnimation { properties: "opacity"; duration: 100 }
    }

    function toggle() {
        if (state == "visible")
            state = "invisible";
        else
            state = "visible";
    }
}
