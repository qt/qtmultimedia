// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window

Window {
    id: root
    visible: true
    title: "Media recorder"
    width: Style.screenWidth
    height: Style.screenHeigth

    StyleRectangle {
        anchors.fill: parent
        Text {
            anchors.fill: parent
            anchors.margins: 20
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("The example is not usable without the permissions.\n"
                       + "Please grant all requested permissions and restart the application.")
        }
    }
}
