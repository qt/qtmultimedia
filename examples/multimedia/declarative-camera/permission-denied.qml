// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick


Rectangle {
    color: "black"
    width: 800
    height: 600

    Text {
        anchors.fill: parent
        text: qsTr("Grant the camera permission and restart the app")
        color: "white"
        font.pointSize: 20
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
